#pragma once

#include <string>
#include <type_traits>

namespace Metrics {
    class IMetric {
    public:
        virtual ~IMetric() = default;
        virtual std::string getName() const = 0;
        virtual std::string getValueAsString() const = 0;
        virtual void evaluate() = 0;
        virtual void reset() = 0;
    };
}

template <typename T>
concept Metric = std::is_base_of_v<Metrics::IMetric, T>;

std::ostream& operator<<(std::ostream& os, Metric auto& metric) {
    os << metric.getName() << ": " << metric.getValueAsString();
    metric.reset();
    
    return os;
}

std::ostream& operator<<(std::ostream& os, const Metric auto& metric) {
    os << metric.getName() << ": " << metric.getValueAsString();
    
    return os;
}