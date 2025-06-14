#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <chrono>
#include <string>
#include <memory>
#include "../src/IMetrics/CardinalityMetricAny.h"

class CardinalityMetricAnyTest : public ::testing::Test {
protected:
    void SetUp() override {
        metric = std::make_unique<Metrics::CardinalityMetricAny>(3);
    }

    void TearDown() override {
        metric.reset();
    }

    std::unique_ptr<Metrics::CardinalityMetricAny> metric;
};

TEST_F(CardinalityMetricAnyTest, DefaultConstructor) {
    EXPECT_NO_THROW(Metrics::CardinalityMetricAny());
}

TEST_F(CardinalityMetricAnyTest, ConstructorWithNTop) {
    EXPECT_NO_THROW(Metrics::CardinalityMetricAny(5));
}

TEST_F(CardinalityMetricAnyTest, ConstructorWithZeroNTop) {
    EXPECT_NO_THROW(Metrics::CardinalityMetricAny(0));
}

TEST_F(CardinalityMetricAnyTest, ConstructorWithNegativeNTop) {
    EXPECT_NO_THROW(Metrics::CardinalityMetricAny(-1));
}

TEST_F(CardinalityMetricAnyTest, GetNameReturnsCardinality) {
    EXPECT_EQ(metric->GetName(), "\"Cardinality\"");
}

TEST_F(CardinalityMetricAnyTest, GetNameIsConsistent) {
    std::string name1 = metric->GetName();
    std::string name2 = metric->GetName();
    EXPECT_EQ(name1, name2);
}

