#pragma once

#include "IMetrics.h"

#include <string>
#include <mutex>
#include <chrono>
#include <atomic>

namespace Metrics {
    class CodeTimeMetric final : public IMetric {
    private:
        static inline std::atomic<unsigned long long> counter = 0;
        static inline std::mutex counter_mutex_;
        static std::string CreateDefaultName();
    public:
        using tag = MetricTags::AlgoMetricTag;
    
        CodeTimeMetric(const std::string& name=CreateDefaultName());
        
        std::string GetName() const noexcept override;
        std::string GetValueAsString() const override;
        void Evaluate() override;
        void Reset() override;
        
        void Start();
        void Stop();
        
    private:
        std::string task_name_;
        std::chrono::high_resolution_clock::time_point start_;
        std::chrono::high_resolution_clock::time_point finish_;
        bool is_running_;
        
        mutable std::mutex mutex_;
    };
}