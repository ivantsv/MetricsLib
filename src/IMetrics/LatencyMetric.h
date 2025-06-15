#pragma once

#include "IMetrics.h"

#include <hdr/hdr_histogram.h>
#include <chrono>
#include <mutex>

namespace Metrics {
 class LatencyMetric final : public IMetric, public MetricTags::ComputerMetricTag {
 public:
    LatencyMetric();
    ~LatencyMetric() override;
    std::string GetName() const noexcept override;
    std::string GetValueAsString() const override;
    void Evaluate() override;
    void Reset() override;
    
    void Observe(std::chrono::nanoseconds latency);
         
 private:
    hdr_histogram* histogram_;
    mutable std::mutex mutex_;
 };
} 