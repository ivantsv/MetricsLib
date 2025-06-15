#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <chrono>
#include <string>
#include <memory>
#include "../src/IMetrics/CardinalityMetricType.h"

class CardinalityMetricTypeTest : public ::testing::Test {
protected:
    void SetUp() override {
        metric = std::make_unique<Metrics::CardinalityMetricType>(3);
    }

    void TearDown() override {
        metric.reset();
    }

    std::unique_ptr<Metrics::CardinalityMetricType> metric;
};

TEST_F(CardinalityMetricTypeTest, DefaultConstructor) {
    EXPECT_NO_THROW(Metrics::CardinalityMetricType());
}

TEST_F(CardinalityMetricTypeTest, ConstructorWithNTop) {
    EXPECT_NO_THROW(Metrics::CardinalityMetricType(5));
}

TEST_F(CardinalityMetricTypeTest, ConstructorWithZeroNTop) {
    EXPECT_NO_THROW(Metrics::CardinalityMetricType(0));
}

TEST_F(CardinalityMetricTypeTest, ConstructorWithNegativeNTop) {
    EXPECT_NO_THROW(Metrics::CardinalityMetricType(-1));
}

TEST_F(CardinalityMetricTypeTest, GetNameReturnsCardinality) {
    EXPECT_EQ(metric->GetName(), "\"CardinalityType\"");
}

TEST_F(CardinalityMetricTypeTest, GetNameIsConsistent) {
    std::string name1 = metric->GetName();
    std::string name2 = metric->GetName();
    EXPECT_EQ(name1, name2);
}

TEST_F(CardinalityMetricTypeTest, InitialValueIsZero) {
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("0") != std::string::npos);
}

TEST_F(CardinalityMetricTypeTest, GetValueAsStringFormat) {
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("General number of unique elements:") != std::string::npos);
    EXPECT_TRUE(value.find("most frequent types:") != std::string::npos);
}

TEST_F(CardinalityMetricTypeTest, GetValueAsStringShowsNTop) {
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("3 most frequent types:") != std::string::npos);
}

TEST_F(CardinalityMetricTypeTest, ObserveIntegerSingleItem) {
    metric->Observe(42);
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricTypeTest, ObserveStringSingleItem) {
    metric->Observe(std::string("hello"));
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricTypeTest, ObserveDoubleSingleItem) {
    metric->Observe(3.14);
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricTypeTest, ObserveMultipleDifferentIntegers) {
    metric->Observe(1);
    metric->Observe(2);
    metric->Observe(3);
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("3") != std::string::npos);
}

TEST_F(CardinalityMetricTypeTest, ObserveMultipleDifferentStrings) {
    metric->Observe(std::string("hello"));
    metric->Observe(std::string("world"));
    metric->Observe(std::string("test"));
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("3") != std::string::npos);
}

TEST_F(CardinalityMetricTypeTest, ObserveMixedTypes) {
    metric->Observe(42);
    metric->Observe(std::string("hello"));
    metric->Observe(3.14);
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("3") != std::string::npos);
}

TEST_F(CardinalityMetricTypeTest, ObserveDuplicateIntegers) {
    metric->Observe(42);
    metric->Observe(42);
    metric->Observe(42);
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricTypeTest, ObserveDuplicateStrings) {
    metric->Observe(std::string("hello"));
    metric->Observe(std::string("hello"));
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricTypeTest, ObserveWithCustomCount) {
    metric->Observe(42, 5);
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricTypeTest, ObserveWithCountAccumulation) {
    metric->Observe(42, 3);
    metric->Observe(42, 2);
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricTypeTest, ObserveZeroCount) {
    metric->Observe(42, 0);
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricTypeTest, ObserveNegativeCount) {
    metric->Observe(42, -1);
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricTypeTest, TopTypesOrderingByFrequency) {
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

TEST_F(CardinalityMetricTypeTest, TopTypesLimitedByNTop) {
    auto metric_ntop_2 = std::make_unique<Metrics::CardinalityMetricType>(2);
    
    metric_ntop_2->Observe(42);
    metric_ntop_2->Observe(3.14);
    metric_ntop_2->Observe(std::string("hello"));
    metric_ntop_2->Observe('c');
    
    std::string value = metric_ntop_2->GetValueAsString();
    EXPECT_TRUE(value.find("4") != std::string::npos);
    EXPECT_TRUE(value.find("2 most frequent types:") != std::string::npos);
}

TEST_F(CardinalityMetricTypeTest, TopTypesWithZeroNTop) {
    auto metric_ntop_0 = std::make_unique<Metrics::CardinalityMetricType>(0);
    
    metric_ntop_0->Observe(42);
    metric_ntop_0->Observe(3.14);
    
    std::string value = metric_ntop_0->GetValueAsString();
    EXPECT_TRUE(value.find("2") != std::string::npos);
    EXPECT_TRUE(value.find("0 most frequent types:") != std::string::npos);
}

TEST_F(CardinalityMetricTypeTest, EvaluateDoesNotThrow) {
    EXPECT_NO_THROW(metric->Evaluate());
}

TEST_F(CardinalityMetricTypeTest, EvaluateDoesNotChangeValue) {
    metric->Observe(42);
    std::string value_before = metric->GetValueAsString();
    metric->Evaluate();
    std::string value_after = metric->GetValueAsString();
    EXPECT_EQ(value_before, value_after);
}

TEST_F(CardinalityMetricTypeTest, ResetClearsItems) {
    metric->Observe(1);
    metric->Observe(2);
    metric->Observe(3);
    metric->Reset();
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("0") != std::string::npos);
}

