#pragma once

namespace MetricTags {
    struct DefaultMetricTag {};
    
    struct AlgoMetricTag : DefaultMetricTag {};
    
    struct ComputerMetricTag : DefaultMetricTag {};
    
    struct ServerMetricTag : DefaultMetricTag {};
}