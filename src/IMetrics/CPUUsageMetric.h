#pragma once

#include "IMetrics.h"

#include <string>
#include <mutex>
#include <chrono> 

struct CPUTimes {
    unsigned long long user = 0;
    unsigned long long nice = 0;
    unsigned long long system = 0;
    unsigned long long idle = 0;
    unsigned long long iowait = 0;
    unsigned long long irq = 0;
    unsigned long long softirq = 0;
    unsigned long long steal = 0;
    unsigned long long guest = 0;
    unsigned long long guest_nice = 0;

    unsigned long long getTotal() const {
        return user + nice + system + idle + iowait + irq + softirq + steal + guest + guest_nice;
    }
};

namespace Metrics {

class CPUUsageMetric final : public IMetric {
public:
    using tag = MetricTags::ComputerMetricTag;

    CPUUsageMetric();

    std::string GetName() const noexcept override;
    std::string GetValueAsString() const override;
    void Evaluate() override;
    void Reset() override;

private:
    double cpu_usage_percent_;
    CPUTimes prev_cpu_times_;
    std::chrono::steady_clock::time_point prev_time_point_;
    mutable std::mutex mutex_;

    bool getCurrentCPUTimes(CPUTimes& times) const;
};

}