#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <random>
#include <set>

#include "IMetrics/CodeTimeMetric.h"

class CodeTimeMetricTest : public ::testing::Test {
protected:
    void SetUp() override {
        metric = std::make_unique<Metrics::CodeTimeMetric>();
    }

    void TearDown() override {
        metric.reset();
    }

    std::unique_ptr<Metrics::CodeTimeMetric> metric;
    
    struct ParsedTime {
        double value;
        std::string unit;
        bool valid;
    };
    
    ParsedTime parseTimeString(const std::string& str) {
        ParsedTime result{0.0, "", false};
        
        size_t space_pos = str.find(' ');
        if (space_pos == std::string::npos) {
            return result;
        }
        
        try {
            result.value = std::stod(str.substr(0, space_pos));
            result.unit = str.substr(space_pos + 1);
            result.valid = true;
        } catch (...) {
            result.valid = false;
        }
        
        return result;
    }
    
    bool isValidTimeFormat(const std::string& str) {
        ParsedTime parsed = parseTimeString(str);
        if (!parsed.valid) return false;
        
        return (parsed.unit == "ns" || parsed.unit == "μs" || 
                parsed.unit == "ms" || parsed.unit == "s") && 
               parsed.value >= 0.0;
    }
    
    long long timeStringToNanoseconds(const std::string& str) {
        ParsedTime parsed = parseTimeString(str);
        if (!parsed.valid) return -1;
        
        if (parsed.unit == "ns") return static_cast<long long>(parsed.value);
        if (parsed.unit == "μs") return static_cast<long long>(parsed.value * 1000);
        if (parsed.unit == "ms") return static_cast<long long>(parsed.value * 1000000);
        if (parsed.unit == "s") return static_cast<long long>(parsed.value * 1000000000);
        
        return -1;
    }
    
    void performWork(int duration_ms) {
        auto start = std::chrono::high_resolution_clock::now();
        volatile long dummy = 0;
        
        while (std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::high_resolution_clock::now() - start).count() < duration_ms) {
            for (int i = 0; i < 1000; ++i) {
                dummy += i * i;
            }
        }
    }
};

TEST_F(CodeTimeMetricTest, DefaultConstructor) {
    EXPECT_TRUE(metric->GetName().find("Algorithm") != std::string::npos);
    EXPECT_TRUE(metric->GetName().find("\"") != std::string::npos);
}

TEST_F(CodeTimeMetricTest, NamedConstructor) {
    auto named_metric = std::make_unique<Metrics::CodeTimeMetric>("TestTask");
    EXPECT_EQ(named_metric->GetName(), "TestTask");
}

TEST_F(CodeTimeMetricTest, EmptyNameUsesDefault) {
    auto empty_named = std::make_unique<Metrics::CodeTimeMetric>();
    EXPECT_TRUE(empty_named->GetName().find("\"Algorithm") != std::string::npos);
}

TEST_F(CodeTimeMetricTest, InitialTimeValue) {
    std::string initial_value = metric->GetValueAsString();
    EXPECT_TRUE(isValidTimeFormat(initial_value));
    
    long long ns = timeStringToNanoseconds(initial_value);
    EXPECT_GE(ns, 0);
    EXPECT_LT(ns, 1000000);
}

TEST_F(CodeTimeMetricTest, StartStopBasic) {
    metric->Start();
    performWork(10);
    metric->Stop();
    
    std::string result = metric->GetValueAsString();
    EXPECT_TRUE(isValidTimeFormat(result));
    
    long long ns = timeStringToNanoseconds(result);
    EXPECT_GT(ns, 5000000);
    EXPECT_LT(ns, 50000000);
}

TEST_F(CodeTimeMetricTest, MultipleStartStop) {
    metric->Start();
    performWork(5);
    metric->Stop();
    
    std::string first_result = metric->GetValueAsString();
    long long first_ns = timeStringToNanoseconds(first_result);
    
    metric->Start();
    performWork(15);
    metric->Stop();
    
    std::string second_result = metric->GetValueAsString();
    long long second_ns = timeStringToNanoseconds(second_result);
    
    EXPECT_GT(second_ns, first_ns);
    EXPECT_GT(second_ns, 10000000);
}

TEST_F(CodeTimeMetricTest, StopWithoutStart) {
    EXPECT_NO_THROW(metric->Stop());
    
    std::string result = metric->GetValueAsString();
    EXPECT_TRUE(isValidTimeFormat(result));
}

