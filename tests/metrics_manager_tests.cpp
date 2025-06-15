#include <gtest/gtest.h>
#include "MetricsManager/MetricsManager.h"
#include "IMetrics/CPUUsageMetric.h"
#include "IMetrics/CPUUtilMetric.h"
#include "IMetrics/HTTPIncomeMetric.h"
#include "IMetrics/LatencyMetric.h"
#include "IMetrics/CodeTimeMetric.h"
#include "IMetrics/IncrementMetric.h"
#include "IMetrics/CardinalityMetricType.h"
#include "IMetrics/CardinalityMetricValue.h"
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <random>
#include <regex>

class MetricsManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_file_name_ = "test_metrics_" + std::to_string(test_counter_++) + ".log";
        manager_ = std::make_unique<MetricsManager<>>(test_file_name_);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    void TearDown() override {
        manager_.reset();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (std::filesystem::exists(test_file_name_)) {
            std::filesystem::remove(test_file_name_);
        }
    }

    std::string ReadLogFile() {
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
        std::ifstream file(test_file_name_);
        if (!file.is_open()) return "";
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        return content;
    }

    int CountLinesInLog() {
        std::string content = ReadLogFile();
        return std::count(content.begin(), content.end(), '\n');
    }

    bool LogContains(const std::string& text) {
        return ReadLogFile().find(text) != std::string::npos;
    }

    std::unique_ptr<MetricsManager<>> manager_;
    std::string test_file_name_;
    static inline std::atomic<int> test_counter_{0};
};

TEST_F(MetricsManagerTest, CreateIncrementMetricWithObject) {
    auto* metric = manager_->CreateMetric<Metrics::IncrementMetric>("TestCounter", 42);
    ASSERT_NE(metric, nullptr);
    EXPECT_EQ(metric->GetName(), "TestCounter");
    EXPECT_EQ(metric->GetValueAsString(), "42");
}

TEST_F(MetricsManagerTest, CreateIncrementMetricWithArgs) {
    auto* ptr = manager_->CreateMetric<Metrics::IncrementMetric>("ArgsCounter", 100);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->GetName(), "ArgsCounter");
    EXPECT_EQ(ptr->GetValueAsString(), "100");
}

TEST_F(MetricsManagerTest, CreateCPUUsageMetric) {
    auto* ptr = manager_->CreateMetric<Metrics::CPUUsageMetric>();
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->GetName(), "\"CPU Usage\"");
    ptr->Evaluate();
    std::string value = ptr->GetValueAsString();
    EXPECT_TRUE(value.find("%") != std::string::npos);
}

TEST_F(MetricsManagerTest, CreateCPUUtilMetric) {
    auto* ptr = manager_->CreateMetric<Metrics::CPUMetric>();
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->GetName(), "\"CPU\"");
    ptr->Evaluate();
    std::string value = ptr->GetValueAsString();
    EXPECT_FALSE(value.empty());
}

TEST_F(MetricsManagerTest, CreateHTTPIncomeMetric) {
    auto* ptr = manager_->CreateMetric<Metrics::HTTPIncomeMetric>(50);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->GetName(), "\"HTTPS requests RPS\"");
    ++(*ptr);
    ++(*ptr);
    ptr->Evaluate();
    EXPECT_EQ(ptr->GetValueAsString(), "2.00");
}

TEST_F(MetricsManagerTest, CreateLatencyMetric) {
    auto* ptr = manager_->CreateMetric<Metrics::LatencyMetric>();
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->GetName(), "\"Percentile Latency\"");
    ptr->Observe(std::chrono::nanoseconds(1000000));
    ptr->Observe(std::chrono::nanoseconds(2000000));
    std::string value = ptr->GetValueAsString();
    EXPECT_TRUE(value.find("P90") != std::string::npos);
    EXPECT_TRUE(value.find("P95") != std::string::npos);
    EXPECT_TRUE(value.find("P99") != std::string::npos);
    EXPECT_TRUE(value.find("P999") != std::string::npos);
}

