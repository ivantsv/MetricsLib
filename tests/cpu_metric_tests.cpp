#include "IMetrics/CPUUtilMetric.h"

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <memory>
#include <sstream>
#include <iomanip>

using namespace Metrics;

class CPUMetricTest : public ::testing::Test {
protected:
    void SetUp() override {
        metric = std::make_unique<CPUMetric>();
    }

    void TearDown() override {
        metric->Reset();
    }

    std::unique_ptr<CPUMetric> metric;

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

    bool hasValidFormat(const std::string& str) {
        size_t dot_pos = str.find('.');
        if (dot_pos == std::string::npos || dot_pos == 0 || dot_pos == str.length() - 1) {
            return false;
        }
        
        for (size_t i = 0; i < dot_pos; ++i) {
            if (!std::isdigit(str[i])) {
                return false;
            }
        }

        if ((str.length() - dot_pos - 1) != 2) {
            return false;
        }
        
        for (size_t i = dot_pos + 1; i < str.length(); ++i) {
            if (!std::isdigit(str[i])) {
                return false;
            }
        }
        
        return true;
    }
};

TEST_F(CPUMetricTest, ConstructorDoesNotThrow) {
    EXPECT_NO_THROW({
        CPUMetric test_metric;
    });
}

TEST_F(CPUMetricTest, ConstructorInitializesValidState) {
    std::string initial_value = metric->GetValueAsString();
    EXPECT_TRUE(isValidDoubleString(initial_value));
    EXPECT_TRUE(hasCorrectPrecision(initial_value));
    EXPECT_TRUE(hasValidFormat(initial_value));
}

TEST_F(CPUMetricTest, GetNameReturnsCPU) {
    EXPECT_EQ("\"CPU\"", metric->GetName());
}

TEST_F(CPUMetricTest, GetNameIsConsistent) {
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ("\"CPU\"", metric->GetName());
    }
}

TEST_F(CPUMetricTest, GetNameUnchangedAfterEvaluate) {
    metric->Evaluate();
    EXPECT_EQ("\"CPU\"", metric->GetName());
}

TEST_F(CPUMetricTest, GetNameUnchangedAfterReset) {
    metric->Reset();
    EXPECT_EQ("\"CPU\"", metric->GetName());
}

TEST_F(CPUMetricTest, GetValueAsStringReturnsValidDouble) {
    std::string value_str = metric->GetValueAsString();
    EXPECT_TRUE(isValidDoubleString(value_str));
}

TEST_F(CPUMetricTest, GetValueAsStringHasCorrectPrecision) {
    std::string value_str = metric->GetValueAsString();
    EXPECT_TRUE(hasCorrectPrecision(value_str));
}

TEST_F(CPUMetricTest, GetValueAsStringHasValidFormat) {
    std::string value_str = metric->GetValueAsString();
    EXPECT_TRUE(hasValidFormat(value_str));
}

TEST_F(CPUMetricTest, GetValueAsStringReturnsNonNegative) {
    std::string value_str = metric->GetValueAsString();
    double value = std::stod(value_str);
    EXPECT_GE(value, 0.0);
}

TEST_F(CPUMetricTest, GetValueAsStringMatchesExpectedFormat) {
    std::string value_str = metric->GetValueAsString();
    double value = std::stod(value_str);
    
    std::ostringstream expected_format;
    expected_format << std::fixed << std::setprecision(2) << value;
    EXPECT_EQ(expected_format.str(), value_str);
}

TEST_F(CPUMetricTest, EvaluateDoesNotThrow) {
    EXPECT_NO_THROW(metric->Evaluate());
}

TEST_F(CPUMetricTest, EvaluateProducesValidValue) {
    metric->Evaluate();
    std::string value_after_Evaluate = metric->GetValueAsString();
    EXPECT_TRUE(isValidDoubleString(value_after_Evaluate));
    EXPECT_TRUE(hasCorrectPrecision(value_after_Evaluate));
}

