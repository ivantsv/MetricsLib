#pragma once

#include "IMetrics.h"
#include "Demangle.h"

#include <string>
#include <variant>
#include <unordered_map>
#include <mutex>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>

template <typename... Args>
concept CardinalityMetricValueItem =
    (requires(Args a, Args b) { { a == b } -> std::convertible_to<bool>; } && ...) && 
    (requires(Args a) { std::hash<std::decay_t<Args>>{}(a); } && ...);
    
template <typename T, typename Head, typename... Tail>
struct TypeInPack {
    static constexpr bool value = (std::is_same_v<std::decay_t<T>, std::decay_t<Head>> ? true : TypeInPack<T, Tail...>::value);
};

template <typename T, typename Head>
struct TypeInPack<T, Head> {
    static constexpr bool value = (std::is_same_v<std::decay_t<T>, std::decay_t<Head>> ? true : false);
};

template <typename T, typename... Types>
static constexpr bool TypeInPack_v = TypeInPack<T, Types...>::value; 

template <typename T>
std::string PrettyPrint(T&& item) {
    if constexpr (requires(T&& item) { std::cout << item; }) {
        std::ostringstream oss;
        oss << item;
        return oss.str();
    } else if constexpr (requires(T&& item) { (std::string)item; }) {
        return (std::string)item;
    } else {
        return "Value can't be printed";
    }
}

namespace Metrics {
    
    template <CardinalityMetricValueItem... Args>    
    class CardinalityMetricValue final : public IMetric {
        using Key = std::variant<Args...>;
        
    public:
        CardinalityMetricValue(int n_top = 5) : n_top_(n_top) {}
        
        std::string GetName() const noexcept override {
            return "\"CardinalityValue\"";
        }
        
        std::string GetValueAsString() const override {
            std::lock_guard<std::mutex> lock(mutex_);
            std::ostringstream oss;
            oss << "General number of unique elements: " << observed_items_.size() << '\n';
            oss << std::to_string(n_top_) << " most frequent types: ";
            
            std::vector<std::pair<Key, long long>> sorted_items;
            for (const auto& item : observed_items_) {
                sorted_items.emplace_back(item.first, item.second);
            }
            
            std::sort(sorted_items.begin(), sorted_items.end(), 
                      [](const auto& a, const auto& b) {
                          return a.second > b.second;
                      });
            
            int steps = std::min(n_top_, (int)sorted_items.size());
            for (int i = 0; i < steps; ++i) {
                std::visit([&oss](const auto& item) {
                    oss << demangle(typeid(item).name()) << " " << PrettyPrint(item);
                }, sorted_items[i].first);
                oss << " (quantity: " << sorted_items[i].second << ")";
                if (i < steps - 1) {
                    oss << ", ";
                }
            }
            
            return oss.str();
        }
        
        void Evaluate() override {}
        
        void Reset() override {
            std::lock_guard<std::mutex> lock(mutex_);
            observed_items_.clear();
        }
        
        template <typename T>
        void Observe(T&& item, long long count=1) requires (TypeInPack_v<T, Args...>) {
            std::lock_guard<std::mutex> lock(mutex_);
            Key key(std::forward<T>(item));
            
            auto it = observed_items_.find(key);
            if (it != observed_items_.end()) {
                it->second += count;
            } else {
                observed_items_[std::move(key)] = count;
            }
        }
    
    private:
        struct KeyHash {
            std::size_t operator()(const Key& item) const {
                return std::visit([](const auto& item){
                    return std::hash<std::decay_t<decltype(item)>>{}(item);
                }, item);
            }
        };
        
        int n_top_;
        std::unordered_map<Key, long long, KeyHash> observed_items_;
        mutable std::mutex mutex_;
    };
}