TEST_F(MetricsManagerTest, CreateCodeTimeMetric) {
    auto* ptr = manager_->CreateMetric<Metrics::CodeTimeMetric>("TestAlgorithm");
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->GetName(), "TestAlgorithm");
    ptr->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ptr->Stop();
    std::string value = ptr->GetValueAsString();
    EXPECT_TRUE(value.find("ms") != std::string::npos || 
                value.find("μs") != std::string::npos || 
                value.find("ns") != std::string::npos);
}

TEST_F(MetricsManagerTest, CreateCardinalityMetricType) {
    auto* ptr = manager_->CreateMetric<Metrics::CardinalityMetricType>(3);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->GetName(), "\"CardinalityType\"");
    ptr->Observe(42);
    ptr->Observe(3.14);
    ptr->Observe(std::string("test"));
    ptr->Observe(42);
    std::string value = ptr->GetValueAsString();
    EXPECT_TRUE(value.find("unique elements: 3") != std::string::npos);
    EXPECT_TRUE(value.find("most frequent types") != std::string::npos);
}

TEST_F(MetricsManagerTest, CreateCardinalityMetricValue) {
    auto* ptr = manager_->CreateMetric<Metrics::CardinalityMetricValue<int, std::string>>(2);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->GetName(), "\"CardinalityValue\"");
    ptr->Observe(42, 3);
    ptr->Observe(std::string("hello"), 2);
    ptr->Observe(100, 1);
    ptr->Observe(42, 1);
    std::string value = ptr->GetValueAsString();
    EXPECT_TRUE(value.find("unique elements: 3") != std::string::npos);
    EXPECT_TRUE(value.find("quantity") != std::string::npos);
}

TEST_F(MetricsManagerTest, GetMetricValidIndex) {
    manager_->CreateMetric<Metrics::IncrementMetric>("Counter1", 10);
    manager_->CreateMetric<Metrics::CPUUsageMetric>();
    manager_->CreateMetric<Metrics::HTTPIncomeMetric>(5);
    
    auto* metric1 = manager_->GetMetric<Metrics::IncrementMetric>(0);
    auto* metric2 = manager_->GetMetric<Metrics::CPUUsageMetric>(1);
    auto* metric3 = manager_->GetMetric<Metrics::HTTPIncomeMetric>(2);
    
    ASSERT_NE(metric1, nullptr);
    ASSERT_NE(metric2, nullptr);
    ASSERT_NE(metric3, nullptr);
    
    EXPECT_EQ(metric1->GetName(), "Counter1");
    EXPECT_EQ(metric2->GetName(), "\"CPU Usage\"");
    EXPECT_EQ(metric3->GetName(), "\"HTTPS requests RPS\"");
}

TEST_F(MetricsManagerTest, GetMetricInvalidIndex) {
    manager_->CreateMetric<Metrics::IncrementMetric>("Counter", 5);
    
    EXPECT_THROW(manager_->GetMetric<Metrics::IncrementMetric>(1), std::out_of_range);
    EXPECT_THROW(manager_->GetMetric<Metrics::IncrementMetric>(100), std::out_of_range);
}

TEST_F(MetricsManagerTest, GetMetricWrongType) {
    manager_->CreateMetric<Metrics::IncrementMetric>("Counter", 5);
    manager_->CreateMetric<Metrics::CPUUsageMetric>();
    manager_->CreateMetric<Metrics::HTTPIncomeMetric>(10);
    
    EXPECT_THROW(manager_->GetMetric<Metrics::CPUUsageMetric>(0), std::runtime_error);
    EXPECT_THROW(manager_->GetMetric<Metrics::IncrementMetric>(1), std::runtime_error);
    EXPECT_THROW(manager_->GetMetric<Metrics::LatencyMetric>(2), std::runtime_error);
}

