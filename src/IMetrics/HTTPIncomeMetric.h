#pragma once

#include "IMetrics.h"

#include <atomic>

namespace Metrics {
    class HTTPIncomeMetric final : public IMetric, public MetricTags::ServerMetricTag {
    private:
        std::atomic<unsigned long long> counter_;
        std::atomic<double> current_rps_value_;
        std::atomic<unsigned long long> last_evaluated_counter_;
        
    public:
        HTTPIncomeMetric(unsigned long long start=0);
        std::string GetName() const noexcept override;
        std::string GetValueAsString() const override;
        void Evaluate() noexcept override;
        void Reset() noexcept override;
        
        HTTPIncomeMetric& operator++(int) noexcept;
        HTTPIncomeMetric& operator++() noexcept;
    };   
}