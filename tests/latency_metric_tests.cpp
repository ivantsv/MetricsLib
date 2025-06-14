#include "IMetrics/LatencyMetric.h"

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <memory>
#include <sstream>
#include <vector>
#include <random>
#include <atomic>

#include <regex>

using namespace Metrics;
using namespace std::chrono_literals;

class LatencyMetricTest : public ::testing::Test {
protected:
    void SetUp() override {
        metric = std::make_unique<LatencyMetric>();
    }

    void TearDown() override {
        metric->Reset();
    }

    std::unique_ptr<LatencyMetric> metric;

    bool isValidPercentileString(const std::string& str) {
        std::regex percentile_regex(R"(P90: \d+(?:\.\d+)?(?:[eE][+-]?\d+)?ns, P95: \d+(?:\.\d+)?(?:[eE][+-]?\d+)?ns, P99: \d+(?:\.\d+)?(?:[eE][+-]?\d+)?ns, P999: \d+(?:\.\d+)?(?:[eE][+-]?\d+)?ns)");
        return std::regex_match(str, percentile_regex);
    }

    bool hasValidFormat(const std::string& str) {
        return str.find("P90:") != std::string::npos &&
               str.find("P95:") != std::string::npos &&
               str.find("P99:") != std::string::npos &&
               str.find("P999:") != std::string::npos &&
               str.find("ns") != std::string::npos;
    }

    std::vector<double> extractPercentileValues(const std::string& str) {
        std::vector<double> values;
        std::regex number_regex(R"((\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)ns)");
        std::sregex_iterator iter(str.begin(), str.end(), number_regex);
        std::sregex_iterator end;

        for (; iter != end; ++iter) {
            values.push_back(std::stod((*iter)[1]));
        }
        return values;
    }

    void observeLatencies(int count, std::chrono::nanoseconds base_latency, std::chrono::nanoseconds variance = 0ns) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<long long> dist(-variance.count(), variance.count());

        for (int i = 0; i < count; ++i) {
            auto latency = base_latency + std::chrono::nanoseconds(dist(gen));
            if (latency.count() < 1) latency = 1ns;
            metric->Observe(latency);
        }
    }
};

TEST_F(LatencyMetricTest, ConstructorDoesNotThrow) {
    EXPECT_NO_THROW({
        LatencyMetric test_metric;
    });
}

TEST_F(LatencyMetricTest, ConstructorInitializesValidState) {
    std::string initial_value = metric->GetValueAsString();
    EXPECT_TRUE(hasValidFormat(initial_value));
    EXPECT_TRUE(isValidPercentileString(initial_value));
}

TEST_F(LatencyMetricTest, GetNameReturnsCorrectName) {
    EXPECT_EQ("\"Percentile Latency\"", metric->GetName());
}

TEST_F(LatencyMetricTest, GetNameIsConsistent) {
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ("\"Percentile Latency\"", metric->GetName());
    }
}

TEST_F(LatencyMetricTest, GetNameUnchangedAfterOperations) {
    metric->Observe(1000ns);
    metric->Evaluate();
    EXPECT_EQ("\"Percentile Latency\"", metric->GetName());
    
    metric->Reset();
    EXPECT_EQ("\"Percentile Latency\"", metric->GetName());
}

TEST_F(LatencyMetricTest, GetValueAsStringReturnsValidFormat) {
    std::string value_str = metric->GetValueAsString();
    EXPECT_TRUE(hasValidFormat(value_str));
    EXPECT_TRUE(isValidPercentileString(value_str));
}

TEST_F(LatencyMetricTest, GetValueAsStringAfterObservations) {
    metric->Observe(1000ns);
    metric->Observe(2000ns);
    metric->Observe(5000ns);
    
    std::string value_str = metric->GetValueAsString();
    EXPECT_TRUE(hasValidFormat(value_str));
    EXPECT_TRUE(isValidPercentileString(value_str));
    
    std::vector<double> values = extractPercentileValues(value_str);
    EXPECT_EQ(values.size(), 4);
    
    for (double val : values) {
        EXPECT_GE(val, 0.0);
    }
}

