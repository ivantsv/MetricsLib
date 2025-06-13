#include "IMetrics/IMetrics.h"

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

class HTTPSIncomeMetricTest : public ::testing::Test {
protected:
    void SetUp() override {
        metric = std::make_unique<HTTPSIncomeMetric>();
    }

    void TearDown() override {
        metric.reset();
    }

    std::unique_ptr<HTTPSIncomeMetric> metric;

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

    void simulateHttpRequests(HTTPSIncomeMetric& metric, int request_count, int delay_ms = 0) {
        for (int i = 0; i < request_count; ++i) {
            ++metric;
            if (delay_ms > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            }
        }
    }

    void simulateHttpRequestsBurst(HTTPSIncomeMetric& metric, int burst_count, int requests_per_burst, int burst_interval_ms) {
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

TEST_F(HTTPSIncomeMetricTest, ConstructorDoesNotThrow) {
    EXPECT_NO_THROW({
        HTTPSIncomeMetric test_metric;
    });
}

TEST_F(HTTPSIncomeMetricTest, ConstructorWithStartValueDoesNotThrow) {
    EXPECT_NO_THROW({
        HTTPSIncomeMetric test_metric(100);
    });
}

TEST_F(HTTPSIncomeMetricTest, ConstructorInitializesValidState) {
    std::string initial_value = metric->getValueAsString();
    EXPECT_TRUE(isValidDoubleString(initial_value));
    EXPECT_TRUE(hasCorrectPrecision(initial_value));
    EXPECT_EQ("0.00", initial_value);
}

TEST_F(HTTPSIncomeMetricTest, ConstructorWithStartValueInitializesCorrectly) {
    HTTPSIncomeMetric custom_metric(50);
    std::string initial_value = custom_metric.getValueAsString();
    EXPECT_TRUE(isValidDoubleString(initial_value));
    EXPECT_TRUE(hasCorrectPrecision(initial_value));
    EXPECT_EQ("0.00", initial_value);
}

TEST_F(HTTPSIncomeMetricTest, GetNameReturnsCorrectName) {
    EXPECT_EQ("\"HTTPS requests RPS\"", metric->getName());
}

TEST_F(HTTPSIncomeMetricTest, GetNameIsConsistent) {
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ("\"HTTPS requests RPS\"", metric->getName());
    }
}

TEST_F(HTTPSIncomeMetricTest, GetNameUnchangedAfterOperations) {
    ++(*metric);
    metric->evaluate();
    EXPECT_EQ("\"HTTPS requests RPS\"", metric->getName());
    
    metric->reset();
    EXPECT_EQ("\"HTTPS requests RPS\"", metric->getName());
}

TEST_F(HTTPSIncomeMetricTest, GetValueAsStringReturnsValidDouble) {
    std::string value_str = metric->getValueAsString();
    EXPECT_TRUE(isValidDoubleString(value_str));
}

TEST_F(HTTPSIncomeMetricTest, GetValueAsStringHasCorrectPrecision) {
    std::string value_str = metric->getValueAsString();
    EXPECT_TRUE(hasCorrectPrecision(value_str));
}

TEST_F(HTTPSIncomeMetricTest, GetValueAsStringReturnsNonNegative) {
    std::string value_str = metric->getValueAsString();
    double value = std::stod(value_str);
    EXPECT_GE(value, 0.0);
}

TEST_F(HTTPSIncomeMetricTest, GetValueAsStringMatchesExpectedFormat) {
    ++(*metric);
    metric->evaluate();
    
    std::string value_str = metric->getValueAsString();
    double value = std::stod(value_str);
    
    std::ostringstream expected_format;
    expected_format << std::fixed << std::setprecision(2) << value;
    EXPECT_EQ(expected_format.str(), value_str);
}

TEST_F(HTTPSIncomeMetricTest, PreIncrementDoesNotThrow) {
    EXPECT_NO_THROW(++(*metric));
}

TEST_F(HTTPSIncomeMetricTest, PostIncrementDoesNotThrow) {
    EXPECT_NO_THROW((*metric)++);
}

TEST_F(HTTPSIncomeMetricTest, IncrementReturnsReference) {
    HTTPSIncomeMetric& ref1 = ++(*metric);
    HTTPSIncomeMetric& ref2 = (*metric)++;
    
    EXPECT_EQ(&ref1, metric.get());
    EXPECT_EQ(&ref2, metric.get());
}

TEST_F(HTTPSIncomeMetricTest, MultipleIncrementsWork) {
    EXPECT_NO_THROW({
        for (int i = 0; i < 100; ++i) {
            ++(*metric);
        }
    });
}

TEST_F(HTTPSIncomeMetricTest, EvaluateDoesNotThrow) {
    EXPECT_NO_THROW(metric->evaluate());
}

TEST_F(HTTPSIncomeMetricTest, EvaluateAfterNoRequestsReturnsZero) {
    metric->evaluate();
    EXPECT_EQ("0.00", metric->getValueAsString());
}

TEST_F(HTTPSIncomeMetricTest, EvaluateCalculatesCorrectRPS) {
    for (int i = 0; i < 5; ++i) {
        ++(*metric);
    }
    
    metric->evaluate();
    EXPECT_EQ("5.00", metric->getValueAsString());
}

TEST_F(HTTPSIncomeMetricTest, ConsecutiveEvaluatesWork) {
    for (int i = 0; i < 3; ++i) {
        ++(*metric);
    }
    metric->evaluate();
    EXPECT_EQ("3.00", metric->getValueAsString());
    
    for (int i = 0; i < 2; ++i) {
        ++(*metric);
    }
    metric->evaluate();
    EXPECT_EQ("2.00", metric->getValueAsString());
}

TEST_F(HTTPSIncomeMetricTest, EvaluateCalculatesIncrementalRequests) {
    metric->evaluate();
    EXPECT_EQ("0.00", metric->getValueAsString());
    
    for (int i = 0; i < 10; ++i) {
        ++(*metric);
    }
    metric->evaluate();
    EXPECT_EQ("10.00", metric->getValueAsString());
    
    for (int i = 0; i < 5; ++i) {
        ++(*metric);
    }
    metric->evaluate();
    EXPECT_EQ("5.00", metric->getValueAsString());
    
    metric->evaluate();
    EXPECT_EQ("0.00", metric->getValueAsString());
}

TEST_F(HTTPSIncomeMetricTest, ResetDoesNotThrow) {
    EXPECT_NO_THROW(metric->reset());
}

TEST_F(HTTPSIncomeMetricTest, ResetSetsValueToZero) {
    for (int i = 0; i < 10; ++i) {
        ++(*metric);
    }
    metric->evaluate();
    
    metric->reset();
    EXPECT_EQ("0.00", metric->getValueAsString());
}

TEST_F(HTTPSIncomeMetricTest, ResetClearsInternalCounters) {
    for (int i = 0; i < 15; ++i) {
        ++(*metric);
    }
    metric->evaluate();
    
    metric->reset();
    
    metric->evaluate();
    EXPECT_EQ("0.00", metric->getValueAsString());
    
    for (int i = 0; i < 3; ++i) {
        ++(*metric);
    }
    metric->evaluate();
    EXPECT_EQ("3.00", metric->getValueAsString());
}

TEST_F(HTTPSIncomeMetricTest, ConsecutiveResetsWork) {
    for (int i = 0; i < 3; ++i) {
        ++(*metric);
        metric->evaluate();
        metric->reset();
        EXPECT_EQ("0.00", metric->getValueAsString()) << "Reset " << (i + 1) << " failed";
    }
}

TEST_F(HTTPSIncomeMetricTest, ConcurrentIncrementsAreThreadSafe) {
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
    
    metric->evaluate();
    double total_expected = num_threads * requests_per_thread;
    double actual = std::stod(metric->getValueAsString());
    
    EXPECT_EQ(total_expected, actual);
}

TEST_F(HTTPSIncomeMetricTest, ConcurrentEvaluatesAreThreadSafe) {
    const int num_threads = 8;
    std::vector<std::thread> threads;
    std::atomic<int> completed_evaluations{0};
    
    for (int i = 0; i < 100; ++i) {
        ++(*metric);
    }
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, &completed_evaluations]() {
            for (int j = 0; j < 10; ++j) {
                EXPECT_NO_THROW(metric->evaluate());
                std::string value = metric->getValueAsString();
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

TEST_F(HTTPSIncomeMetricTest, ConcurrentIncrementsAndEvaluates) {
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
                EXPECT_NO_THROW(metric->evaluate());
                std::string value = metric->getValueAsString();
                EXPECT_TRUE(isValidDoubleString(value));
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    should_stop = true;
    
    metric->evaluate();
    std::string final_value = metric->getValueAsString();
    EXPECT_TRUE(isValidDoubleString(final_value));
}

TEST_F(HTTPSIncomeMetricTest, ConcurrentResetAndOperations) {
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
            EXPECT_NO_THROW(metric->evaluate());
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });
    
    threads.emplace_back([this, &should_stop]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        for (int i = 0; i < 5 && !should_stop; ++i) {
            EXPECT_NO_THROW(metric->reset());
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    should_stop = true;
    
    for (auto& t : threads) {
        t.join();
    }
    
    metric->reset();
    ++(*metric);
    metric->evaluate();
    EXPECT_EQ("1.00", metric->getValueAsString());
}

TEST_F(HTTPSIncomeMetricTest, WorksThroughIMetricPointer) {
    std::unique_ptr<IMetric> metric_ptr = std::make_unique<HTTPSIncomeMetric>();
    
    EXPECT_EQ("\"HTTPS requests RPS\"", metric_ptr->getName());
    
    std::string value = metric_ptr->getValueAsString();
    EXPECT_TRUE(isValidDoubleString(value));
    
    EXPECT_NO_THROW(metric_ptr->evaluate());
    EXPECT_NO_THROW(metric_ptr->reset());
    EXPECT_EQ("0.00", metric_ptr->getValueAsString());
}

TEST_F(HTTPSIncomeMetricTest, HighVolumeIncrementsPerformance) {
    const int num_requests = 100000;
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_requests; ++i) {
        ++(*metric);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 1000) << "100k increments took too long: " << duration.count() << "ms";
    
    metric->evaluate();
    EXPECT_EQ(std::to_string(num_requests) + ".00", metric->getValueAsString());
}

TEST_F(HTTPSIncomeMetricTest, EvaluateCompletesQuickly) {
    for (int i = 0; i < 1000; ++i) {
        ++(*metric);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    metric->evaluate();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 10) << "evaluate() took too long: " << duration.count() << "ms";
}

TEST_F(HTTPSIncomeMetricTest, SimulatedHttpRequestsBurst) {
    for (int i = 0; i < 50; ++i) {
        ++(*metric);
    }
    
    metric->evaluate();
    EXPECT_EQ("50.00", metric->getValueAsString());
    
    metric->evaluate();
    EXPECT_EQ("0.00", metric->getValueAsString());
}

TEST_F(HTTPSIncomeMetricTest, SimulatedSteadyTraffic) {
    std::vector<int> request_intervals = {10, 15, 8, 12, 20, 5};
    std::vector<std::string> expected_values = {"10.00", "15.00", "8.00", "12.00", "20.00", "5.00"};
    
    for (size_t i = 0; i < request_intervals.size(); ++i) {
        for (int j = 0; j < request_intervals[i]; ++j) {
            ++(*metric);
        }
        metric->evaluate();
        EXPECT_EQ(expected_values[i], metric->getValueAsString()) 
            << "Interval " << i << " failed";
    }
}

TEST_F(HTTPSIncomeMetricTest, SimulatedVariableLoad) {
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
        
        metric->evaluate();
        actual_values.push_back(metric->getValueAsString());
        
        std::ostringstream expected;
        expected << std::fixed << std::setprecision(2) << static_cast<double>(requests);
        EXPECT_EQ(expected.str(), actual_values[i]) << "Iteration " << i 
                  << " with " << requests << " requests failed";
    }
}

TEST_F(HTTPSIncomeMetricTest, ZeroRequestsConsistentBehavior) {
    for (int i = 0; i < 5; ++i) {
        metric->evaluate();
        EXPECT_EQ("0.00", metric->getValueAsString()) << "Evaluation " << i << " failed";
    }
}

TEST_F(HTTPSIncomeMetricTest, LargeNumberOfRequests) {
    const unsigned long long large_number = 1000000;
    
    for (unsigned long long i = 0; i < large_number; ++i) {
        ++(*metric);
    }
    
    metric->evaluate();
    
    std::ostringstream expected;
    expected << std::fixed << std::setprecision(2) << static_cast<double>(large_number);
    EXPECT_EQ(expected.str(), metric->getValueAsString());
}

TEST_F(HTTPSIncomeMetricTest, ValueStringFormatConsistency) {
    std::vector<int> test_values = {0, 1, 10, 100, 1000};
    
    for (int value : test_values) {
        metric->reset();
        
        for (int i = 0; i < value; ++i) {
            ++(*metric);
        }
        
        metric->evaluate();
        std::string result = metric->getValueAsString();
        
        EXPECT_TRUE(hasCorrectPrecision(result)) << "Value " << value << " has incorrect precision";
        EXPECT_TRUE(isValidDoubleString(result)) << "Value " << value << " is not valid double";
        
        std::ostringstream expected;
        expected << std::fixed << std::setprecision(2) << static_cast<double>(value);
        EXPECT_EQ(expected.str(), result) << "Value " << value << " format mismatch";
    }
}

class HTTPSIncomeMetricStressTest : public ::testing::Test {
protected:
    void SetUp() override {
        metric = std::make_unique<HTTPSIncomeMetric>();
    }

