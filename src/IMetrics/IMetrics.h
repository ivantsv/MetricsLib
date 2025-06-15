#pragma once

#include <string>
#include <type_traits>

#include "MetricsTags.h"

namespace Metrics {
    
    class IMetric : public MetricTags::DefaultMetricTag {
    public:    
        IMetric() = default;
        virtual ~IMetric() = default;
        virtual std::string GetName() const = 0;
        virtual std::string GetValueAsString() const = 0;
        virtual void Evaluate() = 0;
        virtual void Reset() = 0;
        
        IMetric(const IMetric& other) = delete;
        IMetric(IMetric&& other) = delete;
        
        virtual IMetric& operator=(const IMetric& other) = delete;
        virtual IMetric&& operator=(IMetric&& other) = delete;
    };
}

template <typename T>
concept Metric = std::is_base_of_v<Metrics::IMetric, T>;

std::ostream& operator<<(std::ostream& os, Metric auto& metric) {
    os << metric.GetName() << ": " << metric.GetValueAsString();
    metric.Reset();
    
    return os;
}

std::ostream& operator<<(std::ostream& os, const Metric auto& metric) {
    os << metric.GetName() << ": " << metric.GetValueAsString();
    
    return os;
}