TEST_F(LatencyMetricTest, ObserveDoesNotThrow) {
    EXPECT_NO_THROW(metric->Observe(1000ns));
    EXPECT_NO_THROW(metric->Observe(1ns));
    EXPECT_NO_THROW(metric->Observe(1000000ns));
}

TEST_F(LatencyMetricTest, ObserveVerySmallValues) {
    metric->Observe(1ns);
    metric->Observe(5ns);
    metric->Observe(10ns);
    
    std::string value_str = metric->GetValueAsString();
    EXPECT_TRUE(hasValidFormat(value_str));
    
    std::vector<double> values = extractPercentileValues(value_str);
    for (double val : values) {
        EXPECT_GE(val, 0.0);
        EXPECT_LE(val, 1000.0);
    }
}

TEST_F(LatencyMetricTest, ObserveLargeValues) {
    metric->Observe(1000000ns);
    metric->Observe(2000000ns);
    metric->Observe(5000000ns);
    metric->Observe(10000000ns);
    
    std::string value_str = metric->GetValueAsString();
    EXPECT_TRUE(hasValidFormat(value_str));
    
    std::vector<double> values = extractPercentileValues(value_str);
    ASSERT_EQ(values.size(), 4);
    
    for (double val : values) {
        EXPECT_GE(val, 1000000.0);
    }
}

TEST_F(LatencyMetricTest, PercentilesAreInOrder) {
    observeLatencies(1000, 1000ns, 500ns);
    
    std::string value_str = metric->GetValueAsString();
    std::vector<double> values = extractPercentileValues(value_str);
    
    ASSERT_EQ(values.size(), 4);
    EXPECT_LE(values[0], values[1]);
    EXPECT_LE(values[1], values[2]);
    EXPECT_LE(values[2], values[3]);
}

TEST_F(LatencyMetricTest, EvaluateDoesNotThrow) {
    EXPECT_NO_THROW(metric->Evaluate());
    
    metric->Observe(1000ns);
    EXPECT_NO_THROW(metric->Evaluate());
}

TEST_F(LatencyMetricTest, EvaluateDoesNotChangeValues) {
    metric->Observe(1000ns);
    
    std::string value_before = metric->GetValueAsString();
    metric->Evaluate();
    std::string value_after = metric->GetValueAsString();
    
    EXPECT_EQ(value_before, value_after);
}

TEST_F(LatencyMetricTest, ResetDoesNotThrow) {
    EXPECT_NO_THROW(metric->Reset());
    
    metric->Observe(1000ns);
    EXPECT_NO_THROW(metric->Reset());
}

TEST_F(LatencyMetricTest, ResetClearsObservations) {
    observeLatencies(100, 5000ns, 1000ns);
    
    std::string value_before_reset = metric->GetValueAsString();
    std::vector<double> values_before = extractPercentileValues(value_before_reset);
    
    metric->Reset();
    
    std::string value_after_reset = metric->GetValueAsString();
    std::vector<double> values_after = extractPercentileValues(value_after_reset);
    
    ASSERT_EQ(values_after.size(), 4);
    for (double val : values_after) {
        EXPECT_LE(val, values_before[0]);
    }
}

TEST_F(LatencyMetricTest, ConsecutiveResetsWork) {
    for (int i = 0; i < 5; ++i) {
        metric->Observe(1000ns * (i + 1));
        metric->Reset();
        
        std::string value = metric->GetValueAsString();
        EXPECT_TRUE(hasValidFormat(value));
    }
}

TEST_F(LatencyMetricTest, MultipleObservationsIncreasePrecision) {
    metric->Observe(1000ns);
    std::string value_single = metric->GetValueAsString();
    
    metric->Reset();
    observeLatencies(1000, 1000ns, 100ns);
    std::string value_multiple = metric->GetValueAsString();
    
    EXPECT_TRUE(hasValidFormat(value_single));
    EXPECT_TRUE(hasValidFormat(value_multiple));
}

