#include "IMetrics/HTTPIncomeMetric.h"

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <memory>
#include <sstream>
#include <iomanip>
#include <vector>
#include <random>
#include <atomic>

using namespace Metrics;

class HTTPIncomeMetricTest : public ::testing::Test {
protected:
    void SetUp() override {
        metric = std::make_unique<HTTPIncomeMetric>();
    }

    void TearDown() override {
        metric->Reset();
    }

    std::unique_ptr<HTTPIncomeMetric> metric;

    bool isValidDoubleString(const std::string& str) {
        try {
            double value = std::stod(str);
            return value >= 0.0;
        } catch (...) {
            return false;
        }
    }

    bool hasCorrectPrecision(const std::string& str) {
        size_t dot_pos = str.find('.');
        if (dot_pos == std::string::npos) {
            return false;
        }
        std::string decimal_part = str.substr(dot_pos + 1);
        return decimal_part.length() == 2;
    }

    void simulateHttpRequests(HTTPIncomeMetric& metric, int request_count, int delay_ms = 0) {
        for (int i = 0; i < request_count; ++i) {
            ++metric;
            if (delay_ms > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            }
        }
    }

    void simulateHttpRequestsBurst(HTTPIncomeMetric& metric, int burst_count, int requests_per_burst, int burst_interval_ms) {
        for (int i = 0; i < burst_count; ++i) {
            for (int j = 0; j < requests_per_burst; ++j) {
                ++metric;
            }
            if (i < burst_count - 1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(burst_interval_ms));
            }
        }
    }
};

TEST_F(HTTPIncomeMetricTest, ConstructorDoesNotThrow) {
    EXPECT_NO_THROW({
        HTTPIncomeMetric test_metric;
    });
}

TEST_F(HTTPIncomeMetricTest, ConstructorWithStartValueDoesNotThrow) {
    EXPECT_NO_THROW({
        HTTPIncomeMetric test_metric(100);
    });
}

TEST_F(HTTPIncomeMetricTest, ConstructorInitializesValidState) {
    std::string initial_value = metric->GetValueAsString();
    EXPECT_TRUE(isValidDoubleString(initial_value));
    EXPECT_TRUE(hasCorrectPrecision(initial_value));
    EXPECT_EQ("0.00", initial_value);
}

TEST_F(HTTPIncomeMetricTest, ConstructorWithStartValueInitializesCorrectly) {
    HTTPIncomeMetric custom_metric(50);
    std::string initial_value = custom_metric.GetValueAsString();
    EXPECT_TRUE(isValidDoubleString(initial_value));
    EXPECT_TRUE(hasCorrectPrecision(initial_value));
    EXPECT_EQ("0.00", initial_value);
}

TEST_F(HTTPIncomeMetricTest, GetNameReturnsCorrectName) {
    EXPECT_EQ("\"HTTPS requests RPS\"", metric->GetName());
}

TEST_F(HTTPIncomeMetricTest, GetNameIsConsistent) {
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ("\"HTTPS requests RPS\"", metric->GetName());
    }
}

TEST_F(HTTPIncomeMetricTest, GetNameUnchangedAfterOperations) {
    ++(*metric);
    metric->Evaluate();
    EXPECT_EQ("\"HTTPS requests RPS\"", metric->GetName());
    
    metric->Reset();
    EXPECT_EQ("\"HTTPS requests RPS\"", metric->GetName());
}

TEST_F(HTTPIncomeMetricTest, GetValueAsStringReturnsValidDouble) {
    std::string value_str = metric->GetValueAsString();
    EXPECT_TRUE(isValidDoubleString(value_str));
}

TEST_F(HTTPIncomeMetricTest, GetValueAsStringHasCorrectPrecision) {
    std::string value_str = metric->GetValueAsString();
    EXPECT_TRUE(hasCorrectPrecision(value_str));
}

TEST_F(HTTPIncomeMetricTest, GetValueAsStringReturnsNonNegative) {
    std::string value_str = metric->GetValueAsString();
    double value = std::stod(value_str);
    EXPECT_GE(value, 0.0);
}

TEST_F(HTTPIncomeMetricTest, GetValueAsStringMatchesExpectedFormat) {
    ++(*metric);
    metric->Evaluate();
    
    std::string value_str = metric->GetValueAsString();
    double value = std::stod(value_str);
    
    std::ostringstream expected_format;
    expected_format << std::fixed << std::setprecision(2) << value;
    EXPECT_EQ(expected_format.str(), value_str);
}

