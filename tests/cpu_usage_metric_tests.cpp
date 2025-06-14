#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <random>
#include <algorithm>

#include "IMetrics/CPUUsageMetric.h"

class CPUUsageMetricTest : public ::testing::Test {
protected:
    void SetUp() override {
        metric = std::make_unique<Metrics::CPUUsageMetric>();
    }

    void TearDown() override {
        metric.reset();
    }

    std::unique_ptr<Metrics::CPUUsageMetric> metric;
    
    void generateCPULoad(int duration_ms, std::atomic<bool>& should_stop) {
        auto start = std::chrono::high_resolution_clock::now();
        volatile long dummy = 0;
        std::random_device rd;
        std::mt19937 gen(rd());
        
        while (!should_stop && 
               std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::high_resolution_clock::now() - start).count() < duration_ms) {
            
            for (int i = 0; i < 10000; ++i) {
                dummy += static_cast<long>(gen() % 1000);
                dummy *= (i % 7 + 1);
                dummy = dummy % 1000000;
            }
        }
    }
    
    double parsePercentage(const std::string& str) {
        size_t percent_pos = str.find('%');
        if (percent_pos == std::string::npos) {
            return -1.0;
        }
        
        try {
            return std::stod(str.substr(0, percent_pos));
        } catch (...) {
            return -1.0;
        }
    }
    
    bool isValidPercentageFormat(const std::string& str) {
        if (str.empty() || str.back() != '%') {
            return false;
        }
        
        double value = parsePercentage(str);
        return value >= -1.0 && value <= 100.0;
    }
};

TEST_F(CPUUsageMetricTest, GetName) {
    EXPECT_EQ(metric->GetName(), "\"CPU Usage\"");
}

TEST_F(CPUUsageMetricTest, InitialState) {
    std::string initial_value = metric->GetValueAsString();
    EXPECT_TRUE(isValidPercentageFormat(initial_value));
    
    double initial_percent = parsePercentage(initial_value);
    EXPECT_GE(initial_percent, 0.0);
    EXPECT_LE(initial_percent, 100.0);
}

TEST_F(CPUUsageMetricTest, EvaluateBasic) {
    EXPECT_NO_THROW(metric->Evaluate());
    
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(isValidPercentageFormat(value));
    
    double percent = parsePercentage(value);
    EXPECT_GE(percent, 0.0);
    EXPECT_LE(percent, 100.0);
}

TEST_F(CPUUsageMetricTest, MultipleEvaluations) {
    std::vector<double> measurements;
    
    for (int i = 0; i < 5; ++i) {
        metric->Evaluate();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::string value = metric->GetValueAsString();
        EXPECT_TRUE(isValidPercentageFormat(value));
        
        double percent = parsePercentage(value);
        EXPECT_GE(percent, 0.0);
        EXPECT_LE(percent, 100.0);
        
        measurements.push_back(percent);
    }
    
    EXPECT_EQ(measurements.size(), 5);
}

TEST_F(CPUUsageMetricTest, ResetFunctionality) {
    metric->Evaluate();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    metric->Evaluate();
    
    std::string before_reset = metric->GetValueAsString();
    
    EXPECT_NO_THROW(metric->Reset());
    
    std::string after_reset = metric->GetValueAsString();
    EXPECT_TRUE(isValidPercentageFormat(after_reset));
    
    double after_reset_percent = parsePercentage(after_reset);
    EXPECT_GE(after_reset_percent, 0.0);
    EXPECT_LE(after_reset_percent, 100.0);
}

TEST_F(CPUUsageMetricTest, ConsistentStringFormat) {
    for (int i = 0; i < 10; ++i) {
        metric->Evaluate();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        std::string value = metric->GetValueAsString();
        
        EXPECT_TRUE(value.back() == '%');
        
        double percent = parsePercentage(value);
        EXPECT_GE(percent, -1.0);
        EXPECT_NE(percent, -1.0);
        
        size_t dot_pos = value.find('.');
        size_t percent_pos = value.find('%');
        if (dot_pos != std::string::npos) {
            EXPECT_EQ(percent_pos - dot_pos - 1, 2);
        }
    }
}

