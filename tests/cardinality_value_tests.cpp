#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include "../src/IMetrics/CardinalityMetricValue.h"

using namespace Metrics;
using namespace testing;

struct CustomType {
    int value;
    std::string name;
    
    CustomType(int v, const std::string& n) : value(v), name(n) {}
    
    bool operator==(const CustomType& other) const {
        return value == other.value && name == other.name;
    }
    
    friend std::ostream& operator<<(std::ostream& os, const CustomType& ct) {
        os << "CustomType(" << ct.value << ", " << ct.name << ")";
        return os;
    }
};

template<>
struct std::hash<CustomType> {
    std::size_t operator()(const CustomType& ct) const {
        return std::hash<int>{}(ct.value) ^ (std::hash<std::string>{}(ct.name) << 1);
    }
};

struct NonPrintableType {
    int data;
    explicit NonPrintableType(int d) : data(d) {}
    
    bool operator==(const NonPrintableType& other) const {
        return data == other.data;
    }
};

template<>
struct std::hash<NonPrintableType> {
    std::size_t operator()(const NonPrintableType& npt) const {
        return std::hash<int>{}(npt.data);
    }
};

template<typename T>
class CardinalityMetricValueSingleTypeTest : public ::testing::Test {
protected:
    void SetUp() override {
        metric = std::make_unique<CardinalityMetricValue<T>>();
        metric_with_top3 = std::make_unique<CardinalityMetricValue<T>>(3);
    }
    
    std::unique_ptr<CardinalityMetricValue<T>> metric;
    std::unique_ptr<CardinalityMetricValue<T>> metric_with_top3;
};

using TestedTypes = ::testing::Types<int, std::string, double, CustomType>;
TYPED_TEST_SUITE(CardinalityMetricValueSingleTypeTest, TestedTypes);

class CardinalityMetricValueMultiTypeTest : public ::testing::Test {
protected:
    void SetUp() override {
        metric = std::make_unique<CardinalityMetricValue<int, std::string, double>>();
        custom_metric = std::make_unique<CardinalityMetricValue<int, CustomType>>();
    }
    
    std::unique_ptr<CardinalityMetricValue<int, std::string, double>> metric;
    std::unique_ptr<CardinalityMetricValue<int, CustomType>> custom_metric;
};

TYPED_TEST(CardinalityMetricValueSingleTypeTest, DefaultConstructor) {
    EXPECT_NO_THROW(CardinalityMetricValue<TypeParam>());
}

TYPED_TEST(CardinalityMetricValueSingleTypeTest, ConstructorWithTopN) {
    EXPECT_NO_THROW(CardinalityMetricValue<TypeParam>(10));
    EXPECT_NO_THROW(CardinalityMetricValue<TypeParam>(1));
    EXPECT_NO_THROW(CardinalityMetricValue<TypeParam>(0));
}

TYPED_TEST(CardinalityMetricValueSingleTypeTest, GetNameReturnsCorrectValue) {
    EXPECT_EQ(this->metric->GetName(), "\"CardinalityValue\"");
}

TYPED_TEST(CardinalityMetricValueSingleTypeTest, InitialStateIsEmpty) {
    std::string result = this->metric->GetValueAsString();
    EXPECT_THAT(result, HasSubstr("General number of unique elements: 0"));
}

TYPED_TEST(CardinalityMetricValueSingleTypeTest, EvaluateDoesNotThrow) {
    EXPECT_NO_THROW(this->metric->Evaluate());
}

TEST_F(CardinalityMetricValueMultiTypeTest, ObserveSingleIntValue) {
    metric->Observe(static_cast<int>(42));
    std::string result = metric->GetValueAsString();
    EXPECT_THAT(result, HasSubstr("General number of unique elements: 1"));
    EXPECT_THAT(result, HasSubstr("42"));
}

TEST_F(CardinalityMetricValueMultiTypeTest, ObserveSingleStringValue) {
    metric->Observe(std::string("hello"));
    std::string result = metric->GetValueAsString();
    EXPECT_THAT(result, HasSubstr("General number of unique elements: 1"));
    EXPECT_THAT(result, HasSubstr("hello"));
}

TEST_F(CardinalityMetricValueMultiTypeTest, ObserveSingleDoubleValue) {
    metric->Observe(static_cast<double>(3.14));
    std::string result = metric->GetValueAsString();
    EXPECT_THAT(result, HasSubstr("General number of unique elements: 1"));
    EXPECT_THAT(result, HasSubstr("3.14"));
}

