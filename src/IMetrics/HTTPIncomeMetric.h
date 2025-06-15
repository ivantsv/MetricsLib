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
        using tag = MetricTags::ServerMetricTag;
    
        HTTPSIncomeMetric(unsigned long long start=0);
        std::string GetName() const noexcept override;
        std::string GetValueAsString() const override;
        void Evaluate() noexcept override;
        void Reset() noexcept override;
        
        HTTPSIncomeMetric& operator++(int) noexcept;
        HTTPSIncomeMetric& operator++() noexcept;
    };   
}