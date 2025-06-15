#include "CardinalityMetricType.h"
#include "IMetrics/MyAny.h"
#include "Demangle.h"

#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

using namespace Metrics;

CardinalityMetricType::CardinalityMetricType(int n_top) : n_top_(n_top) {}

std::string CardinalityMetricType::GetName() const noexcept {
    return "\"CardinalityType\"";
}

std::string CardinalityMetricType::GetValueAsString() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    oss << "General number of unique elements: " << observed_items_.size() << '\n';
    oss << std::to_string(n_top_) << " most frequent types: ";
    
    std::vector<std::pair<MyCustomAny::MyAny, long long>> sorted_items;
    for (const auto& item : observed_items_) {
        sorted_items.emplace_back(item.first, item.second);
    }
    
    std::sort(sorted_items.begin(), sorted_items.end(), 
              [](const auto& a, const auto& b) {
                  return a.second > b.second;
              });
    
    int steps = std::min(n_top_, (int)sorted_items.size());
    for (int i = 0; i < steps; ++i) {
        oss << demangle(sorted_items[i].first.type().name()) << " ";
    }
    
    return oss.str();
}

void CardinalityMetricType::Evaluate() {}

void CardinalityMetricType::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    observed_items_.clear();
}