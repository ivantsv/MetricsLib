#include "HTTPIncomeMetric.h"
#include <sstream>
#include <iomanip>

using namespace Metrics;

HTTPIncomeMetric::HTTPIncomeMetric(unsigned long long start)
    : counter_(start)
    , current_rps_value_(0.0)
    , last_evaluated_counter_(start)
{}

std::string HTTPIncomeMetric::GetName() const noexcept {
    return "\"HTTPS requests RPS\"";
}

std::string HTTPIncomeMetric::GetValueAsString() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << current_rps_value_;
    return oss.str();
}

void HTTPIncomeMetric::Evaluate() noexcept {
    unsigned long long current_total_requests = counter_.load(std::memory_order_relaxed);
    unsigned long long requests_in_interval = current_total_requests - last_evaluated_counter_;
    current_rps_value_ = static_cast<double>(requests_in_interval);
    last_evaluated_counter_ = current_total_requests;
}

void HTTPIncomeMetric::Reset() noexcept {
    counter_ = 0;
    current_rps_value_ = 0.0;
    last_evaluated_counter_ = 0;
}

HTTPIncomeMetric& HTTPIncomeMetric::operator++() noexcept {
    ++counter_;
    
    return *this;
}

HTTPIncomeMetric& HTTPIncomeMetric::operator++(int) noexcept {
    return ++(*this);
}