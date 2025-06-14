#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <set>
#include <memory>
#include "../src/IMetrics/IncrementMetric.h"

class IncrementMetricTest : public ::testing::Test {
protected:
    void SetUp() override {
        metric = std::make_unique<Metrics::IncrementMetric>();
    }

    void TearDown() override {
        metric.reset();
    }

    std::unique_ptr<Metrics::IncrementMetric> metric;
};

TEST_F(IncrementMetricTest, DefaultConstructor) {
    EXPECT_NO_THROW(Metrics::IncrementMetric());
}

TEST_F(IncrementMetricTest, NamedConstructor) {
    EXPECT_NO_THROW(Metrics::IncrementMetric("TestMetric"));
}

TEST_F(IncrementMetricTest, ConstructorWithStartValue) {
    EXPECT_NO_THROW(Metrics::IncrementMetric("TestMetric", 100));
}

TEST_F(IncrementMetricTest, DefaultNameContainsIncrement) {
    EXPECT_TRUE(metric->GetName().find("IncrementMetric") != std::string::npos);
}

TEST_F(IncrementMetricTest, NamedMetricReturnsCorrectName) {
    auto named_metric = std::make_unique<Metrics::IncrementMetric>("TestMetric");
    EXPECT_EQ(named_metric->GetName(), "TestMetric");
}

TEST_F(IncrementMetricTest, EmptyNameUsesDefault) {
    auto empty_named = std::make_unique<Metrics::IncrementMetric>();
    EXPECT_TRUE(empty_named->GetName().find("IncrementMetric") != std::string::npos);
}

TEST_F(IncrementMetricTest, GetNameIsConsistent) {
    std::string name1 = metric->GetName();
    std::string name2 = metric->GetName();
    EXPECT_EQ(name1, name2);
}

TEST_F(IncrementMetricTest, InitialValueIsZero) {
    EXPECT_EQ(metric->GetValueAsString(), "0");
}

TEST_F(IncrementMetricTest, InitialValueWithStart) {
    auto metric_with_start = std::make_unique<Metrics::IncrementMetric>("Test", 42);
    EXPECT_EQ(metric_with_start->GetValueAsString(), "42");
}

TEST_F(IncrementMetricTest, PreIncrementWorks) {
    ++(*metric);
    EXPECT_EQ(metric->GetValueAsString(), "1");
}

TEST_F(IncrementMetricTest, PostIncrementWorks) {
    (*metric)++;
    EXPECT_EQ(metric->GetValueAsString(), "1");
}

TEST_F(IncrementMetricTest, PreIncrementReturnsReference) {
    auto& result = ++(*metric);
    EXPECT_EQ(&result, metric.get());
}

TEST_F(IncrementMetricTest, PostIncrementReturnsReference) {
    auto& result = (*metric)++;
    EXPECT_EQ(&result, metric.get());
}

TEST_F(IncrementMetricTest, MultipleIncrements) {
    for (int i = 0; i < 10; ++i) {
        ++(*metric);
    }
    EXPECT_EQ(metric->GetValueAsString(), "10");
}

TEST_F(IncrementMetricTest, MixedIncrements) {
    ++(*metric);
    (*metric)++;
    ++(*metric);
    EXPECT_EQ(metric->GetValueAsString(), "3");
}

TEST_F(IncrementMetricTest, EvaluateDoesNotThrow) {
    EXPECT_NO_THROW(metric->Evaluate());
}

TEST_F(IncrementMetricTest, EvaluateDoesNotChangeValue) {
    ++(*metric);
    std::string value_before = metric->GetValueAsString();
    metric->Evaluate();
    std::string value_after = metric->GetValueAsString();
    EXPECT_EQ(value_before, value_after);
}

TEST_F(IncrementMetricTest, ResetSetsValueToZero) {
    for (int i = 0; i < 5; ++i) {
        ++(*metric);
    }
    metric->Reset();
    EXPECT_EQ(metric->GetValueAsString(), "0");
}

TEST_F(IncrementMetricTest, ResetDoesNotThrow) {
    EXPECT_NO_THROW(metric->Reset());
}

TEST_F(IncrementMetricTest, ResetAfterIncrement) {
    ++(*metric);
    metric->Reset();
    ++(*metric);
    EXPECT_EQ(metric->GetValueAsString(), "1");
}

TEST_F(IncrementMetricTest, GetValueAsStringReturnsNumericString) {
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(std::all_of(value.begin(), value.end(), ::isdigit));
}

TEST_F(IncrementMetricTest, GetValueAsStringIsConsistent) {
    ++(*metric);
    std::string value1 = metric->GetValueAsString();
    std::string value2 = metric->GetValueAsString();
    EXPECT_EQ(value1, value2);
}

TEST_F(IncrementMetricTest, LargeIncrements) {
    for (int i = 0; i < 1000; ++i) {
        ++(*metric);
    }
    EXPECT_EQ(metric->GetValueAsString(), "1000");
}

