#pragma once

#include "IMetrics.h"

#include <mutex>

namespace Metrics {
    class CPUMetric final : public IMetric {
    private:
        double current_utilization_;
        int cpu_count_;
        mutable std::mutex mutex_;
        
    #ifdef _WIN32
        unsigned long long prev_idle_time_;
        unsigned long long prev_kernel_time_;
        unsigned long long prev_user_time_;
    #else
        unsigned long long prev_idle_;
        unsigned long long prev_total_;
    #endif
        
    public:
        CPUMetric();
        std::string getName() const noexcept override;
        std::string getValueAsString() const override;
        void evaluate() override;
        void reset() override;
        
    private:
        void initializeCPUData();
        double calculateCPUUsage();
        int getCPUCount() noexcept;
    };
}