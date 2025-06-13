#include "IMetrics/IMetrics.h"

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
        metric.reset();
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
    std::string initial_value = metric->getValueAsString();
    EXPECT_TRUE(isValidDoubleString(initial_value));
    EXPECT_TRUE(hasCorrectPrecision(initial_value));
    EXPECT_TRUE(hasValidFormat(initial_value));
}

TEST_F(CPUMetricTest, GetNameReturnsCPU) {
    EXPECT_EQ("CPU", metric->getName());
}

TEST_F(CPUMetricTest, GetNameIsConsistent) {
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ("CPU", metric->getName());
    }
}

TEST_F(CPUMetricTest, GetNameUnchangedAfterEvaluate) {
    metric->evaluate();
    EXPECT_EQ("CPU", metric->getName());
}

TEST_F(CPUMetricTest, GetNameUnchangedAfterReset) {
    metric->reset();
    EXPECT_EQ("CPU", metric->getName());
}

TEST_F(CPUMetricTest, GetValueAsStringReturnsValidDouble) {
    std::string value_str = metric->getValueAsString();
    EXPECT_TRUE(isValidDoubleString(value_str));
}

TEST_F(CPUMetricTest, GetValueAsStringHasCorrectPrecision) {
    std::string value_str = metric->getValueAsString();
    EXPECT_TRUE(hasCorrectPrecision(value_str));
}

TEST_F(CPUMetricTest, GetValueAsStringHasValidFormat) {
    std::string value_str = metric->getValueAsString();
    EXPECT_TRUE(hasValidFormat(value_str));
}

TEST_F(CPUMetricTest, GetValueAsStringReturnsNonNegative) {
    std::string value_str = metric->getValueAsString();
    double value = std::stod(value_str);
    EXPECT_GE(value, 0.0);
}

TEST_F(CPUMetricTest, GetValueAsStringMatchesExpectedFormat) {
    std::string value_str = metric->getValueAsString();
    double value = std::stod(value_str);
    
    std::ostringstream expected_format;
    expected_format << std::fixed << std::setprecision(2) << value;
    EXPECT_EQ(expected_format.str(), value_str);
}

TEST_F(CPUMetricTest, EvaluateDoesNotThrow) {
    EXPECT_NO_THROW(metric->evaluate());
}

TEST_F(CPUMetricTest, EvaluateProducesValidValue) {
    metric->evaluate();
    std::string value_after_evaluate = metric->getValueAsString();
    EXPECT_TRUE(isValidDoubleString(value_after_evaluate));
    EXPECT_TRUE(hasCorrectPrecision(value_after_evaluate));
}

TEST_F(CPUMetricTest, MultipleEvaluationsProduceValidValues) {
    std::vector<std::string> values;
    
    for (int i = 0; i < 5; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        metric->evaluate();
        values.push_back(metric->getValueAsString());
    }
    
    for (size_t i = 0; i < values.size(); ++i) {
        EXPECT_TRUE(isValidDoubleString(values[i])) << "Value " << i << " is not valid double";
        EXPECT_TRUE(hasCorrectPrecision(values[i])) << "Value " << i << " has incorrect precision";
    }
}

TEST_F(CPUMetricTest, RapidEvaluationsDoNotThrow) {
    for (int i = 0; i < 10; ++i) {
        EXPECT_NO_THROW(metric->evaluate()) << "Rapid evaluation " << i << " threw exception";
        std::string value = metric->getValueAsString();
        EXPECT_TRUE(isValidDoubleString(value)) << "Rapid evaluation " << i << " produced invalid value";
    }
}

TEST_F(CPUMetricTest, ResetDoesNotThrow) {
    EXPECT_NO_THROW(metric->reset());
}

TEST_F(CPUMetricTest, ResetSetsValueToZero) {
    metric->evaluate();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    metric->evaluate();

    metric->reset();
    EXPECT_EQ("0.00", metric->getValueAsString());
}