    std::unique_ptr<HTTPSIncomeMetric> metric;
};

TEST_F(HTTPSIncomeMetricStressTest, HighFrequencyOperations) {
    const int cycles = 100;
    
    for (int cycle = 0; cycle < cycles; ++cycle) {
        for (int i = 0; i < 1000; ++i) {
            ++(*metric);
        }
        
        metric->evaluate();
        EXPECT_EQ("1000.00", metric->getValueAsString());
        
        metric->reset();
        EXPECT_EQ("0.00", metric->getValueAsString());
    }
    
    ++(*metric);
    metric->evaluate();
    EXPECT_EQ("1.00", metric->getValueAsString());
}

TEST_F(HTTPSIncomeMetricStressTest, ConcurrentStressTest) {
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
                    metric->evaluate();
                }
                
                if (j % 200 == 0) {
                    std::string value = metric->getValueAsString();
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

TEST_F(HTTPSIncomeMetricTest, ConstructorWithMaxValue) {
    HTTPSIncomeMetric max_metric(ULLONG_MAX);
    EXPECT_EQ("0.00", max_metric.getValueAsString());
    EXPECT_NO_THROW(max_metric.evaluate());
}

TEST_F(HTTPSIncomeMetricTest, EvaluateAfterReset) {
    for (int i = 0; i < 10; ++i) {
        ++(*metric);
    }
    metric->evaluate();
    EXPECT_EQ("10.00", metric->getValueAsString());
    
    metric->reset();
    metric->evaluate();
    EXPECT_EQ("0.00", metric->getValueAsString());
}

TEST_F(HTTPSIncomeMetricTest, MultipleEvaluationsWithoutIncrements) {
    metric->evaluate();
    EXPECT_EQ("0.00", metric->getValueAsString());
    
    for (int i = 0; i < 5; ++i) {
        metric->evaluate();
        EXPECT_EQ("0.00", metric->getValueAsString()) << "Evaluation " << i << " failed";
    }
}

TEST_F(HTTPSIncomeMetricTest, IncrementBetweenEvaluations) {
    metric->evaluate();
    EXPECT_EQ("0.00", metric->getValueAsString());
    
    ++(*metric);
    metric->evaluate();
    EXPECT_EQ("1.00", metric->getValueAsString());
    
    ++(*metric);
    metric->evaluate();
    EXPECT_EQ("1.00", metric->getValueAsString());
    
    metric->evaluate();
    EXPECT_EQ("0.00", metric->getValueAsString());
}

TEST_F(HTTPSIncomeMetricTest, ChainedIncrements) {
    HTTPSIncomeMetric& ref1 = ++(*metric);
    HTTPSIncomeMetric& ref2 = ++ref1;
    HTTPSIncomeMetric& ref3 = ++ref2;
    
    EXPECT_EQ(&ref1, metric.get());
    EXPECT_EQ(&ref2, metric.get());
    EXPECT_EQ(&ref3, metric.get());
    
    metric->evaluate();
    EXPECT_EQ("3.00", metric->getValueAsString());
}

TEST_F(HTTPSIncomeMetricTest, MixedIncrementTypes) {
    ++(*metric);
    (*metric)++;
    ++(*metric);
    (*metric)++;
    
    metric->evaluate();
    EXPECT_EQ("4.00", metric->getValueAsString());
}

TEST_F(HTTPSIncomeMetricTest, MemoryStability) {
    for (int cycle = 0; cycle < 100; ++cycle) {
        for (int i = 0; i < 1000; ++i) {
            ++(*metric);
        }
        metric->evaluate();
        EXPECT_EQ("1000.00", metric->getValueAsString());
        metric->reset();
    }
    
    ++(*metric);
    metric->evaluate();
    EXPECT_EQ("1.00", metric->getValueAsString());
}

TEST_F(HTTPSIncomeMetricTest, AtomicOperationsConsistency) {
    const int num_operations = 10000;
    
    for (int i = 0; i < num_operations; ++i) {
        ++(*metric);
    }
    
    metric->evaluate();
    EXPECT_EQ(std::to_string(num_operations) + ".00", metric->getValueAsString());
    
    metric->evaluate();
    EXPECT_EQ("0.00", metric->getValueAsString());
}

TEST_F(HTTPSIncomeMetricTest, StateConsistencyAfterOperations) {
    EXPECT_EQ("0.00", metric->getValueAsString());
    EXPECT_EQ("\"HTTPS requests RPS\"", metric->getName());
    
    ++(*metric);
    EXPECT_EQ("\"HTTPS requests RPS\"", metric->getName());
    
    metric->evaluate();
    EXPECT_EQ("\"HTTPS requests RPS\"", metric->getName());
    EXPECT_NE("0.00", metric->getValueAsString());
    
    metric->reset();
    EXPECT_EQ("\"HTTPS requests RPS\"", metric->getName());
    EXPECT_EQ("0.00", metric->getValueAsString());
}

TEST_F(HTTPSIncomeMetricTest, ConcurrentReadOperations) {
    const int num_threads = 8;
    std::vector<std::thread> threads;
    std::atomic<int> successful_reads{0};
    
    for (int i = 0; i < 50; ++i) {
        ++(*metric);
    }
    metric->evaluate();
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, &successful_reads]() {
            for (int j = 0; j < 100; ++j) {
                std::string name = metric->getName();
                std::string value = metric->getValueAsString();
                
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

TEST_F(HTTPSIncomeMetricTest, ExtremeValueHandling) {
    HTTPSIncomeMetric large_metric(ULLONG_MAX - 1000);
    
    for (int i = 0; i < 100; ++i) {
        EXPECT_NO_THROW(++large_metric);
    }
    
    EXPECT_NO_THROW(large_metric.evaluate());
    std::string value = large_metric.getValueAsString();
    EXPECT_TRUE(isValidDoubleString(value));
    EXPECT_TRUE(hasCorrectPrecision(value));
}

TEST_F(HTTPSIncomeMetricTest, RapidResetOperations) {
    for (int i = 0; i < 1000; ++i) {
        ++(*metric);
        metric->evaluate();
        metric->reset();
        EXPECT_EQ("0.00", metric->getValueAsString());
    }
}

TEST_F(HTTPSIncomeMetricTest, ComplexWorkloadSimulation) {
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
        
        metric->evaluate();
        std::string expected = std::to_string(requests) + ".00";
        EXPECT_EQ(expected, metric->getValueAsString()) 
            << "Phase " << phase << " with " << requests << " requests failed";
    }
}

TEST_F(HTTPSIncomeMetricTest, InterleavedOperations) {
    for (int cycle = 0; cycle < 10; ++cycle) {
        for (int i = 0; i < 5; ++i) {
            ++(*metric);
        }
        
        metric->evaluate();
        EXPECT_EQ("5.00", metric->getValueAsString());
        
        for (int i = 0; i < 3; ++i) {
            ++(*metric);
        }
        
        metric->evaluate();
        EXPECT_EQ("3.00", metric->getValueAsString());
        
        if (cycle % 3 == 0) {
            metric->reset();
            EXPECT_EQ("0.00", metric->getValueAsString());
        }
    }
}

TEST_F(HTTPSIncomeMetricTest, AtomicIncrementConsistency) {
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
    
    metric->evaluate();
    int expected_total = num_threads * increments_per_thread;
    EXPECT_EQ(std::to_string(expected_total) + ".00", metric->getValueAsString());
}

TEST_F(HTTPSIncomeMetricTest, ConcurrentEvaluateAndIncrement) {
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
                metric->evaluate();
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
    
    metric->evaluate();
    std::string value = metric->getValueAsString();
    EXPECT_TRUE(isValidDoubleString(value));
}

TEST_F(HTTPSIncomeMetricTest, OverflowHandling) {
    HTTPSIncomeMetric overflow_metric(ULLONG_MAX - 10);
    
    for (int i = 0; i < 5; ++i) {
        EXPECT_NO_THROW(++overflow_metric);
    }
    
    overflow_metric.evaluate();
    std::string value = overflow_metric.getValueAsString();
    EXPECT_TRUE(isValidDoubleString(value));
    EXPECT_GE(std::stod(value), 0.0);
}

TEST_F(HTTPSIncomeMetricTest, ZeroToNonZeroTransition) {
    for (int i = 0; i < 10; ++i) {
        metric->evaluate();
        EXPECT_EQ("0.00", metric->getValueAsString());
    }
    
    ++(*metric);
    metric->evaluate();
    EXPECT_EQ("1.00", metric->getValueAsString());
    
    metric->evaluate();
    EXPECT_EQ("0.00", metric->getValueAsString());
}

TEST_F(HTTPSIncomeMetricTest, DecimalPrecisionEdgeCases) {
    std::vector<int> test_values = {1, 9, 10, 99, 100, 999, 1000, 9999, 10000};
    
    for (int value : test_values) {
        metric->reset();
        
        for (int i = 0; i < value; ++i) {
            ++(*metric);
        }
        
        metric->evaluate();
        std::string result = metric->getValueAsString();
        
        EXPECT_TRUE(hasCorrectPrecision(result)) << "Value " << value << " precision test failed";
        
        std::ostringstream expected;
        expected << std::fixed << std::setprecision(2) << static_cast<double>(value);
        EXPECT_EQ(expected.str(), result) << "Value " << value << " format test failed";
    }
}

TEST_F(HTTPSIncomeMetricTest, MultipleInstancesIndependence) {
    HTTPSIncomeMetric metric1;
    HTTPSIncomeMetric metric2(100);
    HTTPSIncomeMetric metric3(50);
    
    ++metric1;
    for (int i = 0; i < 5; ++i) {
        ++metric2;
    }
    for (int i = 0; i < 3; ++i) {
        ++metric3;
    }
    
    metric1.evaluate();
    metric2.evaluate();
    metric3.evaluate();
    
    EXPECT_EQ("1.00", metric1.getValueAsString());
    EXPECT_EQ("5.00", metric2.getValueAsString());
    EXPECT_EQ("3.00", metric3.getValueAsString());
    
    metric2.reset();
    EXPECT_EQ("1.00", metric1.getValueAsString());
    EXPECT_EQ("0.00", metric2.getValueAsString());
    EXPECT_EQ("3.00", metric3.getValueAsString());
}

TEST_F(HTTPSIncomeMetricTest, MetricConceptCompliance) {
    HTTPSIncomeMetric test_metric;
    
    std::ostringstream oss;
    oss << test_metric;
    
    std::string output = oss.str();
    EXPECT_TRUE(output.find("HTTPS requests RPS") != std::string::npos);
    EXPECT_TRUE(output.find("0.00") != std::string::npos);
}

TEST_F(HTTPSIncomeMetricTest, ComprehensiveIntegrationTest) {
    const int phases = 5;
    const int operations_per_phase = 100;
    
    for (int phase = 0; phase < phases; ++phase) {
        for (int i = 0; i < operations_per_phase; ++i) {
            ++(*metric);
        }
        
        metric->evaluate();
        EXPECT_EQ(std::to_string(operations_per_phase) + ".00", metric->getValueAsString());
        
        for (int i = 0; i < operations_per_phase / 2; ++i) {
            ++(*metric);
        }
        
        metric->evaluate();
        EXPECT_EQ(std::to_string(operations_per_phase / 2) + ".00", metric->getValueAsString());
        
        if (phase % 3 == 0) {
            metric->reset();
            EXPECT_EQ("0.00", metric->getValueAsString());
        }
    }
    
    ++(*metric);
    metric->evaluate();
    EXPECT_EQ("1.00", metric->getValueAsString());
}