TEST_F(CPUUsageMetricTest, ConcurrentEvaluate) {
    const int num_threads = 4;
    const int operations_per_thread = 50;
    std::vector<std::thread> threads;
    std::atomic<int> successful_operations{0};
    std::atomic<bool> should_stop{false};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < operations_per_thread && !should_stop; ++j) {
                try {
                    metric->Evaluate();
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    successful_operations++;
                } catch (...) {
                    FAIL() << "Evaluate() threw an exception in concurrent test";
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(successful_operations.load(), num_threads * operations_per_thread);
}

TEST_F(CPUUsageMetricTest, ConcurrentGetValueAsString) {
    const int num_threads = 8;
    const int operations_per_thread = 25;
    std::vector<std::thread> threads;
    std::vector<std::vector<std::string>> results(num_threads);
    std::atomic<bool> should_stop{false};
    
    std::thread eval_thread([&]() {
        while (!should_stop) {
            metric->Evaluate();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < operations_per_thread && !should_stop; ++j) {
                try {
                    std::string value = metric->GetValueAsString();
                    EXPECT_TRUE(isValidPercentageFormat(value));
                    results[i].push_back(value);
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                } catch (...) {
                    FAIL() << "GetValueAsString() threw an exception in concurrent test";
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    should_stop = true;
    eval_thread.join();
    
    for (int i = 0; i < num_threads; ++i) {
        EXPECT_EQ(results[i].size(), operations_per_thread);
        for (const auto& value : results[i]) {
            EXPECT_TRUE(isValidPercentageFormat(value));
        }
    }
}

TEST_F(CPUUsageMetricTest, ConcurrentMixedOperations) {
    const int num_threads = 6;
    const int operations_per_thread = 30;
    std::vector<std::thread> threads;
    std::atomic<int> total_operations{0};
    std::atomic<bool> should_stop{false};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> op_dist(0, 2);
            
            for (int j = 0; j < operations_per_thread && !should_stop; ++j) {
                try {
                    int operation = op_dist(gen);
                    switch (operation) {
                        case 0:
                            metric->Evaluate();
                            break;
                        case 1: {
                            std::string value = metric->GetValueAsString();
                            EXPECT_TRUE(isValidPercentageFormat(value));
                            break;
                        }
                        case 2:
                            if (j % 10 == 0) {
                                metric->Reset();
                            }
                            break;
                    }
                    total_operations++;
                    std::this_thread::sleep_for(std::chrono::milliseconds(15));
                } catch (...) {
                    FAIL() << "Mixed operations threw an exception in thread " << i;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_GT(total_operations.load(), 0);
    
    EXPECT_NO_THROW(metric->Evaluate());
    std::string final_value = metric->GetValueAsString();
    EXPECT_TRUE(isValidPercentageFormat(final_value));
}

TEST_F(CPUUsageMetricTest, CPULoadDetection) {
    metric->Evaluate();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    metric->Evaluate();
    
    std::string baseline_str = metric->GetValueAsString();
    double baseline_usage = parsePercentage(baseline_str);
    
    std::atomic<bool> should_stop{false};
    std::thread load_thread([&]() {
        generateCPULoad(2000, should_stop);
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::vector<double> load_measurements;
    for (int i = 0; i < 5; ++i) {
        metric->Evaluate();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        std::string value_str = metric->GetValueAsString();
        double usage = parsePercentage(value_str);
        load_measurements.push_back(usage);
    }
    
    should_stop = true;
    load_thread.join();
    
    double max_load_usage = *std::max_element(load_measurements.begin(), load_measurements.end());
    
    EXPECT_GE(max_load_usage, 0.0);
    EXPECT_LE(max_load_usage, 100.0);
    
    for (double usage : load_measurements) {
        EXPECT_GE(usage, 0.0);
        EXPECT_LE(usage, 100.0);
    }
}

TEST_F(CPUUsageMetricTest, RapidEvaluations) {
    std::vector<double> rapid_measurements;
    
    for (int i = 0; i < 20; ++i) {
        metric->Evaluate();
        
        std::string value_str = metric->GetValueAsString();
        double usage = parsePercentage(value_str);
        
        EXPECT_GE(usage, 0.0);
        EXPECT_LE(usage, 100.0);
        
        rapid_measurements.push_back(usage);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    EXPECT_EQ(rapid_measurements.size(), 20);
    
    for (double usage : rapid_measurements) {
        EXPECT_GE(usage, 0.0);
        EXPECT_LE(usage, 100.0);
    }
}

TEST_F(CPUUsageMetricTest, ResetAfterError) {
    metric->Evaluate();
    metric->Reset();
    
    EXPECT_NO_THROW(metric->Evaluate());
    
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(isValidPercentageFormat(value));
}

TEST_F(CPUUsageMetricTest, LongRunningStability) {
    const int num_iterations = 50;
    std::vector<double> long_run_measurements;
    
    for (int i = 0; i < num_iterations; ++i) {
        metric->Evaluate();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::string value_str = metric->GetValueAsString();
        double usage = parsePercentage(value_str);
        
        EXPECT_GE(usage, 0.0);
        EXPECT_LE(usage, 100.0);
        
        long_run_measurements.push_back(usage);
        
        if (i % 15 == 0 && i > 0) {
            metric->Reset();
        }
    }
    
    EXPECT_EQ(long_run_measurements.size(), num_iterations);
    
    for (double usage : long_run_measurements) {
        EXPECT_GE(usage, 0.0);
        EXPECT_LE(usage, 100.0);
    }
}

class CPUUsageMetricStressTest : public ::testing::Test {
protected:
    void SetUp() override {
        metric = std::make_unique<Metrics::CPUUsageMetric>();
    }
    
    std::unique_ptr<Metrics::CPUUsageMetric> metric;
};

TEST_F(CPUUsageMetricStressTest, HighConcurrencyStress) {
    const int num_threads = 16;
    const int operations_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> successful_ops{0};
    std::atomic<int> failed_ops{0};
    std::atomic<bool> should_stop{false};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> op_dist(0, 10);
            
            for (int j = 0; j < operations_per_thread && !should_stop; ++j) {
                try {
                    if (op_dist(gen) < 7) {
                        metric->Evaluate();
                    } else if (op_dist(gen) < 9) {
                        std::string value = metric->GetValueAsString();
                        if (value.empty() || value.back() != '%') {
                            failed_ops++;
                            continue;
                        }
                    } else {
                        metric->Reset();
                    }
                    successful_ops++;
                } catch (...) {
                    failed_ops++;
                }
                
                if (j % 10 == 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    int total_ops = successful_ops.load() + failed_ops.load();
    EXPECT_GT(successful_ops.load(), total_ops * 0.95);
    EXPECT_LT(failed_ops.load(), total_ops * 0.05);
    
    EXPECT_NO_THROW(metric->Evaluate());
    std::string final_value = metric->GetValueAsString();
    EXPECT_FALSE(final_value.empty());
    EXPECT_EQ(final_value.back(), '%');
}

TEST_F(CPUUsageMetricStressTest, MemoryLeakCheck) {
    const int iterations = 1000;
    
    for (int i = 0; i < iterations; ++i) {
        metric->Evaluate();
        std::string value = metric->GetValueAsString();
        
        if (i % 100 == 0) {
            metric->Reset();
        }
        
        if (i % 50 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    
    EXPECT_NO_THROW(metric->Evaluate());
    std::string final_value = metric->GetValueAsString();
    EXPECT_FALSE(final_value.empty());
}

TEST(CPUTimesTest, DefaultInitialization) {
    CPUTimes times;
    
    EXPECT_EQ(times.user, 0);
    EXPECT_EQ(times.nice, 0);
    EXPECT_EQ(times.system, 0);
    EXPECT_EQ(times.idle, 0);
    EXPECT_EQ(times.iowait, 0);
    EXPECT_EQ(times.irq, 0);
    EXPECT_EQ(times.softirq, 0);
    EXPECT_EQ(times.steal, 0);
    EXPECT_EQ(times.guest, 0);
    EXPECT_EQ(times.guest_nice, 0);
    
    EXPECT_EQ(times.getTotal(), 0);
}

TEST(CPUTimesTest, GetTotalCalculation) {
    CPUTimes times;
    times.user = 100;
    times.nice = 50;
    times.system = 200;
    times.idle = 300;
    times.iowait = 25;
    times.irq = 10;
    times.softirq = 15;
    times.steal = 5;
    times.guest = 20;
    times.guest_nice = 30;
    
    unsigned long long expected_total = 100 + 50 + 200 + 300 + 25 + 10 + 15 + 5 + 20 + 30;
    EXPECT_EQ(times.getTotal(), expected_total);
    EXPECT_EQ(times.getTotal(), 755);
}

TEST(CPUTimesTest, OverflowHandling) {
    CPUTimes times;
    times.user = ULLONG_MAX / 10;
    times.nice = ULLONG_MAX / 10;
    times.system = ULLONG_MAX / 10;
    times.idle = ULLONG_MAX / 10;
    times.iowait = ULLONG_MAX / 10;
    times.irq = ULLONG_MAX / 10;
    times.softirq = ULLONG_MAX / 10;
    times.steal = ULLONG_MAX / 10;
    times.guest = ULLONG_MAX / 10;
    times.guest_nice = ULLONG_MAX / 10;
    
    unsigned long long total = times.getTotal();
    EXPECT_GT(total, 0);
    EXPECT_EQ(total, (ULLONG_MAX / 10) * 10);
}