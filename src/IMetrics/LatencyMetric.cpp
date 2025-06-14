#include "LatencyMetric.h"

#include <mutex>
#include <sstream>
#include <stdexcept>

using namespace Metrics;

LatencyMetric::LatencyMetric() {
    std::lock_guard<std::mutex> lock(mutex_);
    int64_t max_latency_ns = 3600000000000;
    if (hdr_init(1, max_latency_ns, 3, &histogram_) != 0) {
        throw std::runtime_error("Error during LatencyMetric creation.");
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
    double p90, p95, p99, p999;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        p90 = hdr_value_at_percentile(histogram_, 90.0);
        p95 = hdr_value_at_percentile(histogram_, 95.0);
        p99 = hdr_value_at_percentile(histogram_, 99.0);
        p999 = hdr_value_at_percentile(histogram_, 99.9);
    }
    
    std::ostringstream oss;
    oss << "P90: " << p90 << "ns, ";
    oss << "P95: " << p95 << "ns, ";
    oss << "P99: " << p99 << "ns, ";
    oss << "P999: " << p999 << "ns";
    return oss.str();
}

void LatencyMetric::Evaluate() {}

void LatencyMetric::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    hdr_reset(histogram_);
}