TEST_F(HTTPIncomeMetricTest, PreIncrementDoesNotThrow) {
    EXPECT_NO_THROW(++(*metric));
}

TEST_F(HTTPIncomeMetricTest, PostIncrementDoesNotThrow) {
    EXPECT_NO_THROW((*metric)++);
}

TEST_F(HTTPIncomeMetricTest, IncrementReturnsReference) {
    HTTPIncomeMetric& ref1 = ++(*metric);
    HTTPIncomeMetric& ref2 = (*metric)++;
    
    EXPECT_EQ(&ref1, metric.get());
    EXPECT_EQ(&ref2, metric.get());
}

TEST_F(HTTPIncomeMetricTest, MultipleIncrementsWork) {
    EXPECT_NO_THROW({
        for (int i = 0; i < 100; ++i) {
            ++(*metric);
        }
    });
}

TEST_F(HTTPIncomeMetricTest, EvaluateDoesNotThrow) {
    EXPECT_NO_THROW(metric->Evaluate());
}

TEST_F(HTTPIncomeMetricTest, EvaluateAfterNoRequestsReturnsZero) {
    metric->Evaluate();
    EXPECT_EQ("0.00", metric->GetValueAsString());
}

TEST_F(HTTPIncomeMetricTest, EvaluateCalculatesCorrectRPS) {
    for (int i = 0; i < 5; ++i) {
        ++(*metric);
    }
    
    metric->Evaluate();
    EXPECT_EQ("5.00", metric->GetValueAsString());
}

TEST_F(HTTPIncomeMetricTest, ConsecutiveEvaluatesWork) {
    for (int i = 0; i < 3; ++i) {
        ++(*metric);
    }
    metric->Evaluate();
    EXPECT_EQ("3.00", metric->GetValueAsString());
    
    for (int i = 0; i < 2; ++i) {
        ++(*metric);
    }
    metric->Evaluate();
    EXPECT_EQ("2.00", metric->GetValueAsString());
}

TEST_F(HTTPIncomeMetricTest, EvaluateCalculatesIncrementalRequests) {
    metric->Evaluate();
    EXPECT_EQ("0.00", metric->GetValueAsString());
    
    for (int i = 0; i < 10; ++i) {
        ++(*metric);
    }
    metric->Evaluate();
    EXPECT_EQ("10.00", metric->GetValueAsString());
    
    for (int i = 0; i < 5; ++i) {
        ++(*metric);
    }
    metric->Evaluate();
    EXPECT_EQ("5.00", metric->GetValueAsString());
    
    metric->Evaluate();
    EXPECT_EQ("0.00", metric->GetValueAsString());
}

TEST_F(HTTPIncomeMetricTest, ResetDoesNotThrow) {
    EXPECT_NO_THROW(metric->Reset());
}

TEST_F(HTTPIncomeMetricTest, ResetSetsValueToZero) {
    for (int i = 0; i < 10; ++i) {
        ++(*metric);
    }
    metric->Evaluate();
    
    metric->Reset();
    EXPECT_EQ("0.00", metric->GetValueAsString());
}

TEST_F(HTTPIncomeMetricTest, ResetClearsInternalCounters) {
    for (int i = 0; i < 15; ++i) {
        ++(*metric);
    }
    metric->Evaluate();
    
    metric->Reset();
    
    metric->Evaluate();
    EXPECT_EQ("0.00", metric->GetValueAsString());
    
    for (int i = 0; i < 3; ++i) {
        ++(*metric);
    }
    metric->Evaluate();
    EXPECT_EQ("3.00", metric->GetValueAsString());
}

TEST_F(HTTPIncomeMetricTest, ConsecutiveResetsWork) {
    for (int i = 0; i < 3; ++i) {
        ++(*metric);
        metric->Evaluate();
        metric->Reset();
        EXPECT_EQ("0.00", metric->GetValueAsString()) << "Reset " << (i + 1) << " failed";
    }
}