TEST_F(IncrementMetricTest, WorksThroughIMetricPointer) {
    std::unique_ptr<Metrics::IMetric> base_ptr = std::make_unique<Metrics::IncrementMetric>("Test");
    EXPECT_NO_THROW(base_ptr->GetName());
    EXPECT_NO_THROW(base_ptr->GetValueAsString());
    EXPECT_NO_THROW(base_ptr->Evaluate());
    EXPECT_NO_THROW(base_ptr->Reset());
}

TEST_F(IncrementMetricTest, UniqueDefaultNames) {
    auto metric1 = std::make_unique<Metrics::IncrementMetric>();
    auto metric2 = std::make_unique<Metrics::IncrementMetric>();
    auto metric3 = std::make_unique<Metrics::IncrementMetric>();
    
    std::set<std::string> names;
    names.insert(metric1->GetName());
    names.insert(metric2->GetName());
    names.insert(metric3->GetName());
    
    EXPECT_EQ(names.size(), 3);
}

TEST_F(IncrementMetricTest, ConcurrentIncrements) {
    const int num_threads = 10;
    const int increments_per_thread = 100;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < increments_per_thread; ++j) {
                ++(*metric);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    int expected_value = num_threads * increments_per_thread;
    EXPECT_EQ(metric->GetValueAsString(), std::to_string(expected_value));
}

TEST_F(IncrementMetricTest, ConcurrentMixedIncrements) {
    const int num_threads = 8;
    const int increments_per_thread = 50;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < increments_per_thread; ++j) {
                if (i % 2 == 0) {
                    ++(*metric);
                } else {
                    (*metric)++;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    int expected_value = num_threads * increments_per_thread;
    EXPECT_EQ(metric->GetValueAsString(), std::to_string(expected_value));
}

TEST_F(IncrementMetricTest, ConcurrentGetValueAsString) {
    const int num_threads = 10;
    const int reads_per_thread = 50;
    std::vector<std::thread> threads;
    std::vector<std::vector<std::string>> results(num_threads);
    
    for (int i = 0; i < 100; ++i) {
        ++(*metric);
    }
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < reads_per_thread; ++j) {
                std::string value = metric->GetValueAsString();
                results[i].push_back(value);
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    for (int i = 0; i < num_threads; ++i) {
        EXPECT_EQ(results[i].size(), reads_per_thread);
        for (const auto& value : results[i]) {
            EXPECT_FALSE(value.empty());
            EXPECT_TRUE(std::all_of(value.begin(), value.end(), ::isdigit));
        }
    }
}

TEST_F(IncrementMetricTest, ConcurrentIncrementAndRead) {
    const int num_increment_threads = 5;
    const int num_read_threads = 5;
    const int operations_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<bool> should_stop{false};
    
    for (int i = 0; i < num_increment_threads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                ++(*metric);
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        });
    }
    
    for (int i = 0; i < num_read_threads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                std::string value = metric->GetValueAsString();
                EXPECT_FALSE(value.empty());
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(metric->GetValueAsString(), std::to_string(num_increment_threads * operations_per_thread));
}

TEST_F(IncrementMetricTest, ConcurrentReset) {
    const int num_threads = 10;
    const int operations_per_thread = 20;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                if (i % 3 == 0) {
                    metric->Reset();
                } else {
                    ++(*metric);
                }
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::string final_value = metric->GetValueAsString();
    EXPECT_TRUE(std::all_of(final_value.begin(), final_value.end(), ::isdigit));
}

TEST_F(IncrementMetricTest, ConcurrentNameGeneration) {
    const int num_threads = 10;
    const int metrics_per_thread = 10;
    std::vector<std::thread> threads;
    std::vector<std::vector<std::string>> thread_names(num_threads);
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < metrics_per_thread; ++j) {
                auto metric = std::make_unique<Metrics::IncrementMetric>();
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

TEST_F(IncrementMetricTest, HighVolumeIncrements) {
    const int num_increments = 100000;
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_increments; ++i) {
        ++(*metric);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 1000);
    EXPECT_EQ(metric->GetValueAsString(), std::to_string(num_increments));
}

class IncrementMetricStressTest : public ::testing::Test {
protected:
    void SetUp() override {
        metric = std::make_unique<Metrics::IncrementMetric>();
    }

    std::unique_ptr<Metrics::IncrementMetric> metric;
};

TEST_F(IncrementMetricStressTest, MassiveConcurrentOperations) {
    const int num_threads = 20;
    const int operations_per_thread = 1000;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                switch (j % 4) {
                    case 0: ++(*metric); break;
                    case 1: (*metric)++; break;
                    case 2: metric->GetValueAsString(); break;
                    case 3: if (j % 100 == 0) metric->Reset(); break;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::string final_value = metric->GetValueAsString();
    EXPECT_TRUE(std::all_of(final_value.begin(), final_value.end(), ::isdigit));
}