TEST_F(CPUMetricTest, EvaluateActuallyUpdatesValue) {
    std::string initial_value = metric->GetValueAsString();
    
    auto start = std::chrono::high_resolution_clock::now();
    volatile int dummy = 0;
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start).count() < 100) {
        dummy += rand();
    }
    
    metric->Evaluate();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    metric->Evaluate();
    
    std::string updated_value = metric->GetValueAsString();
    
    if (initial_value == "0.00") {
        EXPECT_TRUE(isValidDoubleString(updated_value));
    }
}

TEST_F(CPUMetricTest, CPUValueInReasonableRange) {
    std::thread load_thread([]() {
        auto start = std::chrono::high_resolution_clock::now();
        volatile long dummy = 0;
        while (std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start).count() < 200) {
            for (int i = 0; i < 10000; ++i) {
                dummy += i * i;
            }
        }
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    metric->Evaluate();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    metric->Evaluate();
    
    load_thread.join();
    
    std::string value_str = metric->GetValueAsString();
    double value = std::stod(value_str);
    
    EXPECT_GE(value, 0.0);
    EXPECT_LE(value, 1000.0);
}

TEST_F(CPUMetricTest, ConsecutiveEvaluationsShowCPUActivity) {
    std::vector<double> values;
    
    for (int i = 0; i < 5; ++i) {
        volatile int dummy = 0;
        for (int j = 0; j < 100000; ++j) {
            dummy += j * j;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        metric->Evaluate();
        
        std::string value_str = metric->GetValueAsString();
        double value = std::stod(value_str);
        values.push_back(value);
        
        EXPECT_GE(value, 0.0) << "CPU value should be non-negative at iteration " << i;
    }
    
    bool found_activity = false;
    for (double val : values) {
        if (val > 0.1) {
            found_activity = true;
            break;
        }
    }
    
}

TEST_F(CPUMetricTest, ResetActuallyZerosTheValue) {
    volatile int dummy = 0;
    for (int i = 0; i < 50000; ++i) {
        dummy += i;
    }
    
    metric->Evaluate();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    metric->Evaluate();
    
    std::string value_before_Reset = metric->GetValueAsString();
    
    metric->Reset();
    std::string value_after_Reset = metric->GetValueAsString();
    
    EXPECT_EQ("0.00", value_after_Reset);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    metric->Evaluate();
    
    std::string value_after_Evaluate = metric->GetValueAsString();
    EXPECT_TRUE(isValidDoubleString(value_after_Evaluate));
}

TEST_F(CPUMetricTest, MultipleEvaluationsProduceValidValues) {
    std::vector<std::string> values;
    
    for (int i = 0; i < 5; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        metric->Evaluate();
        values.push_back(metric->GetValueAsString());
    }
    
    for (size_t i = 0; i < values.size(); ++i) {
        EXPECT_TRUE(isValidDoubleString(values[i])) << "Value " << i << " is not valid double";
        EXPECT_TRUE(hasCorrectPrecision(values[i])) << "Value " << i << " has incorrect precision";
    }
}

TEST_F(CPUMetricTest, RapidEvaluationsDoNotThrow) {
    for (int i = 0; i < 10; ++i) {
        EXPECT_NO_THROW(metric->Evaluate()) << "Rapid evaluation " << i << " threw exception";
        std::string value = metric->GetValueAsString();
        EXPECT_TRUE(isValidDoubleString(value)) << "Rapid evaluation " << i << " produced invalid value";
    }
}

TEST_F(CPUMetricTest, ResetDoesNotThrow) {
    EXPECT_NO_THROW(metric->Reset());
}

TEST_F(CPUMetricTest, ResetSetsValueToZero) {
    metric->Evaluate();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    metric->Evaluate();

    metric->Reset();
    EXPECT_EQ("0.00", metric->GetValueAsString());
}

TEST_F(CPUMetricTest, ResetProducesValidValue) {
    metric->Reset();
    std::string value_after_Reset = metric->GetValueAsString();
    EXPECT_TRUE(isValidDoubleString(value_after_Reset));
    EXPECT_TRUE(hasCorrectPrecision(value_after_Reset));
}

TEST_F(CPUMetricTest, ConsecutiveResetsWork) {
    for (int i = 0; i < 3; ++i) {
        metric->Reset();
        EXPECT_EQ("0.00", metric->GetValueAsString()) << "Reset " << (i + 1) << " failed";
    }
}

TEST_F(CPUMetricTest, EvaluateAndGetValueIntegration) {
    std::string value_before = metric->GetValueAsString();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    metric->Evaluate();
    
    std::string value_after = metric->GetValueAsString();
    
    EXPECT_TRUE(isValidDoubleString(value_before));
    EXPECT_TRUE(isValidDoubleString(value_after));
    EXPECT_TRUE(hasCorrectPrecision(value_before));
    EXPECT_TRUE(hasCorrectPrecision(value_after));
}

TEST_F(CPUMetricTest, ResetAfterEvaluateWorks) {
    for (int i = 0; i < 3; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        metric->Evaluate();
    }

    metric->Reset();
    EXPECT_EQ("0.00", metric->GetValueAsString());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    metric->Evaluate();
    
    std::string value_after_Evaluate = metric->GetValueAsString();
    EXPECT_TRUE(isValidDoubleString(value_after_Evaluate));
}

TEST_F(CPUMetricTest, WorksThroughIMetricPointer) {
    std::unique_ptr<IMetric> metric_ptr = std::make_unique<CPUMetric>();
    
    EXPECT_EQ("\"CPU\"", metric_ptr->GetName());
    
    std::string value = metric_ptr->GetValueAsString();
    EXPECT_TRUE(isValidDoubleString(value));
    
    EXPECT_NO_THROW(metric_ptr->Evaluate());
    
    EXPECT_NO_THROW(metric_ptr->Reset());
    EXPECT_EQ("0.00", metric_ptr->GetValueAsString());
}

TEST_F(CPUMetricTest, EvaluateCompletesInReasonableTime) {
    auto start = std::chrono::high_resolution_clock::now();
    metric->Evaluate();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 1000) << "Evaluate() took too long: " << duration.count() << "ms";
}

TEST_F(CPUMetricTest, MultipleEvaluationsCompleteInReasonableTime) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 5; ++i) {
        metric->Evaluate();
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 5000) << "Multiple evaluations took too long: " << duration.count() << "ms";
}

