#include <gtest/gtest.h>
#include "MetricsManager/MetricsManager.h"
#include "IMetrics/IncrementMetric.h"
#include "IMetrics/HTTPIncomeMetric.h"
#include "IMetrics/CPUUsageMetric.h"
#include <thread>
#include <chrono>
#include <filesystem>

class SimpleMetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_file_ = "simple_test_" + std::to_string(counter_++) + ".log";
        manager_ = std::make_unique<MetricsManager<>>(test_file_);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    void TearDown() override {
        manager_.reset();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (std::filesystem::exists(test_file_)) {
            std::filesystem::remove(test_file_);
        }
    }

    std::unique_ptr<MetricsManager<>> manager_;
    std::string test_file_;
    static inline std::atomic<int> counter_{0};
};

TEST_F(SimpleMetricsTest, CreateIncrementMetric) {
    auto* metric = manager_->CreateMetric<Metrics::IncrementMetric>("TestCounter", 42);
    ASSERT_NE(metric, nullptr);
    EXPECT_EQ(metric->GetValueAsString(), "42");
    EXPECT_EQ(metric->GetName(), "TestCounter");
}

TEST_F(SimpleMetricsTest, IncrementAndReset) {
    auto* metric = manager_->CreateMetric<Metrics::IncrementMetric>("Counter", 10);
    ++(*metric);
    ++(*metric);
    EXPECT_EQ(metric->GetValueAsString(), "12");
    
    manager_->Log(0);
    EXPECT_EQ(metric->GetValueAsString(), "0");
}

TEST_F(SimpleMetricsTest, HTTPMetricBasics) {
    auto* metric = manager_->CreateMetric<Metrics::HTTPIncomeMetric>(0);
    ASSERT_NE(metric, nullptr);
    
    ++(*metric);
    ++(*metric);
    ++(*metric);
    
    metric->Evaluate();
    EXPECT_EQ(metric->GetValueAsString(), "3.00");
    
    manager_->Log(0);
    EXPECT_EQ(metric->GetValueAsString(), "0.00");
}

TEST_F(SimpleMetricsTest, CPUMetricCreation) {
    auto* metric = manager_->CreateMetric<Metrics::CPUUsageMetric>();
    ASSERT_NE(metric, nullptr);
    EXPECT_EQ(metric->GetName(), "\"CPU Usage\"");
    
    metric->Evaluate();
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("%") != std::string::npos);
}

TEST_F(SimpleMetricsTest, GetMetricByIndex) {
    manager_->CreateMetric<Metrics::IncrementMetric>("First", 1);
    manager_->CreateMetric<Metrics::IncrementMetric>("Second", 2);
    
    auto* first = manager_->GetMetric<Metrics::IncrementMetric>(0);
    auto* second = manager_->GetMetric<Metrics::IncrementMetric>(1);
    
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(first->GetValueAsString(), "1");
    EXPECT_EQ(second->GetValueAsString(), "2");
}

TEST_F(SimpleMetricsTest, LogAllMetrics) {
    manager_->CreateMetric<Metrics::IncrementMetric>("Counter1", 5);
    manager_->CreateMetric<Metrics::IncrementMetric>("Counter2", 10);
    
    EXPECT_NO_THROW(manager_->Log());
    
    auto* metric1 = manager_->GetMetric<Metrics::IncrementMetric>(0);
    auto* metric2 = manager_->GetMetric<Metrics::IncrementMetric>(1);
    
    EXPECT_EQ(metric1->GetValueAsString(), "0");
    EXPECT_EQ(metric2->GetValueAsString(), "0");
}

TEST_F(SimpleMetricsTest, ErrorHandling) {
    manager_->CreateMetric<Metrics::IncrementMetric>("Test", 1);
    
    EXPECT_THROW(manager_->GetMetric<Metrics::IncrementMetric>(1), std::out_of_range);
    EXPECT_THROW(manager_->GetMetric<Metrics::HTTPIncomeMetric>(0), std::runtime_error);
    EXPECT_THROW(manager_->Log(1), std::out_of_range);
}

TEST_F(SimpleMetricsTest, ConcurrentAccess) {
    auto* counter = manager_->CreateMetric<Metrics::IncrementMetric>("SharedCounter", 0);
    
    std::vector<std::thread> threads;
    const int num_threads = 5;
    const int increments_per_thread = 10;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < increments_per_thread; ++j) {
                ++(*counter);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    int final_value = std::stoi(counter->GetValueAsString());
    EXPECT_EQ(final_value, num_threads * increments_per_thread);
}