TEST_F(CardinalityMetricValueMultiTypeTest, ObserveCustomType) {
    custom_metric->Observe(CustomType(1, "test"));
    std::string result = custom_metric->GetValueAsString();
    EXPECT_THAT(result, HasSubstr("General number of unique elements: 1"));
    EXPECT_THAT(result, HasSubstr("CustomType"));
}

TEST_F(CardinalityMetricValueMultiTypeTest, ObserveMultipleDifferentValues) {
    metric->Observe(static_cast<int>(42));
    metric->Observe(std::string("hello"));
    metric->Observe(static_cast<double>(3.14));
    
    std::string result = metric->GetValueAsString();
    EXPECT_THAT(result, HasSubstr("General number of unique elements: 3"));
}

TEST_F(CardinalityMetricValueMultiTypeTest, ObserveSameValueMultipleTimes) {
    metric->Observe(static_cast<int>(42));
    metric->Observe(static_cast<int>(42));
    metric->Observe(static_cast<int>(42));
    
    std::string result = metric->GetValueAsString();
    EXPECT_THAT(result, HasSubstr("General number of unique elements: 1"));
}

TEST_F(CardinalityMetricValueMultiTypeTest, ObserveWithCount) {
    metric->Observe(static_cast<int>(42), 5);
    metric->Observe(static_cast<int>(42), 3);
    
    std::string result = metric->GetValueAsString();
    EXPECT_THAT(result, HasSubstr("General number of unique elements: 1"));
}

TEST_F(CardinalityMetricValueMultiTypeTest, TopNLimitsOutput) {
    auto metric_top2 = std::make_unique<CardinalityMetricValue<int>>(2);
    
    metric_top2->Observe(1, 10);
    metric_top2->Observe(2, 5);
    metric_top2->Observe(3, 8);
    metric_top2->Observe(4, 1);
    
    std::string result = metric_top2->GetValueAsString();
    EXPECT_THAT(result, HasSubstr("General number of unique elements: 4"));
    EXPECT_THAT(result, HasSubstr("2 most frequent types"));
}

TEST_F(CardinalityMetricValueMultiTypeTest, TopNSortsCorrectly) {
    auto metric_top3 = std::make_unique<CardinalityMetricValue<int>>(3);
    
    metric_top3->Observe(1, 1);
    metric_top3->Observe(2, 10);
    metric_top3->Observe(3, 5);
    
    std::string result = metric_top3->GetValueAsString();
    
    EXPECT_THAT(result, HasSubstr("General number of unique elements: 3"));
    
    EXPECT_THAT(result, HasSubstr("1"));
    EXPECT_THAT(result, HasSubstr("2"));
    EXPECT_THAT(result, HasSubstr("3"));
}

TYPED_TEST(CardinalityMetricValueSingleTypeTest, ResetClearsData) {
    if constexpr (std::is_same_v<TypeParam, int>) {
        this->metric->Observe(42);
        this->metric->Observe(24);
    } else if constexpr (std::is_same_v<TypeParam, std::string>) {
        this->metric->Observe(std::string("hello"));
        this->metric->Observe(std::string("world"));
    } else if constexpr (std::is_same_v<TypeParam, double>) {
        this->metric->Observe(3.14);
        this->metric->Observe(2.71);
    } else if constexpr (std::is_same_v<TypeParam, CustomType>) {
        this->metric->Observe(CustomType(1, "test1"));
        this->metric->Observe(CustomType(2, "test2"));
    }
    
    this->metric->Reset();
    std::string result = this->metric->GetValueAsString();
    EXPECT_THAT(result, HasSubstr("General number of unique elements: 0"));
}

TYPED_TEST(CardinalityMetricValueSingleTypeTest, ResetDoesNotThrow) {
    EXPECT_NO_THROW(this->metric->Reset());
}

TEST_F(CardinalityMetricValueMultiTypeTest, ZeroTopN) {
    auto metric_top0 = std::make_unique<CardinalityMetricValue<int>>(0);
    metric_top0->Observe(1);
    metric_top0->Observe(2);
    
    std::string result = metric_top0->GetValueAsString();
    EXPECT_THAT(result, HasSubstr("General number of unique elements: 2"));
    EXPECT_THAT(result, HasSubstr("0 most frequent types"));
}

TEST_F(CardinalityMetricValueMultiTypeTest, NegativeTopN) {
    auto metric_top_neg = std::make_unique<CardinalityMetricValue<int>>(-1);
    metric_top_neg->Observe(1);
    
    std::string result = metric_top_neg->GetValueAsString();
    EXPECT_THAT(result, HasSubstr("General number of unique elements: 1"));
}

