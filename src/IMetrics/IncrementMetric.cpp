#include "IncrementMetric.h"

using namespace Metrics;

std::string IncrementMetric::CreateDefaultName() {
    std::lock_guard<std::mutex> lock(counter_mutex_);
    return "\"IncrementMetric " + std::to_string(++counter) + "\"";
}

IncrementMetric::IncrementMetric(const std::string& name, unsigned long long start) 
    : name_(name)
    , counter_(start)
{}

std::string IncrementMetric::GetName() const noexcept {
    return name_;
}

std::string IncrementMetric::GetValueAsString() const noexcept {
    return std::to_string(counter_);
}

void IncrementMetric::Evaluate() {}

void IncrementMetric::Reset() {
    counter_ = 0;
}

IncrementMetric& IncrementMetric::operator++() {
    ++counter_;
    
    return *this;
}

IncrementMetric& IncrementMetric::operator++(int) {
    return ++(*this);
}