#pragma once

#include "IMetrics.h"
#include "MyAny.h"

#include <string>
#include <mutex>
#include <unordered_map>

namespace Metrics {
    
    class CardinalityMetricAny final : public IMetric {
    public:
        CardinalityMetricAny(int n_top = 5);
        std::string GetName() const noexcept override;
        std::string GetValueAsString() const override;
        void Evaluate() override;
        void Reset() override;
        
        template <typename T>
        void Observe(T&& item, long long count=1) {
            std::lock_guard<std::mutex> lock(mutex_);
            MyCustomAny::MyAny key(std::forward<T>(item));
            
            auto it = observed_items_.find(key);
            if (it != observed_items_.end()) {
                it->second += count;
            } else {
                observed_items_[std::move(key)] = count;
            }
        }
        
    private:
        struct MyAnyHash {
            std::size_t operator()(const MyCustomAny::MyAny& any) const {
                if (!any.has_value()) {
                    return 0;
                }
                return any.type().hash_code();
            }
        };
    
        int n_top_;
        std::unordered_map<MyCustomAny::MyAny, long long, MyAnyHash> observed_items_;
        mutable std::mutex mutex_;
    };
}