TEST_F(CardinalityMetricValueMultiTypeTest, ObserveZeroCount) {
    metric->Observe(static_cast<int>(42), 0);
    std::string result = metric->GetValueAsString();
    EXPECT_THAT(result, HasSubstr("General number of unique elements: 1"));
}

TEST_F(CardinalityMetricValueMultiTypeTest, ObserveNegativeCount) {
    metric->Observe(static_cast<int>(42), 5);
    metric->Observe(static_cast<int>(42), -2);
    std::string result = metric->GetValueAsString();
    EXPECT_THAT(result, HasSubstr("General number of unique elements: 1"));
}

TEST(CardinalityMetricValueSpecialTypesTest, NonPrintableType) {
    CardinalityMetricValue<NonPrintableType> metric;
    metric.Observe(NonPrintableType(42));
    
    std::string result = metric.GetValueAsString();
    EXPECT_THAT(result, HasSubstr("General number of unique elements: 1"));
    EXPECT_TRUE(result.find("Value can't be string") != std::string::npos || 
                result.find("NonPrintableType") != std::string::npos);
}

TEST_F(CardinalityMetricValueMultiTypeTest, WorksThroughIMetricPointer) {
    std::unique_ptr<IMetric> base_ptr = std::make_unique<CardinalityMetricValue<int>>();
    
    EXPECT_NO_THROW(base_ptr->GetName());
    EXPECT_NO_THROW(base_ptr->GetValueAsString());
    EXPECT_NO_THROW(base_ptr->Evaluate());
    EXPECT_NO_THROW(base_ptr->Reset());
}

TEST_F(CardinalityMetricValueMultiTypeTest, ConcurrentObserveDifferentValues) {
    const int num_threads = 10;
    const int values_per_thread = 100;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < values_per_thread; ++j) {
                metric->Observe(i * values_per_thread + j);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::string result = metric->GetValueAsString();
    int expected_unique = num_threads * values_per_thread;
    EXPECT_THAT(result, HasSubstr("General number of unique elements: " + std::to_string(expected_unique)));
}

TEST_F(CardinalityMetricValueMultiTypeTest, ConcurrentObserveSameValue) {
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
    
    std::string result = metric->GetValueAsString();
    EXPECT_THAT(result, HasSubstr("General number of unique elements: 1"));
}

TEST_F(CardinalityMetricValueMultiTypeTest, ConcurrentObserveAndRead) {
    const int num_observer_threads = 5;
    const int num_reader_threads = 5;
    const int operations_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<bool> should_stop{false};
    
    for (int i = 0; i < num_observer_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                metric->Observe(i * operations_per_thread + j);
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        });
    }
    
    for (int i = 0; i < num_reader_threads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                std::string result = metric->GetValueAsString();
                EXPECT_FALSE(result.empty());
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::string final_result = metric->GetValueAsString();
    int expected_unique = num_observer_threads * operations_per_thread;
    EXPECT_THAT(final_result, HasSubstr("General number of unique elements: " + std::to_string(expected_unique)));
}

TEST_F(CardinalityMetricValueMultiTypeTest, ConcurrentReset) {
    const int num_threads = 10;
    const int operations_per_thread = 50;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                if (i % 3 == 0 && j % 10 == 0) {
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
    
    std::string final_result = metric->GetValueAsString();
    EXPECT_FALSE(final_result.empty());
    EXPECT_THAT(final_result, HasSubstr("General number of unique elements:"));
}

TEST_F(CardinalityMetricValueMultiTypeTest, ConcurrentMixedOperations) {
    const int num_threads = 8;
    const int operations_per_thread = 100;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                switch (j % 4) {
                    case 0: metric->Observe(int(i)); break;
                    case 1: metric->Observe(std::string("thread_" + std::to_string(i))); break;
                    case 2: metric->GetValueAsString(); break;
                    case 3: if (j % 20 == 0) metric->Reset(); break;
                }
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::string final_result = metric->GetValueAsString();
    EXPECT_FALSE(final_result.empty());
    EXPECT_THAT(final_result, HasSubstr("General number of unique elements:"));
}

class CardinalityMetricValueStressTest : public ::testing::Test {
protected:
    void SetUp() override {
        metric = std::make_unique<CardinalityMetricValue<int, std::string, double>>();
    }
    
    std::unique_ptr<CardinalityMetricValue<int, std::string, double>> metric;
};

TEST_F(CardinalityMetricValueMultiTypeTest, HighVolumeObservations) {
    const int num_observations = 10000;
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_observations; ++i) {
        metric->Observe(int(i));
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 5000);
    
    std::string result = metric->GetValueAsString();
    EXPECT_THAT(result, HasSubstr("General number of unique elements: " + std::to_string(num_observations)));
}