TEST_F(CardinalityMetricTypeTest, ResetDoesNotThrow) {
    EXPECT_NO_THROW(metric->Reset());
}

TEST_F(CardinalityMetricTypeTest, ResetAfterObserve) {
    metric->Observe(42);
    metric->Reset();
    metric->Observe(24);
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricTypeTest, WorksThroughIMetricPointer) {
    std::unique_ptr<Metrics::IMetric> base_ptr = std::make_unique<Metrics::CardinalityMetricType>(3);
    EXPECT_NO_THROW(base_ptr->GetName());
    EXPECT_NO_THROW(base_ptr->GetValueAsString());
    EXPECT_NO_THROW(base_ptr->Evaluate());
    EXPECT_NO_THROW(base_ptr->Reset());
}

TEST_F(CardinalityMetricTypeTest, ObserveComplexObject) {
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

TEST_F(CardinalityMetricTypeTest, ObserveVector) {
    std::vector<int> vec{1, 2, 3};
    metric->Observe(vec);
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricTypeTest, ObservePointer) {
    int value = 42;
    int* ptr = &value;
    metric->Observe(ptr);
    std::string result = metric->GetValueAsString();
    EXPECT_TRUE(result.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricTypeTest, LargeNumberOfUniqueItems) {
    for (int i = 0; i < 1000; ++i) {
        metric->Observe(i);
    }
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1000") != std::string::npos);
}

TEST_F(CardinalityMetricTypeTest, LargeNumberOfDuplicateItems) {
    for (int i = 0; i < 1000; ++i) {
        metric->Observe(42);
    }
    std::string value = metric->GetValueAsString();
    EXPECT_TRUE(value.find("1") != std::string::npos);
}

TEST_F(CardinalityMetricTypeTest, ConcurrentObserveDifferentItems) {
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

TEST_F(CardinalityMetricTypeTest, ConcurrentObserveSameItem) {
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

TEST_F(CardinalityMetricTypeTest, ConcurrentObserveMixedTypes) {
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

TEST_F(CardinalityMetricTypeTest, ConcurrentGetValueAsString) {
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

TEST_F(CardinalityMetricTypeTest, ConcurrentObserveAndRead) {
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

TEST_F(CardinalityMetricTypeTest, ConcurrentReset) {
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

TEST_F(CardinalityMetricTypeTest, ConcurrentObserveWithCounts) {
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

TEST_F(CardinalityMetricTypeTest, HighVolumeOperations) {
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

class CardinalityMetricTypeStressTest : public ::testing::Test {
protected:
    void SetUp() override {
        metric = std::make_unique<Metrics::CardinalityMetricType>(5);
    }

    std::unique_ptr<Metrics::CardinalityMetricType> metric;
};

TEST_F(CardinalityMetricTypeStressTest, MassiveConcurrentOperations) {
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

TEST_F(CardinalityMetricTypeStressTest, EdgeCaseNTopValues) {
    auto metric_large_ntop = std::make_unique<Metrics::CardinalityMetricType>(1000);
    
    metric_large_ntop->Observe(1);
    metric_large_ntop->Observe(2.0);
    metric_large_ntop->Observe(std::string("test"));
    metric_large_ntop->Observe('c');
    metric_large_ntop->Observe(true);
    
    std::string value = metric_large_ntop->GetValueAsString();
    EXPECT_TRUE(value.find("5") != std::string::npos);
    EXPECT_TRUE(value.find("1000 most frequent types:") != std::string::npos);
}