TEST_F(MetricsManagerTest, LogSingleMetricByIndex) {
    auto* metric = manager_->CreateMetric<Metrics::IncrementMetric>("TestCounter", 42);
    ++(*metric);
    ++(*metric);
    
    EXPECT_EQ(metric->GetValueAsString(), "44");
    
    manager_->Log(0);
    
    EXPECT_TRUE(LogContains("TestCounter"));
    EXPECT_TRUE(LogContains("44"));
    EXPECT_EQ(metric->GetValueAsString(), "0");
}

TEST_F(MetricsManagerTest, IncrementMetricFunctionality) {
    auto* metric = manager_->CreateMetric<Metrics::IncrementMetric>("TestCounter", 0);
    
    ++(*metric);
    ++(*metric);
    ++(*metric);
    
    EXPECT_EQ(metric->GetValueAsString(), "3");
    
    manager_->Log(0);
    EXPECT_TRUE(LogContains("TestCounter"));
    EXPECT_TRUE(LogContains("3"));
    EXPECT_EQ(metric->GetValueAsString(), "0");
}

// TEST_F(MetricsManagerTest, HTTPIncomeMetricFunctionality) {
//     auto* metric = manager_->CreateMetric<Metrics::HTTPIncomeMetric>(0);
    
//     for (int i = 0; i < 25; ++i) {
//         ++(*metric);
//     }
    
//     metric->Evaluate();
//     EXPECT_EQ(metric->GetValueAsString(), "25.00");
    
//     manager_->Log(0);
//     EXPECT_EQ(metric->GetValueAsString(), "0.00");
    
//     EXPECT_TRUE(LogContains("HTTPS requests RPS"));
// }

TEST_F(MetricsManagerTest, CPUUsageMetricFunctionality) {
    auto* metric = manager_->CreateMetric<Metrics::CPUUsageMetric>();
    
    metric->Evaluate();
    std::string value_before = metric->GetValueAsString();
    EXPECT_TRUE(value_before.find("%") != std::string::npos);
    
    manager_->Log(0);
    EXPECT_TRUE(LogContains("CPU Usage"));
    
    std::string value_after = metric->GetValueAsString();
    EXPECT_TRUE(value_after.find("%") != std::string::npos);
}

TEST_F(MetricsManagerTest, CPUUtilMetricFunctionality) {
    auto* metric = manager_->CreateMetric<Metrics::CPUMetric>();
    
    metric->Evaluate();
    std::string value = metric->GetValueAsString();
    EXPECT_FALSE(value.empty());
    
    manager_->Log(0);
    EXPECT_TRUE(LogContains("\"CPU\""));
}

TEST_F(MetricsManagerTest, LatencyMetricFunctionality) {
    auto* metric = manager_->CreateMetric<Metrics::LatencyMetric>();
    
    metric->Observe(std::chrono::nanoseconds(1000000));
    metric->Observe(std::chrono::nanoseconds(2000000));
    metric->Observe(std::chrono::nanoseconds(3000000));
    metric->Observe(std::chrono::nanoseconds(4000000));
    metric->Observe(std::chrono::nanoseconds(5000000));
    
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("P90") != std::string::npos);
    EXPECT_TRUE(value.find("P95") != std::string::npos);
    EXPECT_TRUE(value.find("P99") != std::string::npos);
    EXPECT_TRUE(value.find("P999") != std::string::npos);
    
    manager_->Log(0);
    EXPECT_TRUE(LogContains("Percentile Latency"));
}

TEST_F(MetricsManagerTest, CodeTimeMetricFunctionality) {
    auto* metric = manager_->CreateMetric<Metrics::CodeTimeMetric>("TestAlgorithm");
    
    metric->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    metric->Stop();
    
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("ms") != std::string::npos || 
                value.find("μs") != std::string::npos || 
                value.find("ns") != std::string::npos);
    
    manager_->Log(0);
    EXPECT_TRUE(LogContains("TestAlgorithm"));
}

TEST_F(MetricsManagerTest, CardinalityMetricTypeFunctionality) {
    auto* metric = manager_->CreateMetric<Metrics::CardinalityMetricType>(3);
    
    metric->Observe(42);
    metric->Observe(3.14);
    metric->Observe(std::string("test"));
    metric->Observe(42);
    metric->Observe(99);
    
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("unique elements: 4") != std::string::npos);
    EXPECT_TRUE(value.find("most frequent types") != std::string::npos);
    
    manager_->Log(0);
    EXPECT_TRUE(LogContains("CardinalityType"));
}

