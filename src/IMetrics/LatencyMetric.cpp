#include "LatencyMetric.h"

#include <mutex>
#include <sstream>
#include <stdexcept>

using namespace Metrics;

LatencyMetric::LatencyMetric() {
    std::lock_guard<std::mutex> lock(mutex_);
    int64_t max_latency_ns = 3600000000000;
    if (hdr_init(1, max_latency_ns, 3, &histogram_) != 0) {
        throw std::runtime_error("Error during LatencyMetric creation");
    }
}

LatencyMetric::~LatencyMetric() {
    std::lock_guard<std::mutex> lock(mutex_);
    hdr_close(histogram_);
}

void LatencyMetric::Observe(std::chrono::nanoseconds latency) {
    std::lock_guard<std::mutex> lock(mutex_);
    hdr_record_value(histogram_, latency.count());
}

std::string LatencyMetric::GetName() const noexcept {
    return "\"Percentile Latency\"";
}

std::string LatencyMetric::GetValueAsString() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    oss << "P90: " << GetPercentile(90.0) << "ns, ";
    oss << "P95: " << GetPercentile(95.0) << "ns, ";
    oss << "P99: " << GetPercentile(99.0) << "ns, ";
    oss << "P999: " << GetPercentile(99.9) << "ns";
    return oss.str();
}

void LatencyMetric::Evaluate() {}

void LatencyMetric::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    hdr_reset(histogram_);
}

double LatencyMetric::GetPercentile(double percentile) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return hdr_value_at_percentile(histogram_, percentile);
}