TEST_F(LatencyMetricTest, DifferentLatencyDistributions) {
    struct TestCase {
        std::string name;
        std::chrono::nanoseconds base;
        std::chrono::nanoseconds variance;
        int count;
    };
    
    std::vector<TestCase> test_cases = {
        {"Low latency", 100ns, 50ns, 1000},
        {"Medium latency", 10000ns, 5000ns, 500},
        {"High latency", 1000000ns, 500000ns, 100},
        {"Uniform distribution", 5000ns, 4000ns, 1000}
    };
    
    for (const auto& test_case : test_cases) {
        metric->Reset();
        observeLatencies(test_case.count, test_case.base, test_case.variance);
        
        std::string value = metric->GetValueAsString();
        EXPECT_TRUE(hasValidFormat(value)) << "Failed for: " << test_case.name;
        
        std::vector<double> percentiles = extractPercentileValues(value);
        ASSERT_EQ(percentiles.size(), 4) << "Failed for: " << test_case.name;
        
        EXPECT_LE(percentiles[0], percentiles[1]) << "P90 > P95 for: " << test_case.name;
        EXPECT_LE(percentiles[1], percentiles[2]) << "P95 > P99 for: " << test_case.name;
        EXPECT_LE(percentiles[2], percentiles[3]) << "P99 > P999 for: " << test_case.name;
    }
}

TEST_F(LatencyMetricTest, ConcurrentObservationsAreThreadSafe) {
    const int num_threads = 8;
    const int observations_per_thread = 1000;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i, observations_per_thread]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<long long> dist(1000, 10000);
            
            for (int j = 0; j < observations_per_thread; ++j) {
                auto latency = std::chrono::nanoseconds(dist(gen));
                metric->Observe(latency);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(hasValidFormat(value));
    
    std::vector<double> percentiles = extractPercentileValues(value);
    ASSERT_EQ(percentiles.size(), 4);
    
    for (double val : percentiles) {
        EXPECT_GE(val, 1000.0);
        EXPECT_LE(val, 10000.0);
    }
}

TEST_F(LatencyMetricTest, ConcurrentObservationsAndReads) {
    const int num_observer_threads = 4;
    const int num_reader_threads = 2;
    const int operations_per_thread = 500;
    
    std::vector<std::thread> threads;
    std::atomic<bool> should_stop{false};
    std::atomic<int> successful_reads{0};
    
    for (int i = 0; i < num_observer_threads; ++i) {
        threads.emplace_back([this, &should_stop, operations_per_thread]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<long long> dist(500, 5000);
            
            for (int j = 0; j < operations_per_thread && !should_stop; ++j) {
                auto latency = std::chrono::nanoseconds(dist(gen));
                metric->Observe(latency);
                std::this_thread::sleep_for(1us);
            }
        });
    }
    
    for (int i = 0; i < num_reader_threads; ++i) {
        threads.emplace_back([this, &should_stop, &successful_reads]() {
            while (!should_stop) {
                std::string value = metric->GetValueAsString();
                if (hasValidFormat(value)) {
                    ++successful_reads;
                }
                std::this_thread::sleep_for(1ms);
            }
        });
    }
    
    std::this_thread::sleep_for(100ms);
    should_stop = true;
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_GT(successful_reads.load(), 0);
}