TEST_F(MetricsManagerTest, CardinalityMetricValueFunctionality) {
    auto* metric = manager_->CreateMetric<Metrics::CardinalityMetricValue<int, std::string>>(2);
    
    metric->Observe(42, 3);
    metric->Observe(std::string("hello"), 2);
    metric->Observe(100, 1);
    metric->Observe(42, 1);
    
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("unique elements: 3") != std::string::npos);
    EXPECT_TRUE(value.find("quantity") != std::string::npos);
    
    manager_->Log(0);
    EXPECT_TRUE(LogContains("CardinalityValue"));
}

TEST_F(MetricsManagerTest, LogAllMetricsDefault) {
    manager_->CreateMetric<Metrics::IncrementMetric>("Counter1", 10);
    manager_->CreateMetric<Metrics::IncrementMetric>("Counter2", 20);
    manager_->CreateMetric<Metrics::CPUUsageMetric>();
    manager_->CreateMetric<Metrics::HTTPIncomeMetric>(30);
    
    manager_->Log();
    
    EXPECT_TRUE(LogContains("Counter1"));
    EXPECT_TRUE(LogContains("Counter2"));
    EXPECT_TRUE(LogContains("CPU Usage"));
    EXPECT_TRUE(LogContains("HTTPS requests RPS"));
    EXPECT_GE(CountLinesInLog(), 4);
}

TEST_F(MetricsManagerTest, LogMetricsByDefaultTag) {
    manager_->CreateMetric<Metrics::IncrementMetric>("DefaultCounter", 10);
    manager_->CreateMetric<Metrics::CPUUsageMetric>();
    manager_->CreateMetric<Metrics::HTTPIncomeMetric>(5);
    manager_->CreateMetric<Metrics::CodeTimeMetric>("TestAlgo");
    manager_->CreateMetric<Metrics::CardinalityMetricType>(3);
    
    manager_->Log<MetricTags::DefaultMetricTag>();
    
    std::string log_content = ReadLogFile();
    EXPECT_TRUE(log_content.find("DefaultCounter") != std::string::npos);
    EXPECT_TRUE(log_content.find("CPU Usage") != std::string::npos);
    EXPECT_TRUE(log_content.find("HTTPS") != std::string::npos);
    EXPECT_TRUE(log_content.find("TestAlgo") != std::string::npos);
    EXPECT_TRUE(log_content.find("CardinalityType") != std::string::npos);
}

TEST_F(MetricsManagerTest, LogMetricsByComputerTag) {
    manager_->CreateMetric<Metrics::CPUUsageMetric>();
    manager_->CreateMetric<Metrics::CPUMetric>();
    manager_->CreateMetric<Metrics::LatencyMetric>();
    manager_->CreateMetric<Metrics::HTTPIncomeMetric>(50);
    manager_->CreateMetric<Metrics::CodeTimeMetric>("Algorithm1");
    manager_->CreateMetric<Metrics::IncrementMetric>("Counter", 5);
    
    manager_->Log<MetricTags::ComputerMetricTag>();
    
    std::string log_content = ReadLogFile();
    EXPECT_TRUE(log_content.find("CPU Usage") != std::string::npos);
    EXPECT_TRUE(log_content.find("\"CPU\"") != std::string::npos);
    EXPECT_TRUE(log_content.find("Percentile Latency") != std::string::npos);
    EXPECT_FALSE(log_content.find("HTTPS") != std::string::npos);
    EXPECT_FALSE(log_content.find("Algorithm1") != std::string::npos);
    EXPECT_FALSE(log_content.find("Counter") != std::string::npos);
}