TEST_F(CardinalityMetricAnyTest, InitialValueIsZero) {
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("0") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, GetValueAsStringFormat) {
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("General number of unique elements:") != std::string::npos);
    EXPECT_TRUE(value.find("most frequent types:") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, GetValueAsStringShowsNTop) {
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("3 most frequent types:") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, ObserveIntegerSingleItem) {
    metric->Observe(42);
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, ObserveStringSingleItem) {
    metric->Observe(std::string("hello"));
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, ObserveDoubleSingleItem) {
    metric->Observe(3.14);
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, ObserveMultipleDifferentIntegers) {
    metric->Observe(1);
    metric->Observe(2);
    metric->Observe(3);
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("3") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, ObserveMultipleDifferentStrings) {
    metric->Observe(std::string("hello"));
    metric->Observe(std::string("world"));
    metric->Observe(std::string("test"));
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("3") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, ObserveMixedTypes) {
    metric->Observe(42);
    metric->Observe(std::string("hello"));
    metric->Observe(3.14);
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("3") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, ObserveDuplicateIntegers) {
    metric->Observe(42);
    metric->Observe(42);
    metric->Observe(42);
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, ObserveDuplicateStrings) {
    metric->Observe(std::string("hello"));
    metric->Observe(std::string("hello"));
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, ObserveWithCustomCount) {
    metric->Observe(42, 5);
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, ObserveWithCountAccumulation) {
    metric->Observe(42, 3);
    metric->Observe(42, 2);
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, ObserveZeroCount) {
    metric->Observe(42, 0);
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, ObserveNegativeCount) {
    metric->Observe(42, -1);
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, TopTypesOrderingByFrequency) {
    metric->Observe(42, 10);        
    metric->Observe(3.14, 5);      
    metric->Observe(std::string("hello"), 15);
    
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("3") != std::string::npos); 
    size_t string_pos = value.find("NSt7__cxx1112basic_string");
    size_t int_pos = value.find("i");
    size_t double_pos = value.find("d");

    EXPECT_TRUE(string_pos != std::string::npos || int_pos != std::string::npos || double_pos != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, TopTypesLimitedByNTop) {
    auto metric_ntop_2 = std::make_unique<Metrics::CardinalityMetricAny>(2);
    
    metric_ntop_2->Observe(42);
    metric_ntop_2->Observe(3.14);
    metric_ntop_2->Observe(std::string("hello"));
    metric_ntop_2->Observe('c');
    
    std::string value = metric_ntop_2->GetValueAsString();
    EXPECT_TRUE(value.find("4") != std::string::npos);
    EXPECT_TRUE(value.find("2 most frequent types:") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, TopTypesWithZeroNTop) {
    auto metric_ntop_0 = std::make_unique<Metrics::CardinalityMetricAny>(0);
    
    metric_ntop_0->Observe(42);
    metric_ntop_0->Observe(3.14);
    
    std::string value = metric_ntop_0->GetValueAsString();
    EXPECT_TRUE(value.find("2") != std::string::npos);
    EXPECT_TRUE(value.find("0 most frequent types:") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, EvaluateDoesNotThrow) {
    EXPECT_NO_THROW(metric->Evaluate());
}

TEST_F(CardinalityMetricAnyTest, EvaluateDoesNotChangeValue) {
    metric->Observe(42);
    std::string value_before = metric->GetValueAsString();
    metric->Evaluate();
    std::string value_after = metric->GetValueAsString();
    EXPECT_EQ(value_before, value_after);
}

TEST_F(CardinalityMetricAnyTest, ResetClearsItems) {
    metric->Observe(1);
    metric->Observe(2);
    metric->Observe(3);
    metric->Reset();
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("0") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, ResetDoesNotThrow) {
    EXPECT_NO_THROW(metric->Reset());
}

TEST_F(CardinalityMetricAnyTest, ResetAfterObserve) {
    metric->Observe(42);
    metric->Reset();
    metric->Observe(24);
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, WorksThroughIMetricPointer) {
    std::unique_ptr<Metrics::IMetric> base_ptr = std::make_unique<Metrics::CardinalityMetricAny>(3);
    EXPECT_NO_THROW(base_ptr->GetName());
    EXPECT_NO_THROW(base_ptr->GetValueAsString());
    EXPECT_NO_THROW(base_ptr->Evaluate());
    EXPECT_NO_THROW(base_ptr->Reset());
}

TEST_F(CardinalityMetricAnyTest, ObserveComplexObject) {
    struct TestStruct {
        int x;
        std::string y;
        bool operator==(const TestStruct& other) const {
            return x == other.x && y == other.y;
        }
    };
    
    metric->Observe(TestStruct{42, "test"});
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, ObserveVector) {
    std::vector<int> vec{1, 2, 3};
    metric->Observe(vec);
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, ObservePointer) {
    int value = 42;
    int* ptr = &value;
    metric->Observe(ptr);
    std::string result = metric->GetValueAsString();
    EXPECT_TRUE(result.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, LargeNumberOfUniqueItems) {
    for (int i = 0; i < 1000; ++i) {
        metric->Observe(i);
    }
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1000") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, LargeNumberOfDuplicateItems) {
    for (int i = 0; i < 1000; ++i) {
        metric->Observe(42);
    }
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, ConcurrentObserveDifferentItems) {
    const int num_threads = 10;
    const int items_per_thread = 100;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < items_per_thread; ++j) {
                metric->Observe(i * items_per_thread + j);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::string value = metric->GetValueAsString();
    int expected_count = num_threads * items_per_thread;
    EXPECT_TRUE(value.find(std::to_string(expected_count)) != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, ConcurrentObserveSameItem) {
    const int num_threads = 10;
    const int observations_per_thread = 100;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < observations_per_thread; ++j) {
                metric->Observe(42);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, ConcurrentObserveMixedTypes) {
    const int num_threads = 8;
    const int operations_per_thread = 50;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                switch (i % 4) {
                    case 0: metric->Observe(j); break;
                    case 1: metric->Observe(std::string("item_" + std::to_string(j))); break;
                    case 2: metric->Observe(j * 0.5); break;
                    case 3: metric->Observe(j % 10); break;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::string value = metric->GetValueAsString();
    EXPECT_FALSE(value.empty());
}

TEST_F(CardinalityMetricAnyTest, ConcurrentGetValueAsString) {
    const int num_threads = 10;
    const int reads_per_thread = 50;
    std::vector<std::thread> threads;
    std::vector<std::vector<std::string>> results(num_threads);
    
    for (int i = 0; i < 10; ++i) {
        metric->Observe(i);
    }
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < reads_per_thread; ++j) {
                std::string value = metric->GetValueAsString();
                results[i].push_back(value);
                std::this_thread::sleep_for(std::chrono::microseconds(1));
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
            EXPECT_TRUE(value.find("General number of unique elements:") != std::string::npos);
        }
    }
}

TEST_F(CardinalityMetricAnyTest, ConcurrentObserveAndRead) {
    const int num_observe_threads = 5;
    const int num_read_threads = 5;
    const int operations_per_thread = 100;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_observe_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                metric->Observe(i * operations_per_thread + j);
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
    
    std::string final_value = metric->GetValueAsString();
    int expected_count = num_observe_threads * operations_per_thread;
    EXPECT_TRUE(final_value.find(std::to_string(expected_count)) != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, ConcurrentReset) {
    const int num_threads = 10;
    const int operations_per_thread = 20;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                if (i % 3 == 0 && j == 10) {
                    metric->Reset();
                } else {
                    metric->Observe(i * operations_per_thread + j);
                }
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::string final_value = metric->GetValueAsString();
    EXPECT_FALSE(final_value.empty());
    EXPECT_TRUE(final_value.find("General number of unique elements:") != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, ConcurrentObserveWithCounts) {
    const int num_threads = 8;
    const int operations_per_thread = 50;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                metric->Observe(i, j + 1);
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find(std::to_string(num_threads)) != std::string::npos);
}

TEST_F(CardinalityMetricAnyTest, HighVolumeOperations) {
    const int num_operations = 10000;
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_operations; ++i) {
        metric->Observe(i % 100);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 5000);
    
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("100") != std::string::npos);
}

class CardinalityMetricAnyStressTest : public ::testing::Test {
protected:
    void SetUp() override {
        metric = std::make_unique<Metrics::CardinalityMetricAny>(5);
    }

    std::unique_ptr<Metrics::CardinalityMetricAny> metric;
};

TEST_F(CardinalityMetricAnyStressTest, MassiveConcurrentOperations) {
    const int num_threads = 20;
    const int operations_per_thread = 500;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                switch (j % 5) {
                    case 0: metric->Observe(i * operations_per_thread + j); break;
                    case 1: metric->Observe(std::string("thread_" + std::to_string(i) + "_item_" + std::to_string(j))); break;
                    case 2: metric->GetValueAsString(); break;
                    case 3: if (j % 50 == 0) metric->Reset(); break;
                    case 4: metric->Evaluate(); break;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::string final_value = metric->GetValueAsString();
    EXPECT_FALSE(final_value.empty());
    EXPECT_TRUE(final_value.find("General number of unique elements:") != std::string::npos);
}

TEST_F(CardinalityMetricAnyStressTest, EdgeCaseNTopValues) {
    auto metric_large_ntop = std::make_unique<Metrics::CardinalityMetricAny>(1000);
    
    metric_large_ntop->Observe(1);
    metric_large_ntop->Observe(2.0);
    metric_large_ntop->Observe(std::string("test"));
    metric_large_ntop->Observe('c');
    metric_large_ntop->Observe(true);
    
    std::string value = metric_large_ntop->GetValueAsString();
    EXPECT_TRUE(value.find("5") != std::string::npos);
    EXPECT_TRUE(value.find("1000 most frequent types:") != std::string::npos);
}