TEST_F(CodeTimeMetricTest, MultipleStopsAfterOneStart) {
    metric->Start();
    performWork(5);
    metric->Stop();
    
    std::string first_result = metric->GetValueAsString();
    
    metric->Stop();
    metric->Stop();
    
    std::string after_multiple_stops = metric->GetValueAsString();
    EXPECT_EQ(first_result, after_multiple_stops);
}

TEST_F(CodeTimeMetricTest, ResetFunctionality) {
    metric->Start();
    performWork(10);
    metric->Stop();
    
    std::string before_reset = metric->GetValueAsString();
    long long before_ns = timeStringToNanoseconds(before_reset);
    
    metric->Reset();
    
    std::string after_reset = metric->GetValueAsString();
    long long after_ns = timeStringToNanoseconds(after_reset);
    
    EXPECT_LT(after_ns, before_ns);
    EXPECT_LT(after_ns, 1000000);
}

TEST_F(CodeTimeMetricTest, EvaluateMethod) {
    EXPECT_NO_THROW(metric->Evaluate());
    
    metric->Start();
    performWork(5);
    metric->Evaluate();
    metric->Stop();
    
    std::string result = metric->GetValueAsString();
    EXPECT_TRUE(isValidTimeFormat(result));
}

TEST_F(CodeTimeMetricTest, NanosecondFormatting) {
    metric->Start();
    volatile int dummy = 0;
    for (int i = 0; i < 10; ++i) dummy += i;
    metric->Stop();
    
    std::string result = metric->GetValueAsString();
    EXPECT_TRUE(isValidTimeFormat(result));
    
    ParsedTime parsed = parseTimeString(result);
    EXPECT_TRUE(parsed.unit == "ns" || parsed.unit == "μs");
}

TEST_F(CodeTimeMetricTest, MillisecondFormatting) {
    metric->Start();
    performWork(50);
    metric->Stop();
    
    std::string result = metric->GetValueAsString();
    EXPECT_TRUE(isValidTimeFormat(result));
    
    ParsedTime parsed = parseTimeString(result);
    EXPECT_EQ(parsed.unit, "ms");
    EXPECT_GT(parsed.value, 30.0);
    EXPECT_LT(parsed.value, 200.0);
}

TEST_F(CodeTimeMetricTest, SecondFormatting) {
    metric->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    metric->Stop();
    
    std::string result = metric->GetValueAsString();
    EXPECT_TRUE(isValidTimeFormat(result));
    
    ParsedTime parsed = parseTimeString(result);
    EXPECT_EQ(parsed.unit, "s");
    EXPECT_GT(parsed.value, 1.0);
    EXPECT_LT(parsed.value, 2.0);
}