TEST_F(HTTPIncomeMetricTest, ConcurrentIncrementsAreThreadSafe) {
    const int num_threads = 4;
    const int requests_per_thread = 1000;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, requests_per_thread]() {
            for (int j = 0; j < requests_per_thread; ++j) {
                ++(*metric);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    metric->Evaluate();
    double total_expected = num_threads * requests_per_thread;
    double actual = std::stod(metric->GetValueAsString());
    
    EXPECT_EQ(total_expected, actual);
}

TEST_F(HTTPIncomeMetricTest, ConcurrentEvaluatesAreThreadSafe) {
    const int num_threads = 8;
    std::vector<std::thread> threads;
    std::atomic<int> completed_evaluations{0};
    
    for (int i = 0; i < 100; ++i) {
        ++(*metric);
    }
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, &completed_evaluations]() {
            for (int j = 0; j < 10; ++j) {
                EXPECT_NO_THROW(metric->Evaluate());
                std::string value = metric->GetValueAsString();
                EXPECT_TRUE(isValidDoubleString(value));
                ++completed_evaluations;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(completed_evaluations.load(), num_threads * 10);
}

TEST_F(HTTPIncomeMetricTest, ConcurrentIncrementsAndEvaluates) {
    const int increment_threads = 3;
    const int evaluate_threads = 2;
    const int requests_per_thread = 500;
    const int evaluations_per_thread = 20;
    
    std::vector<std::thread> threads;
    std::atomic<bool> should_stop{false};
    
    for (int i = 0; i < increment_threads; ++i) {
        threads.emplace_back([this, requests_per_thread, &should_stop]() {
            for (int j = 0; j < requests_per_thread && !should_stop; ++j) {
                ++(*metric);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
    }
    
    for (int i = 0; i < evaluate_threads; ++i) {
        threads.emplace_back([this, evaluations_per_thread, &should_stop]() {
            for (int j = 0; j < evaluations_per_thread && !should_stop; ++j) {
                EXPECT_NO_THROW(metric->Evaluate());
                std::string value = metric->GetValueAsString();
                EXPECT_TRUE(isValidDoubleString(value));
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    should_stop = true;
    
    metric->Evaluate();
    std::string final_value = metric->GetValueAsString();
    EXPECT_TRUE(isValidDoubleString(final_value));
}

TEST_F(HTTPIncomeMetricTest, ConcurrentResetAndOperations) {
    std::vector<std::thread> threads;
    std::atomic<bool> should_stop{false};
    
    threads.emplace_back([this, &should_stop]() {
        for (int i = 0; i < 1000 && !should_stop; ++i) {
            ++(*metric);
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
    });
    
    threads.emplace_back([this, &should_stop]() {
        for (int i = 0; i < 50 && !should_stop; ++i) {
            EXPECT_NO_THROW(metric->Evaluate());
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });
    
    threads.emplace_back([this, &should_stop]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        for (int i = 0; i < 5 && !should_stop; ++i) {
            EXPECT_NO_THROW(metric->Reset());
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    should_stop = true;
    
    for (auto& t : threads) {
        t.join();
    }
    
    metric->Reset();
    ++(*metric);
    metric->Evaluate();
    EXPECT_EQ("1.00", metric->GetValueAsString());
}

TEST_F(HTTPIncomeMetricTest, WorksThroughIMetricPointer) {
    std::unique_ptr<IMetric> metric_ptr = std::make_unique<HTTPIncomeMetric>();
    
    EXPECT_EQ("\"HTTPS requests RPS\"", metric_ptr->GetName());
    
    std::string value = metric_ptr->GetValueAsString();
    EXPECT_TRUE(isValidDoubleString(value));
    
    EXPECT_NO_THROW(metric_ptr->Evaluate());
    EXPECT_NO_THROW(metric_ptr->Reset());
    EXPECT_EQ("0.00", metric_ptr->GetValueAsString());
}

TEST_F(HTTPIncomeMetricTest, HighVolumeIncrementsPerformance) {
    const int num_requests = 100000;
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_requests; ++i) {
        ++(*metric);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 1000) << "100k increments took too long: " << duration.count() << "ms";
    
    metric->Evaluate();
    EXPECT_EQ(std::to_string(num_requests) + ".00", metric->GetValueAsString());
}

TEST_F(HTTPIncomeMetricTest, EvaluateCompletesQuickly) {
    for (int i = 0; i < 1000; ++i) {
        ++(*metric);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    metric->Evaluate();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 10) << "Evaluate() took too long: " << duration.count() << "ms";
}

TEST_F(HTTPIncomeMetricTest, SimulatedHttpRequestsBurst) {
    for (int i = 0; i < 50; ++i) {
        ++(*metric);
    }
    
    metric->Evaluate();
    EXPECT_EQ("50.00", metric->GetValueAsString());
    
    metric->Evaluate();
    EXPECT_EQ("0.00", metric->GetValueAsString());
}

TEST_F(HTTPIncomeMetricTest, SimulatedSteadyTraffic) {
    std::vector<int> request_intervals = {10, 15, 8, 12, 20, 5};
    std::vector<std::string> expected_values = {"10.00", "15.00", "8.00", "12.00", "20.00", "5.00"};
    
    for (size_t i = 0; i < request_intervals.size(); ++i) {
        for (int j = 0; j < request_intervals[i]; ++j) {
            ++(*metric);
        }
        metric->Evaluate();
        EXPECT_EQ(expected_values[i], metric->GetValueAsString()) 
            << "Interval " << i << " failed";
    }
}

TEST_F(HTTPIncomeMetricTest, SimulatedVariableLoad) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> request_dist(1, 100);
    
    std::vector<int> request_counts;
    std::vector<std::string> actual_values;
    
    for (int i = 0; i < 10; ++i) {
        int requests = request_dist(gen);
        request_counts.push_back(requests);
        
        for (int j = 0; j < requests; ++j) {
            ++(*metric);
        }
        
        metric->Evaluate();
        actual_values.push_back(metric->GetValueAsString());
        
        std::ostringstream expected;
        expected << std::fixed << std::setprecision(2) << static_cast<double>(requests);
        EXPECT_EQ(expected.str(), actual_values[i]) << "Iteration " << i 
                  << " with " << requests << " requests failed";
    }
}

TEST_F(HTTPIncomeMetricTest, ZeroRequestsConsistentBehavior) {
    for (int i = 0; i < 5; ++i) {
        metric->Evaluate();
        EXPECT_EQ("0.00", metric->GetValueAsString()) << "Evaluation " << i << " failed";
    }
}

TEST_F(HTTPIncomeMetricTest, LargeNumberOfRequests) {
    const unsigned long long large_number = 1000000;
    
    for (unsigned long long i = 0; i < large_number; ++i) {
        ++(*metric);
    }
    
    metric->Evaluate();
    
    std::ostringstream expected;
    expected << std::fixed << std::setprecision(2) << static_cast<double>(large_number);
    EXPECT_EQ(expected.str(), metric->GetValueAsString());
}

TEST_F(HTTPIncomeMetricTest, ValueStringFormatConsistency) {
    std::vector<int> test_values = {0, 1, 10, 100, 1000};
    
    for (int value : test_values) {
        metric->Reset();
        
        for (int i = 0; i < value; ++i) {
            ++(*metric);
        }
        
        metric->Evaluate();
        std::string result = metric->GetValueAsString();
        
        EXPECT_TRUE(hasCorrectPrecision(result)) << "Value " << value << " has incorrect precision";
        EXPECT_TRUE(isValidDoubleString(result)) << "Value " << value << " is not valid double";
        
        std::ostringstream expected;
        expected << std::fixed << std::setprecision(2) << static_cast<double>(value);
        EXPECT_EQ(expected.str(), result) << "Value " << value << " format mismatch";
    }
}

class HTTPIncomeMetricStressTest : public ::testing::Test {
protected:
    void SetUp() override {
        metric = std::make_unique<HTTPIncomeMetric>();
    }

    std::unique_ptr<HTTPIncomeMetric> metric;
};

TEST_F(HTTPIncomeMetricStressTest, HighFrequencyOperations) {
    const int cycles = 100;
    
    for (int cycle = 0; cycle < cycles; ++cycle) {
        for (int i = 0; i < 1000; ++i) {
            ++(*metric);
        }
        
        metric->Evaluate();
        EXPECT_EQ("1000.00", metric->GetValueAsString());
        
        metric->Reset();
        EXPECT_EQ("0.00", metric->GetValueAsString());
    }
    
    ++(*metric);
    metric->Evaluate();
    EXPECT_EQ("1.00", metric->GetValueAsString());
}

TEST_F(HTTPIncomeMetricStressTest, ConcurrentStressTest) {
    const int num_threads = 10;
    const int operations_per_thread = 1000;
    std::vector<std::thread> threads;
    std::atomic<int> total_operations{0};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, operations_per_thread, &total_operations]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                ++(*metric);
                ++total_operations;
                
                if (j % 100 == 0) {
                    metric->Evaluate();
                }
                
                if (j % 200 == 0) {
                    std::string value = metric->GetValueAsString();
                    EXPECT_TRUE(std::stod(value) >= 0.0);
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(total_operations.load(), num_threads * operations_per_thread);
}

TEST_F(HTTPIncomeMetricTest, ConstructorWithMaxValue) {
    HTTPIncomeMetric max_metric(ULLONG_MAX);
    EXPECT_EQ("0.00", max_metric.GetValueAsString());
    EXPECT_NO_THROW(max_metric.Evaluate());
}

TEST_F(HTTPIncomeMetricTest, EvaluateAfterReset) {
    for (int i = 0; i < 10; ++i) {
        ++(*metric);
    }
    metric->Evaluate();
    EXPECT_EQ("10.00", metric->GetValueAsString());
    
    metric->Reset();
    metric->Evaluate();
    EXPECT_EQ("0.00", metric->GetValueAsString());
}

TEST_F(HTTPIncomeMetricTest, MultipleEvaluationsWithoutIncrements) {
    metric->Evaluate();
    EXPECT_EQ("0.00", metric->GetValueAsString());
    
    for (int i = 0; i < 5; ++i) {
        metric->Evaluate();
        EXPECT_EQ("0.00", metric->GetValueAsString()) << "Evaluation " << i << " failed";
    }
}

TEST_F(HTTPIncomeMetricTest, IncrementBetweenEvaluations) {
    metric->Evaluate();
    EXPECT_EQ("0.00", metric->GetValueAsString());
    
    ++(*metric);
    metric->Evaluate();
    EXPECT_EQ("1.00", metric->GetValueAsString());
    
    ++(*metric);
    metric->Evaluate();
    EXPECT_EQ("1.00", metric->GetValueAsString());
    
    metric->Evaluate();
    EXPECT_EQ("0.00", metric->GetValueAsString());
}

TEST_F(HTTPIncomeMetricTest, ChainedIncrements) {
    HTTPIncomeMetric& ref1 = ++(*metric);
    HTTPIncomeMetric& ref2 = ++ref1;
    HTTPIncomeMetric& ref3 = ++ref2;
    
    EXPECT_EQ(&ref1, metric.get());
    EXPECT_EQ(&ref2, metric.get());
    EXPECT_EQ(&ref3, metric.get());
    
    metric->Evaluate();
    EXPECT_EQ("3.00", metric->GetValueAsString());
}

TEST_F(HTTPIncomeMetricTest, MixedIncrementTypes) {
    ++(*metric);
    (*metric)++;
    ++(*metric);
    (*metric)++;
    
    metric->Evaluate();
    EXPECT_EQ("4.00", metric->GetValueAsString());
}

TEST_F(HTTPIncomeMetricTest, MemoryStability) {
    for (int cycle = 0; cycle < 100; ++cycle) {
        for (int i = 0; i < 1000; ++i) {
            ++(*metric);
        }
        metric->Evaluate();
        EXPECT_EQ("1000.00", metric->GetValueAsString());
        metric->Reset();
    }
    
    ++(*metric);
    metric->Evaluate();
    EXPECT_EQ("1.00", metric->GetValueAsString());
}

TEST_F(HTTPIncomeMetricTest, AtomicOperationsConsistency) {
    const int num_operations = 10000;
    
    for (int i = 0; i < num_operations; ++i) {
        ++(*metric);
    }
    
    metric->Evaluate();
    EXPECT_EQ(std::to_string(num_operations) + ".00", metric->GetValueAsString());
    
    metric->Evaluate();
    EXPECT_EQ("0.00", metric->GetValueAsString());
}

TEST_F(HTTPIncomeMetricTest, StateConsistencyAfterOperations) {
    EXPECT_EQ("0.00", metric->GetValueAsString());
    EXPECT_EQ("\"HTTPS requests RPS\"", metric->GetName());
    
    ++(*metric);
    EXPECT_EQ("\"HTTPS requests RPS\"", metric->GetName());
    
    metric->Evaluate();
    EXPECT_EQ("\"HTTPS requests RPS\"", metric->GetName());
    EXPECT_NE("0.00", metric->GetValueAsString());
    
    metric->Reset();
    EXPECT_EQ("\"HTTPS requests RPS\"", metric->GetName());
    EXPECT_EQ("0.00", metric->GetValueAsString());
}

TEST_F(HTTPIncomeMetricTest, ConcurrentReadOperations) {
    const int num_threads = 8;
    std::vector<std::thread> threads;
    std::atomic<int> successful_reads{0};
    
    for (int i = 0; i < 50; ++i) {
        ++(*metric);
    }
    metric->Evaluate();
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, &successful_reads]() {
            for (int j = 0; j < 100; ++j) {
                std::string name = metric->GetName();
                std::string value = metric->GetValueAsString();
                
                if (name == "\"HTTPS requests RPS\"" && 
                    isValidDoubleString(value) && 
                    hasCorrectPrecision(value)) {
                    ++successful_reads;
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(successful_reads.load(), num_threads * 100);
}

TEST_F(HTTPIncomeMetricTest, ExtremeValueHandling) {
    HTTPIncomeMetric large_metric(ULLONG_MAX - 1000);
    
    for (int i = 0; i < 100; ++i) {
        EXPECT_NO_THROW(++large_metric);
    }
    
    EXPECT_NO_THROW(large_metric.Evaluate());
    std::string value = large_metric.GetValueAsString();
    EXPECT_TRUE(isValidDoubleString(value));
    EXPECT_TRUE(hasCorrectPrecision(value));
}

TEST_F(HTTPIncomeMetricTest, RapidResetOperations) {
    for (int i = 0; i < 1000; ++i) {
        ++(*metric);
        metric->Evaluate();
        metric->Reset();
        EXPECT_EQ("0.00", metric->GetValueAsString());
    }
}

TEST_F(HTTPIncomeMetricTest, ComplexWorkloadSimulation) {
    std::vector<int> traffic_pattern = {
        0, 10, 50, 100, 200, 150, 75, 25, 5, 0,
        100, 100, 100, 100, 100,
        500, 1000, 500, 100, 0
    };
    
    for (size_t phase = 0; phase < traffic_pattern.size(); ++phase) {
        int requests = traffic_pattern[phase];
        
        for (int i = 0; i < requests; ++i) {
            ++(*metric);
        }
        
        metric->Evaluate();
        std::string expected = std::to_string(requests) + ".00";
        EXPECT_EQ(expected, metric->GetValueAsString()) 
            << "Phase " << phase << " with " << requests << " requests failed";
    }
}

TEST_F(HTTPIncomeMetricTest, InterleavedOperations) {
    for (int cycle = 0; cycle < 10; ++cycle) {
        for (int i = 0; i < 5; ++i) {
            ++(*metric);
        }
        
        metric->Evaluate();
        EXPECT_EQ("5.00", metric->GetValueAsString());
        
        for (int i = 0; i < 3; ++i) {
            ++(*metric);
        }
        
        metric->Evaluate();
        EXPECT_EQ("3.00", metric->GetValueAsString());
        
        if (cycle % 3 == 0) {
            metric->Reset();
            EXPECT_EQ("0.00", metric->GetValueAsString());
        }
    }
}

TEST_F(HTTPIncomeMetricTest, AtomicIncrementConsistency) {
    const int num_threads = 16;
    const int increments_per_thread = 1000;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, increments_per_thread]() {
            for (int j = 0; j < increments_per_thread; ++j) {
                ++(*metric);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    metric->Evaluate();
    int expected_total = num_threads * increments_per_thread;
    EXPECT_EQ(std::to_string(expected_total) + ".00", metric->GetValueAsString());
}

TEST_F(HTTPIncomeMetricTest, ConcurrentEvaluateAndIncrement) {
    std::atomic<bool> start_flag{false};
    std::atomic<int> evaluation_count{0};
    std::vector<std::thread> threads;
    
    threads.emplace_back([this, &start_flag]() {
        while (!start_flag) {
            std::this_thread::yield();
        }
        for (int i = 0; i < 10000; ++i) {
            ++(*metric);
        }
    });
    
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([this, &start_flag, &evaluation_count]() {
            while (!start_flag) {
                std::this_thread::yield();
            }
            for (int j = 0; j < 100; ++j) {
                metric->Evaluate();
                ++evaluation_count;
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
    }
    
    start_flag = true;
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(evaluation_count.load(), 400);
    
    metric->Evaluate();
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(isValidDoubleString(value));
}

TEST_F(HTTPIncomeMetricTest, OverflowHandling) {
    HTTPIncomeMetric overflow_metric(ULLONG_MAX - 10);
    
    for (int i = 0; i < 5; ++i) {
        EXPECT_NO_THROW(++overflow_metric);
    }
    
    overflow_metric.Evaluate();
    std::string value = overflow_metric.GetValueAsString();
    EXPECT_TRUE(isValidDoubleString(value));
    EXPECT_GE(std::stod(value), 0.0);
}

TEST_F(HTTPIncomeMetricTest, ZeroToNonZeroTransition) {
    for (int i = 0; i < 10; ++i) {
        metric->Evaluate();
        EXPECT_EQ("0.00", metric->GetValueAsString());
    }
    
    ++(*metric);
    metric->Evaluate();
    EXPECT_EQ("1.00", metric->GetValueAsString());
    
    metric->Evaluate();
    EXPECT_EQ("0.00", metric->GetValueAsString());
}

TEST_F(HTTPIncomeMetricTest, DecimalPrecisionEdgeCases) {
    std::vector<int> test_values = {1, 9, 10, 99, 100, 999, 1000, 9999, 10000};
    
    for (int value : test_values) {
        metric->Reset();
        
        for (int i = 0; i < value; ++i) {
            ++(*metric);
        }
        
        metric->Evaluate();
        std::string result = metric->GetValueAsString();
        
        EXPECT_TRUE(hasCorrectPrecision(result)) << "Value " << value << " precision test failed";
        
        std::ostringstream expected;
        expected << std::fixed << std::setprecision(2) << static_cast<double>(value);
        EXPECT_EQ(expected.str(), result) << "Value " << value << " format test failed";
    }
}

TEST_F(HTTPIncomeMetricTest, MultipleInstancesIndependence) {
    HTTPIncomeMetric metric1;
    HTTPIncomeMetric metric2(100);
    HTTPIncomeMetric metric3(50);
    
    ++metric1;
    for (int i = 0; i < 5; ++i) {
        ++metric2;
    }
    for (int i = 0; i < 3; ++i) {
        ++metric3;
    }
    
    metric1.Evaluate();
    metric2.Evaluate();
    metric3.Evaluate();
    
    EXPECT_EQ("1.00", metric1.GetValueAsString());
    EXPECT_EQ("5.00", metric2.GetValueAsString());
    EXPECT_EQ("3.00", metric3.GetValueAsString());
    
    metric2.Reset();
    EXPECT_EQ("1.00", metric1.GetValueAsString());
    EXPECT_EQ("0.00", metric2.GetValueAsString());
    EXPECT_EQ("3.00", metric3.GetValueAsString());
}

TEST_F(HTTPIncomeMetricTest, MetricConceptCompliance) {
    HTTPIncomeMetric test_metric;
    
    std::ostringstream oss;
    oss << test_metric;
    
    std::string output = oss.str();
    EXPECT_TRUE(output.find("HTTPS requests RPS") != std::string::npos);
    EXPECT_TRUE(output.find("0.00") != std::string::npos);
}

TEST_F(HTTPIncomeMetricTest, ComprehensiveIntegrationTest) {
    const int phases = 5;
    const int operations_per_phase = 100;
    
    for (int phase = 0; phase < phases; ++phase) {
        for (int i = 0; i < operations_per_phase; ++i) {
            ++(*metric);
        }
        
        metric->Evaluate();
        EXPECT_EQ(std::to_string(operations_per_phase) + ".00", metric->GetValueAsString());
        
        for (int i = 0; i < operations_per_phase / 2; ++i) {
            ++(*metric);
        }
        
        metric->Evaluate();
        EXPECT_EQ(std::to_string(operations_per_phase / 2) + ".00", metric->GetValueAsString());
        
        if (phase % 3 == 0) {
            metric->Reset();
            EXPECT_EQ("0.00", metric->GetValueAsString());
        }
    }
    
    ++(*metric);
    metric->Evaluate();
    EXPECT_EQ("1.00", metric->GetValueAsString());
}