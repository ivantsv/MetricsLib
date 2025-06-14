#pragma once

#include "IMetrics.h"

#include <atomic>

namespace Metrics {
    class HTTPSIncomeMetric final : public IMetric {
    private:
        std::atomic<unsigned long long> counter_;
        std::atomic<double> current_rps_value_;
        std::atomic<unsigned long long> last_evaluated_counter_;
        
    public:
        HTTPSIncomeMetric(unsigned long long start=0);
        std::string getName() const noexcept override;
        std::string getValueAsString() const override;
        void evaluate() noexcept override;
        void reset() noexcept override;
        
        HTTPSIncomeMetric& operator++(int) noexcept;
        HTTPSIncomeMetric& operator++() noexcept;
    };   
}