TEST_F(CodeTimeMetricTest, ConcurrentStartStop) {
    const int num_threads = 8;
    const int operations_per_thread = 20;
    std::vector<std::thread> threads;
    std::atomic<int> successful_ops{0};
    std::atomic<bool> should_stop{false};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < operations_per_thread && !should_stop; ++j) {
                try {
                    metric->Start();
                    performWork(1 + (j % 5));
                    metric->Stop();
                    
                    std::string result = metric->GetValueAsString();
                    if (isValidTimeFormat(result)) {
                        successful_ops++;
                    }
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                } catch (...) {
                    FAIL() << "Exception in thread " << i << ", operation " << j;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_GT(successful_ops.load(), (num_threads * operations_per_thread) * 0.9);
}

TEST_F(CodeTimeMetricTest, ConcurrentGetValueAsString) {
    const int num_threads = 10;
    const int reads_per_thread = 50;
    std::vector<std::thread> threads;
    std::vector<std::vector<std::string>> results(num_threads);
    std::atomic<bool> should_stop{false};
    
    metric->Start();
    
    std::thread work_thread([&]() {
        while (!should_stop) {
            performWork(5);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < reads_per_thread; ++j) {
                try {
                    std::string value = metric->GetValueAsString();
                    EXPECT_TRUE(isValidTimeFormat(value));
                    results[i].push_back(value);
                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                } catch (...) {
                    FAIL() << "GetValueAsString threw exception in thread " << i;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    metric->Stop();
    should_stop = true;
    work_thread.join();
    
    for (int i = 0; i < num_threads; ++i) {
        EXPECT_EQ(results[i].size(), reads_per_thread);
        for (const auto& value : results[i]) {
            EXPECT_TRUE(isValidTimeFormat(value));
        }
    }
}

TEST_F(CodeTimeMetricTest, ConcurrentMixedOperations) {
    const int num_threads = 6;
    const int operations_per_thread = 30;
    std::vector<std::thread> threads;
    std::atomic<int> total_operations{0};
    std::atomic<bool> should_stop{false};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> op_dist(0, 4);
            
            for (int j = 0; j < operations_per_thread && !should_stop; ++j) {
                try {
                    int operation = op_dist(gen);
                    switch (operation) {
                        case 0:
                            metric->Start();
                            break;
                        case 1:
                            metric->Stop();
                            break;
                        case 2: {
                            std::string value = metric->GetValueAsString();
                            EXPECT_TRUE(isValidTimeFormat(value));
                            break;
                        }
                        case 3:
                            metric->Reset();
                            break;
                        case 4:
                            metric->Evaluate();
                            break;
                    }
                    total_operations++;
                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                } catch (...) {
                    FAIL() << "Mixed operations threw exception in thread " << i;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(total_operations.load(), num_threads * operations_per_thread);
    
    EXPECT_NO_THROW(metric->Start());
    EXPECT_NO_THROW(metric->Stop());
    std::string final_value = metric->GetValueAsString();
    EXPECT_TRUE(isValidTimeFormat(final_value));
}

TEST_F(CodeTimeMetricTest, UniqueDefaultNames) {
    const int num_metrics = 20;
    std::vector<std::unique_ptr<Metrics::CodeTimeMetric>> metrics;
    std::set<std::string> names;
    
    for (int i = 0; i < num_metrics; ++i) {
        metrics.push_back(std::make_unique<Metrics::CodeTimeMetric>());
        names.insert(metrics.back()->GetName());
    }
    
    EXPECT_EQ(names.size(), num_metrics);
    
    for (const auto& name : names) {
        EXPECT_TRUE(name.find("Algorithm") != std::string::npos);
    }
}

TEST_F(CodeTimeMetricTest, ConcurrentNameGeneration) {
    const int num_threads = 10;
    const int metrics_per_thread = 10;
    std::vector<std::thread> threads;
    std::vector<std::vector<std::string>> thread_names(num_threads);
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < metrics_per_thread; ++j) {
                auto metric = std::make_unique<Metrics::CodeTimeMetric>();
                thread_names[i].push_back(metric->GetName());
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::set<std::string> all_names;
    for (int i = 0; i < num_threads; ++i) {
        for (const auto& name : thread_names[i]) {
            all_names.insert(name);
        }
    }
    
    int total_expected = num_threads * metrics_per_thread;
    EXPECT_EQ(all_names.size(), total_expected);
}

TEST_F(CodeTimeMetricTest, VeryShortTiming) {
    metric->Start();
    volatile int x = 1;
    x += 1;
    metric->Stop();
    
    std::string result = metric->GetValueAsString();
    EXPECT_TRUE(isValidTimeFormat(result));
    
    long long ns = timeStringToNanoseconds(result);
    EXPECT_GE(ns, 0);
    EXPECT_LT(ns, 1000000);
}

TEST_F(CodeTimeMetricTest, RepeatedStartWithoutStop) {
    metric->Start();
    performWork(5);
    
    metric->Start();
    performWork(10);
    metric->Stop();
    
    std::string result = metric->GetValueAsString();
    long long ns = timeStringToNanoseconds(result);
    
    EXPECT_GT(ns, 8000000);
    EXPECT_LT(ns, 30000000);
}

TEST_F(CodeTimeMetricTest, PrecisionConsistency) {
    std::vector<long long> measurements;
    
    for (int i = 0; i < 10; ++i) {
        metric->Start();
        performWork(20);
        metric->Stop();
        
        std::string result = metric->GetValueAsString();
        long long ns = timeStringToNanoseconds(result);
        measurements.push_back(ns);
        
        metric->Reset();
    }
    
    for (long long ns : measurements) {
        EXPECT_GT(ns, 15000000);
        EXPECT_LT(ns, 50000000);
    }
    
    double mean = 0.0;
    for (long long ns : measurements) {
        mean += ns;
    }
    mean /= measurements.size();
    
    double variance = 0.0;
    for (long long ns : measurements) {
        variance += (ns - mean) * (ns - mean);
    }
    variance /= measurements.size();
    double std_dev = std::sqrt(variance);
    
    EXPECT_LT(std_dev, mean * 0.5);
}

class CodeTimeMetricStressTest : public ::testing::Test {
protected:
    void SetUp() override {
        metric = std::make_unique<Metrics::CodeTimeMetric>("StressTest");
    }
    
    std::unique_ptr<Metrics::CodeTimeMetric> metric;
};