TEST_F(MetricsManagerTest, LogMetricsByServerTag) {
    manager_->CreateMetric<Metrics::HTTPIncomeMetric>(100);
    manager_->CreateMetric<Metrics::CPUUsageMetric>();
    manager_->CreateMetric<Metrics::IncrementMetric>("General", 5);
    manager_->CreateMetric<Metrics::CodeTimeMetric>("TestAlgo");
    
    manager_->Log<MetricTags::ServerMetricTag>();
    
    std::string log_content = ReadLogFile();
    EXPECT_TRUE(log_content.find("HTTPS") != std::string::npos);
    EXPECT_FALSE(log_content.find("CPU Usage") != std::string::npos);
    EXPECT_FALSE(log_content.find("General") != std::string::npos);
    EXPECT_FALSE(log_content.find("TestAlgo") != std::string::npos);
}

TEST_F(MetricsManagerTest, LogMetricsByAlgoTag) {
    manager_->CreateMetric<Metrics::CodeTimeMetric>("TestAlgorithm");
    manager_->CreateMetric<Metrics::CPUUsageMetric>();
    manager_->CreateMetric<Metrics::HTTPIncomeMetric>(25);
    manager_->CreateMetric<Metrics::IncrementMetric>("Counter", 10);
    
    manager_->Log<MetricTags::AlgoMetricTag>();
    
    std::string log_content = ReadLogFile();
    EXPECT_TRUE(log_content.find("TestAlgorithm") != std::string::npos);
    EXPECT_FALSE(log_content.find("CPU Usage") != std::string::npos);
    EXPECT_FALSE(log_content.find("HTTPS") != std::string::npos);
    EXPECT_FALSE(log_content.find("Counter") != std::string::npos);
}

TEST_F(MetricsManagerTest, MetricResetAfterLog) {
    auto* counter = manager_->CreateMetric<Metrics::IncrementMetric>("ResetTest", 5);
    auto* http_metric = manager_->CreateMetric<Metrics::HTTPIncomeMetric>(0);
    auto* cardinality = manager_->CreateMetric<Metrics::CardinalityMetricType>(3);
    
    ++(*counter);
    ++(*counter);
    ++(*http_metric);
    ++(*http_metric);
    ++(*http_metric);
    cardinality->Observe(42);
    cardinality->Observe(3.14);
    
    EXPECT_EQ(counter->GetValueAsString(), "7");
    http_metric->Evaluate();
    EXPECT_EQ(http_metric->GetValueAsString(), "3.00");
    EXPECT_TRUE(cardinality->GetValueAsString().find("unique elements: 2") != std::string::npos);
    
    manager_->Log();
    
    EXPECT_EQ(counter->GetValueAsString(), "0");
    EXPECT_EQ(http_metric->GetValueAsString(), "0.00");
    EXPECT_TRUE(cardinality->GetValueAsString().find("unique elements: 0") != std::string::npos);
}