TEST_F(CPUMetricTest, ResetProducesValidValue) {
    metric->reset();
    std::string value_after_reset = metric->getValueAsString();
    EXPECT_TRUE(isValidDoubleString(value_after_reset));
    EXPECT_TRUE(hasCorrectPrecision(value_after_reset));
}

TEST_F(CPUMetricTest, ConsecutiveResetsWork) {
    for (int i = 0; i < 3; ++i) {
        metric->reset();
        EXPECT_EQ("0.00", metric->getValueAsString()) << "Reset " << (i + 1) << " failed";
    }
}

TEST_F(CPUMetricTest, EvaluateAndGetValueIntegration) {
    std::string value_before = metric->getValueAsString();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    metric->evaluate();
    
    std::string value_after = metric->getValueAsString();
    
    EXPECT_TRUE(isValidDoubleString(value_before));
    EXPECT_TRUE(isValidDoubleString(value_after));
    EXPECT_TRUE(hasCorrectPrecision(value_before));
    EXPECT_TRUE(hasCorrectPrecision(value_after));
}

TEST_F(CPUMetricTest, ResetAfterEvaluateWorks) {
    for (int i = 0; i < 3; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        metric->evaluate();
    }

    metric->reset();
    EXPECT_EQ("0.00", metric->getValueAsString());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    metric->evaluate();
    
    std::string value_after_evaluate = metric->getValueAsString();
    EXPECT_TRUE(isValidDoubleString(value_after_evaluate));
}

TEST_F(CPUMetricTest, WorksThroughIMetricPointer) {
    std::unique_ptr<IMetric> metric_ptr = std::make_unique<CPUMetric>();
    
    EXPECT_EQ("CPU", metric_ptr->getName());
    
    std::string value = metric_ptr->getValueAsString();
    EXPECT_TRUE(isValidDoubleString(value));
    
    EXPECT_NO_THROW(metric_ptr->evaluate());
    
    EXPECT_NO_THROW(metric_ptr->reset());
    EXPECT_EQ("0.00", metric_ptr->getValueAsString());
}

TEST_F(CPUMetricTest, CRTPInterfaceWorks) {
    ComputerMetrics<CPUMetric>* crtp_ptr = metric.get();
    EXPECT_NO_THROW(crtp_ptr->evaluate());
    
    IMetric* base_ptr = metric.get();
    EXPECT_NO_THROW(base_ptr->evaluate());
}

TEST_F(CPUMetricTest, EvaluateCompletesInReasonableTime) {
    auto start = std::chrono::high_resolution_clock::now();
    metric->evaluate();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 1000) << "evaluate() took too long: " << duration.count() << "ms";
}

TEST_F(CPUMetricTest, MultipleEvaluationsCompleteInReasonableTime) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 5; ++i) {
        metric->evaluate();
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 5000) << "Multiple evaluations took too long: " << duration.count() << "ms";
}

TEST_F(CPUMetricTest, InheritsFromCorrectBaseClasses) {
    static_assert(std::is_base_of_v<ComputerMetrics<CPUMetric>, CPUMetric>, 
                  "CPUMetric should inherit from ComputerMetrics<CPUMetric>");
    static_assert(std::is_base_of_v<IMetric, CPUMetric>, 
                  "CPUMetric should inherit from IMetric");
}

TEST_F(CPUMetricTest, ValueStringFormatConsistency) {
    std::vector<std::string> scenarios = {"initial", "after_evaluate", "after_reset"};
    
    for (const auto& scenario : scenarios) {
        if (scenario == "after_evaluate") {
            metric->evaluate();
        } else if (scenario == "after_reset") {
            metric->reset();
        }
        
        std::string value_str = metric->getValueAsString();
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
            EXPECT_NO_THROW(metric->reset());
        } else {
            EXPECT_NO_THROW(metric->evaluate());
        }
        
        EXPECT_EQ("CPU", metric->getName());
        
        std::string value = metric->getValueAsString();
        EXPECT_TRUE(std::stod(value) >= 0.0);
    }
}

