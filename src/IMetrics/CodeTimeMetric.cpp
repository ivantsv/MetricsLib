#include "CodeTimeMetric.h"

#include <string>
#include <sstream>
#include <iomanip>

using namespace Metrics;

std::string CodeTimeMetric::CreateDefaultName() {
    std::lock_guard<std::mutex> lock(counter_mutex_);
    return "\"Algorithm " + std::to_string(++counter) + "\"";
}

CodeTimeMetric::CodeTimeMetric(const std::string& name)
    : task_name_(name)
    , start_(std::chrono::high_resolution_clock::now())
    , finish_(std::chrono::high_resolution_clock::now())
    , is_running_(false)
{}

std::string CodeTimeMetric::GetName() const noexcept {
    return task_name_;
}

std::string CodeTimeMetric::GetValueAsString() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(finish_ - start_).count();
    if (duration < 1'000) {
        oss << duration << " ns";
    } else if (duration < 1'000'000) {
        oss << duration / 1'000.0 << " μs";
    } else if (duration < 1'000'000'000) {
        oss << duration / 1'000'000.0 << " ms";
    } else {
        oss << duration / 1'000'000'000.0 << " s";
    }
    
    return oss.str();
}

void CodeTimeMetric::Evaluate() {}

void CodeTimeMetric::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    start_ = std::chrono::high_resolution_clock::now();
    finish_ = std::chrono::high_resolution_clock::now();
    is_running_ = false;
}

void CodeTimeMetric::Start() {
    std::lock_guard<std::mutex> lock(mutex_);
    start_ = std::chrono::high_resolution_clock::now();
    finish_ = std::chrono::high_resolution_clock::now();
    is_running_ = true;
}

void CodeTimeMetric::Stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_running_) {
        finish_ = std::chrono::high_resolution_clock::now();   
        is_running_ = false;
    }
}