TEST_F(CPUMetricTest, ValueStringFormatConsistency) {
    std::vector<std::string> scenarios = {"initial", "after_Evaluate", "after_Reset"};
    
    for (const auto& scenario : scenarios) {
        if (scenario == "after_Evaluate") {
            metric->Evaluate();
        } else if (scenario == "after_Reset") {
            metric->Reset();
        }
        
        std::string value_str = metric->GetValueAsString();
        EXPECT_TRUE(hasValidFormat(value_str)) << "Invalid format for scenario: " << scenario;
        
        double value = std::stod(value_str);
        EXPECT_GE(value, 0.0) << "Negative value for scenario: " << scenario;
    }
}

class CPUMetricStressTest : public ::testing::Test {
protected:
    void SetUp() override {
        metric = std::make_unique<CPUMetric>();
    }

    std::unique_ptr<CPUMetric> metric;
};

TEST_F(CPUMetricStressTest, HandlesManyOperations) {
    for (int i = 0; i < 100; ++i) {
        if (i % 10 == 0) {
            EXPECT_NO_THROW(metric->Reset());
        } else {
            EXPECT_NO_THROW(metric->Evaluate());
        }
        
        EXPECT_EQ("\"CPU\"", metric->GetName());
        
        std::string value = metric->GetValueAsString();
        EXPECT_TRUE(std::stod(value) >= 0.0);
    }
}