TEST_F(LatencyMetricTest, ConcurrentResetAndOperations) {
    std::vector<std::thread> threads;
    std::atomic<bool> should_stop{false};
    
    threads.emplace_back([this, &should_stop]() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<long long> dist(1000, 5000);
        
        for (int i = 0; i < 1000 && !should_stop; ++i) {
            auto latency = std::chrono::nanoseconds(dist(gen));
            metric->Observe(latency);
            std::this_thread::sleep_for(100us);
        }
    });
    
    threads.emplace_back([this, &should_stop]() {
        for (int i = 0; i < 100 && !should_stop; ++i) {
            std::string value = metric->GetValueAsString();
            EXPECT_TRUE(hasValidFormat(value));
            std::this_thread::sleep_for(1ms);
        }
    });
    
    threads.emplace_back([this, &should_stop]() {
        std::this_thread::sleep_for(50ms);
        for (int i = 0; i < 10 && !should_stop; ++i) {
            EXPECT_NO_THROW(metric->Reset());
            std::this_thread::sleep_for(10ms);
        }
    });
    
    std::this_thread::sleep_for(200ms);
    should_stop = true;
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::string final_value = metric->GetValueAsString();
    EXPECT_TRUE(hasValidFormat(final_value));
}

TEST_F(LatencyMetricTest, WorksThroughIMetricPointer) {
    std::unique_ptr<IMetric> metric_ptr = std::make_unique<LatencyMetric>();
    
    EXPECT_EQ("\"Percentile Latency\"", metric_ptr->GetName());
    
    std::string value = metric_ptr->GetValueAsString();
    EXPECT_TRUE(hasValidFormat(value));
    
    EXPECT_NO_THROW(metric_ptr->Evaluate());
    EXPECT_NO_THROW(metric_ptr->Reset());
    
    std::string value_after_reset = metric_ptr->GetValueAsString();
    EXPECT_TRUE(hasValidFormat(value_after_reset));
}

TEST_F(LatencyMetricTest, HighVolumeObservationsPerformance) {
    const int num_observations = 100000;
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_observations; ++i) {
        metric->Observe(std::chrono::nanoseconds(1000 + (i % 1000)));
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 5000) << "100k observations took too long: " << duration.count() << "ms";
    
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(hasValidFormat(value));
}

TEST_F(LatencyMetricTest, GetValueAsStringCompletesQuickly) {
    observeLatencies(10000, 1000ns, 500ns);
    
    auto start = std::chrono::high_resolution_clock::now();
    std::string value = metric->GetValueAsString();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 100) << "GetValueAsString() took too long: " << duration.count() << "ms";
    
    EXPECT_TRUE(hasValidFormat(value));
}

TEST_F(LatencyMetricTest, ExtremeLatencyValues) {
    metric->Observe(1ns);
    metric->Observe(std::chrono::hours(1));
    metric->Observe(std::chrono::milliseconds(500));
    
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(hasValidFormat(value));
    
    std::vector<double> percentiles = extractPercentileValues(value);
    ASSERT_EQ(percentiles.size(), 4);
    
    for (double val : percentiles) {
        EXPECT_GE(val, 0.0);
    }
}

TEST_F(LatencyMetricTest, LatencyStatisticalProperties) {
    const int sample_size = 10000;
    const auto target_latency = 5000ns;
    const auto variance = 1000ns;
    
    observeLatencies(sample_size, target_latency, variance);
    
    std::string value = metric->GetValueAsString();
    std::vector<double> percentiles = extractPercentileValues(value);
    
    ASSERT_EQ(percentiles.size(), 4);
    
    double p90 = percentiles[0];
    double p95 = percentiles[1];
    double p99 = percentiles[2];
    double p999 = percentiles[3];
    
    EXPECT_GE(p90, target_latency.count() - variance.count() * 2);
    EXPECT_LE(p90, target_latency.count() + variance.count() * 3);
    
    EXPECT_GE(p95, p90);
    EXPECT_GE(p99, p95);
    EXPECT_GE(p999, p99);
    
    EXPECT_LE(p999, target_latency.count() + variance.count() * 5);
}

TEST_F(LatencyMetricTest, ResetBetweenMeasurements) {
    observeLatencies(1000, 1000ns, 100ns);
    std::vector<double> first_measurement = extractPercentileValues(metric->GetValueAsString());
    
    metric->Reset();
    
    observeLatencies(1000, 10000ns, 1000ns);
    std::vector<double> second_measurement = extractPercentileValues(metric->GetValueAsString());
    
    ASSERT_EQ(first_measurement.size(), 4);
    ASSERT_EQ(second_measurement.size(), 4);
    
    for (size_t i = 0; i < 4; ++i) {
        EXPECT_LT(first_measurement[i], second_measurement[i] * 0.8);
    }
}