TEST_F(MetricsManagerTest, ConcurrentMetricCreation) {
    const int num_threads = 20;
    const int metrics_per_thread = 10;
    std::vector<std::thread> threads;
    std::atomic<int> created_count{0};
    std::vector<std::vector<Metrics::IncrementMetric*>> thread_metrics(num_threads);
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < metrics_per_thread; ++j) {
                auto* metric = manager_->CreateMetric<Metrics::IncrementMetric>(
                    "Thread" + std::to_string(i) + "_Metric" + std::to_string(j), j);
                if (metric != nullptr) {
                    thread_metrics[i].push_back(metric);
                    created_count++;
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(created_count.load(), num_threads * metrics_per_thread);
    
    for (int i = 0; i < num_threads; ++i) {
        EXPECT_EQ(thread_metrics[i].size(), metrics_per_thread);
    }
}

TEST_F(MetricsManagerTest, ConcurrentIncrementMetricsModification) {
    const int num_metrics = 10;
    const int threads_per_metric = 5;
    const int increments_per_thread = 100;
    
    std::vector<Metrics::IncrementMetric*> metrics;
    for (int i = 0; i < num_metrics; ++i) {
        auto* metric = manager_->CreateMetric<Metrics::IncrementMetric>(
            "ConcurrentMetric" + std::to_string(i), 0);
        metrics.push_back(metric);
    }
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_metrics; ++i) {
        for (int t = 0; t < threads_per_metric; ++t) {
            threads.emplace_back([&, i]() {
                for (int j = 0; j < increments_per_thread; ++j) {
                    ++(*metrics[i]);
                }
            });
        }
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    for (int i = 0; i < num_metrics; ++i) {
        int expected_value = threads_per_metric * increments_per_thread;
        EXPECT_EQ(std::stoi(metrics[i]->GetValueAsString()), expected_value);
    }
}

TEST_F(MetricsManagerTest, ConcurrentLoggingWithIncrementMetrics) {
    const int num_metrics = 15;
    std::vector<Metrics::IncrementMetric*> metrics;
    
    for (int i = 0; i < num_metrics; ++i) {
        auto* metric = manager_->CreateMetric<Metrics::IncrementMetric>(
            "LogMetric" + std::to_string(i), 0);
        metrics.push_back(metric);
    }
    
    std::vector<std::thread> threads;
    std::atomic<int> log_count{0};
    
    for (int i = 0; i < num_metrics; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < 50; ++j) {
                ++(*metrics[i]);
            }
            manager_->Log(i);
            log_count++;
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(log_count.load(), num_metrics);
    
    for (auto* metric : metrics) {
        EXPECT_EQ(metric->GetValueAsString(), "0");
    }
    
    EXPECT_GE(CountLinesInLog(), num_metrics);
}

TEST_F(MetricsManagerTest, ConcurrentMixedMetricsOperation) {
    auto* increment_metric = manager_->CreateMetric<Metrics::IncrementMetric>("SharedCounter", 0);
    auto* http_metric = manager_->CreateMetric<Metrics::HTTPIncomeMetric>(0);
    auto* latency_metric = manager_->CreateMetric<Metrics::LatencyMetric>();
    auto* code_timer = manager_->CreateMetric<Metrics::CodeTimeMetric>("SharedTimer");
    
    const int num_threads = 10;
    std::vector<std::thread> threads;
    std::atomic<int> operations_completed{0};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < 100; ++j) {
                ++(*increment_metric);
                ++(*http_metric);
                latency_metric->Observe(std::chrono::nanoseconds(1000000 + i * 100000));
                
                if (j == 50) {
                    code_timer->Start();
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                    code_timer->Stop();
                }
            }
            operations_completed++;
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(operations_completed.load(), num_threads);
    
    int final_increment_value = std::stoi(increment_metric->GetValueAsString());
    EXPECT_EQ(final_increment_value, num_threads * 100);
    
    http_metric->Evaluate();
    EXPECT_EQ(http_metric->GetValueAsString(), "1000.00");
}

TEST_F(MetricsManagerTest, MassiveIncrementMetricsStress) {
    const int num_metrics = 100;
    const int num_threads = 10;
    const int operations_per_thread = 1000;
    
    std::vector<Metrics::IncrementMetric*> metrics;
    for (int i = 0; i < num_metrics; ++i) {
        auto* metric = manager_->CreateMetric<Metrics::IncrementMetric>(
            "StressMetric" + std::to_string(i), 0);
        metrics.push_back(metric);
    }
    
    std::vector<std::thread> worker_threads;
    std::atomic<int> total_operations{0};
    
    for (int t = 0; t < num_threads; ++t) {
        worker_threads.emplace_back([&, t]() {
            std::random_device rd;
            std::mt19937 gen(rd() ^ t);
            std::uniform_int_distribution<> dis(0, num_metrics - 1);
            
            for (int i = 0; i < operations_per_thread; ++i) {
                int metric_idx = dis(gen);
                ++(*metrics[metric_idx]);
                total_operations++;
                
                if (i % 50 == 0) {
                    manager_->Log(metric_idx);
                }
            }
        });
    }
    
    for (auto& t : worker_threads) {
        t.join();
    }
    
    EXPECT_EQ(total_operations.load(), num_threads * operations_per_thread);
    
    manager_->Log();
    
    for (auto* metric : metrics) {
        EXPECT_EQ(metric->GetValueAsString(), "0");
    }
}