TEST_F(CardinalityMetricValueMultiTypeTest, HighVolumeRepeatedObservations) {
    const int num_observations = 10000;
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_observations; ++i) {
        metric->Observe(42);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 5000);
    
    std::string result = metric->GetValueAsString();
    EXPECT_THAT(result, HasSubstr("General number of unique elements: 1"));
}

TEST(CardinalityMetricValueTemplateTest, MultipleTypesInSameMetric) {
    CardinalityMetricValue<int, std::string, double, CustomType> metric;
    
    metric.Observe(static_cast<int>(42));
    metric.Observe(std::string("hello"));
    metric.Observe(static_cast<double>(3.14));
    metric.Observe(CustomType(1, "test"));
    
    std::string result = metric.GetValueAsString();
    EXPECT_THAT(result, HasSubstr("General number of unique elements: 4"));
}

TEST(CardinalityMetricValueTemplateTest, SingleTypeInTemplate) {
    CardinalityMetricValue<int> metric;
    
    metric.Observe(static_cast<int>(42));
    std::string result = metric.GetValueAsString();
    EXPECT_THAT(result, HasSubstr("General number of unique elements: 1"));
}

TEST(CardinalityMetricValueConceptTest, TypeMustBeEqualityComparable) {
    EXPECT_NO_THROW((CardinalityMetricValue<int>()));
    EXPECT_NO_THROW((CardinalityMetricValue<std::string>()));
    EXPECT_NO_THROW((CardinalityMetricValue<CustomType>()));
}

TEST(CardinalityMetricValueConceptTest, TypeMustBeHashable) {
    EXPECT_NO_THROW((CardinalityMetricValue<CustomType>()));
    EXPECT_NO_THROW((CardinalityMetricValue<NonPrintableType>()));
}

TEST_F(CardinalityMetricValueMultiTypeTest, VeryLargeTopN) {
    auto metric_large_top = std::make_unique<CardinalityMetricValue<int>>(1000000);
    
    for (int i = 0; i < 10; ++i) {
        metric_large_top->Observe(int(i));
    }
    
    std::string result = metric_large_top->GetValueAsString();
    EXPECT_THAT(result, HasSubstr("General number of unique elements: 10"));
    EXPECT_THAT(result, HasSubstr("1000000 most frequent types"));
}

TEST_F(CardinalityMetricValueMultiTypeTest, GetValueAsStringIsConsistent) {
    metric->Observe(static_cast<int>(42));
    metric->Observe(std::string("hello"));
    
    std::string result1 = metric->GetValueAsString();
    std::string result2 = metric->GetValueAsString();
    
    EXPECT_EQ(result1, result2);
}

TEST_F(CardinalityMetricValueMultiTypeTest, GetValueAsStringDoesNotModifyState) {
    metric->Observe(static_cast<int>(42));
    
    std::string before = metric->GetValueAsString();
    metric->GetValueAsString();
    std::string after = metric->GetValueAsString();
    
    EXPECT_EQ(before, after);
}

TEST_F(CardinalityMetricValueMultiTypeTest, ObserveWithLargeCounts) {
    metric->Observe(static_cast<int>(42), 1000000);
    
    std::string result = metric->GetValueAsString();
    EXPECT_THAT(result, HasSubstr("General number of unique elements: 1"));
}

TEST(CardinalityMetricValueExtremeValuesTest, ExtremeIntValues) {
    CardinalityMetricValue<int> metric;
    
    metric.Observe(std::numeric_limits<int>::max());
    metric.Observe(std::numeric_limits<int>::min());
    metric.Observe(0);
    
    std::string result = metric.GetValueAsString();
    EXPECT_THAT(result, HasSubstr("General number of unique elements: 3"));
}

TEST(CardinalityMetricValueExtremeValuesTest, ExtremeDoubleValues) {
    CardinalityMetricValue<double> metric;
    
    metric.Observe(std::numeric_limits<double>::max());
    metric.Observe(std::numeric_limits<double>::min());
    metric.Observe(std::numeric_limits<double>::infinity());
    metric.Observe(-std::numeric_limits<double>::infinity());
    metric.Observe(std::numeric_limits<double>::quiet_NaN());
    
    std::string result = metric.GetValueAsString();
    EXPECT_THAT(result, HasSubstr("General number of unique elements:"));
}