class LatencyMetricStressTest : public ::testing::Test {
protected:
    void SetUp() override {
        metric = std::make_unique<LatencyMetric>();
    }

    std::unique_ptr<LatencyMetric> metric;
};

TEST_F(LatencyMetricStressTest, HighFrequencyOperations) {
    const int cycles = 100;
    const int observations_per_cycle = 1000;
    
    for (int cycle = 0; cycle < cycles; ++cycle) {
        for (int i = 0; i < observations_per_cycle; ++i) {
            auto latency = std::chrono::nanoseconds(1000 + (i % 5000));
            metric->Observe(latency);
        }
        
        if (cycle % 10 == 0) {
            std::string value = metric->GetValueAsString();
            EXPECT_TRUE(value.find("P90:") != std::string::npos);
            
            if (cycle % 20 == 0) {
                metric->Reset();
            }
        }
    }
    
    std::string final_value = metric->GetValueAsString();
    EXPECT_TRUE(final_value.find("P90:") != std::string::npos);
}

TEST_F(LatencyMetricStressTest, ConcurrentStressTest) {
    const int num_threads = 12;
    const int operations_per_thread = 5000;
    std::vector<std::thread> threads;
    std::atomic<int> total_observations{0};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i, operations_per_thread, &total_observations]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<long long> dist(100, 50000);
            
            for (int j = 0; j < operations_per_thread; ++j) {
                auto latency = std::chrono::nanoseconds(dist(gen));
                metric->Observe(latency);
                ++total_observations;
                
                if (j % 1000 == 0) {
                    std::string value = metric->GetValueAsString();
                }
                
                if (j % 2000 == 0) {
                    std::this_thread::sleep_for(1us);
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(total_observations.load(), num_threads * operations_per_thread);
    
    std::string final_value = metric->GetValueAsString();
    EXPECT_TRUE(final_value.find("P90:") != std::string::npos);
    EXPECT_TRUE(final_value.find("P95:") != std::string::npos);
    EXPECT_TRUE(final_value.find("P99:") != std::string::npos);
    EXPECT_TRUE(final_value.find("P999:") != std::string::npos);
}

TEST_F(LatencyMetricTest, MemoryStability) {
    for (int cycle = 0; cycle < 500; ++cycle) {
        for (int i = 0; i < 100; ++i) {
            auto latency = std::chrono::nanoseconds(1000 + (i * 10));
            metric->Observe(latency);
        }
        
        if (cycle % 50 == 0) {
            std::string value = metric->GetValueAsString();
            EXPECT_TRUE(value.find("P90:") != std::string::npos);
        }
        
        if (cycle % 100 == 0) {
            metric->Reset();
        }
    }
    
    std::string final_value = metric->GetValueAsString();
    EXPECT_TRUE(final_value.find("P90:") != std::string::npos);
}

TEST_F(LatencyMetricTest, MetricConceptCompliance) {
    LatencyMetric test_metric;
    
    test_metric.Observe(1000ns);
    test_metric.Observe(2000ns);
    test_metric.Observe(5000ns);
    
    std::ostringstream oss;
    oss << test_metric;
    
    std::string output = oss.str();
    EXPECT_TRUE(output.find("Percentile Latency") != std::string::npos);
    EXPECT_TRUE(output.find("P90:") != std::string::npos);
}

TEST_F(LatencyMetricTest, HistogramDistributionAnalysis) {
    const int sample_count = 10000;
    
    auto low_latency = 1000ns;
    auto high_latency = 10000ns;
    
    for (int i = 0; i < sample_count / 2; ++i) {
        metric->Observe(low_latency);
    }
    for (int i = 0; i < sample_count / 2; ++i) {
        metric->Observe(high_latency);
    }
    
    std::string value = metric->GetValueAsString();
    std::vector<double> percentiles = extractPercentileValues(value);
    
    ASSERT_EQ(percentiles.size(), 4);
    
    double p90 = percentiles[0];
    double p95 = percentiles[1];
    double p99 = percentiles[2];
    double p999 = percentiles[3];
    
    EXPECT_GE(p90, low_latency.count());
    EXPECT_LE(p99, high_latency.count() * 1.1);
    
    EXPECT_LE(p90, p95);
    EXPECT_LE(p95, p99);
    EXPECT_LE(p99, p999);
}

TEST_F(LatencyMetricTest, HistogramNormalDistribution) {
    const int sample_count = 5000;
    const auto mean_latency = 5000ns;
    const auto std_dev = 1000ns;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<double> dist(mean_latency.count(), std_dev.count());
    
    for (int i = 0; i < sample_count; ++i) {
        auto latency_value = static_cast<long long>(std::abs(dist(gen)));
        if (latency_value < 1) latency_value = 1;
        metric->Observe(std::chrono::nanoseconds(latency_value));
    }
    
    std::string value = metric->GetValueAsString();
    std::vector<double> percentiles = extractPercentileValues(value);
    
    ASSERT_EQ(percentiles.size(), 4);
    
    double p90 = percentiles[0];
    double p95 = percentiles[1];
    double p99 = percentiles[2];
    
    EXPECT_GE(p90, mean_latency.count() - 2 * std_dev.count());
    EXPECT_LE(p99, mean_latency.count() + 4 * std_dev.count());
    
    EXPECT_GT(p95, p90);
    EXPECT_GT(p99, p95);
}

TEST_F(LatencyMetricTest, HistogramExponentialDistribution) {
    const int sample_count = 3000;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::exponential_distribution<double> dist(0.001);
    
    for (int i = 0; i < sample_count; ++i) {
        auto latency_value = static_cast<long long>(dist(gen)) + 100;
        metric->Observe(std::chrono::nanoseconds(latency_value));
    }
    
    std::string value = metric->GetValueAsString();
    std::vector<double> percentiles = extractPercentileValues(value);
    
    ASSERT_EQ(percentiles.size(), 4);
    
    for (size_t i = 0; i < percentiles.size() - 1; ++i) {
        EXPECT_LT(percentiles[i], percentiles[i + 1]);
    }
    
    EXPECT_GT(percentiles[3] / percentiles[0], 2.0);
}

TEST_F(LatencyMetricTest, ComprehensiveIntegrationTest) {
    const int phases = 5;
    const int base_observations = 1000;
    
    for (int phase = 0; phase < phases; ++phase) {
        auto base_latency = std::chrono::nanoseconds(1000 * (phase + 1));
        auto variance = std::chrono::nanoseconds(500 * (phase + 1));
        
        observeLatencies(base_observations * (phase + 1), base_latency, variance);
        
        std::string value = metric->GetValueAsString();
        EXPECT_TRUE(hasValidFormat(value)) << "Phase " << phase << " failed";
        
        std::vector<double> percentiles = extractPercentileValues(value);
        ASSERT_EQ(percentiles.size(), 4) << "Phase " << phase << " percentiles count";
        
        EXPECT_LE(percentiles[0], percentiles[1]) << "Phase " << phase << " P90 <= P95";
        EXPECT_LE(percentiles[1], percentiles[2]) << "Phase " << phase << " P95 <= P99";
        EXPECT_LE(percentiles[2], percentiles[3]) << "Phase " << phase << " P99 <= P999";
        
        EXPECT_EQ("\"Percentile Latency\"", metric->GetName()) << "Phase " << phase << " name consistency";
        
        if (phase % 2 == 0) {
            metric->Reset();
        }
    }
    
    std::string final_value = metric->GetValueAsString();
    EXPECT_TRUE(hasValidFormat(final_value));
}