TEST_F(MetricsManagerTest, AllMetricTypesIntegration) {
    auto* increment_metric = manager_->CreateMetric<Metrics::IncrementMetric>("IntegrationCounter", 0);
    auto* cpu_usage_metric = manager_->CreateMetric<Metrics::CPUUsageMetric>();
    auto* cpu_util_metric = manager_->CreateMetric<Metrics::CPUMetric>();
    auto* http_metric = manager_->CreateMetric<Metrics::HTTPIncomeMetric>(0);
    auto* latency_metric = manager_->CreateMetric<Metrics::LatencyMetric>();
    auto* code_timer = manager_->CreateMetric<Metrics::CodeTimeMetric>("IntegrationAlgorithm");
    auto* cardinality_type = manager_->CreateMetric<Metrics::CardinalityMetricType>(3);
    auto* cardinality_value = manager_->CreateMetric<Metrics::CardinalityMetricValue<int, std::string>>(2);
    
    for (int i = 0; i < 20; ++i) {
        ++(*increment_metric);
        ++(*http_metric);
        latency_metric->Observe(std::chrono::nanoseconds(1000000 + i * 50000));
        cardinality_type->Observe(i % 5);
        cardinality_value->Observe(i % 3);
    }
    
    code_timer->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    code_timer->Stop();
    
    cpu_usage_metric->Evaluate();
    cpu_util_metric->Evaluate();
    http_metric->Evaluate();
    
    manager_->Log();
    
    std::string log_content = ReadLogFile();
    EXPECT_TRUE(log_content.find("IntegrationCounter") != std::string::npos);
    EXPECT_TRUE(log_content.find("CPU Usage") != std::string::npos);
    EXPECT_TRUE(log_content.find("\"CPU\"") != std::string::npos);
    EXPECT_TRUE(log_content.find("HTTPS requests RPS") != std::string::npos);
    EXPECT_TRUE(log_content.find("Percentile Latency") != std::string::npos);
    EXPECT_TRUE(log_content.find("IntegrationAlgorithm") != std::string::npos);
    EXPECT_TRUE(log_content.find("CardinalityType") != std::string::npos);
    EXPECT_TRUE(log_content.find("CardinalityValue") != std::string::npos);
    
    EXPECT_EQ(increment_metric->GetValueAsString(), "0");
    EXPECT_EQ(http_metric->GetValueAsString(), "0.00");
    EXPECT_GE(CountLinesInLog(), 8);
}

TEST_F(MetricsManagerTest, LogFileTimestampFormat) {
    auto* metric = manager_->CreateMetric<Metrics::IncrementMetric>("TimestampTest", 42);
    
    manager_->Log(0);
    
    std::string log_content = ReadLogFile();
    EXPECT_TRUE(log_content.find("TimestampTest") != std::string::npos);
    EXPECT_TRUE(log_content.find("42") != std::string::npos);
    
    std::regex timestamp_regex(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3})");
    EXPECT_TRUE(std::regex_search(log_content, timestamp_regex));
}

TEST_F(MetricsManagerTest, EmptyManagerLogScenario) {
    manager_->Log();
    
    std::string log_content = ReadLogFile();
    EXPECT_TRUE(log_content.empty() || log_content.find_first_not_of(" \t\n\r") == std::string::npos);
}

TEST_F(MetricsManagerTest, SingleMetricMultipleLogging) {
    auto* metric = manager_->CreateMetric<Metrics::IncrementMetric>("MultiLogTest", 0);
    
    ++(*metric);
    ++(*metric);
    manager_->Log(0);
    
    ++(*metric);
    ++(*metric);
    ++(*metric);
    manager_->Log(0);
    
    std::string log_content = ReadLogFile();
    EXPECT_EQ(CountLinesInLog(), 2);
    EXPECT_TRUE(log_content.find("2") != std::string::npos);
    EXPECT_TRUE(log_content.find("3") != std::string::npos);
}