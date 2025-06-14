#pragma once

#include "IMetrics.h"

#include <atomic>
#include <mutex>
#include <string>

namespace Metrics {
    class IncrementMetric : public IMetric {
    private:
        static inline std::atomic<unsigned long long> counter = 0;
        static inline std::mutex counter_mutex_;
        static std::string CreateDefaultName();
        
    public:
        IncrementMetric(const std::string& name=CreateDefaultName(), unsigned long long start=0);
        std::string GetName() const noexcept override;
        std::string GetValueAsString() const noexcept override;
        void Evaluate() override;
        void Reset() override;
        
        IncrementMetric& operator++();
        IncrementMetric& operator++(int);
        
    private:
        std::string name_;
        std::atomic<unsigned long long> counter_;
    };
}