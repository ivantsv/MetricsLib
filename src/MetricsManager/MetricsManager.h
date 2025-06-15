#pragma once

#include "MultiThreadWriter/Writer.h"
#include "MultiThreadWriter/WriterUtils.h"
#include "IMetrics/IMetrics.h"
#include "IMetrics/Demangle.h"

#include <stdexcept>
#include <type_traits>
#include <vector>
#include <memory>
#include <mutex>

template <typename Alloc = std::allocator<std::unique_ptr<Metrics::IMetric>>>
class MetricsManager {
private:
    static inline std::atomic<unsigned long long> counter = 0;
    static inline std::mutex counter_mutex_;
    static std::string CreateLogDefaultName() {
        std::lock_guard<std::mutex> lock(counter_mutex_);
        return "metrics" + std::to_string(++counter) + ".log";
    }
    
public:
    MetricsManager(const std::string& name=CreateLogDefaultName()) : async_writer_(name) 
    {
        async_writer_.Start();
    }
    
    ~MetricsManager() {
        async_writer_.Stop();
    }
    
    MetricsManager(const MetricsManager& other) = delete;
    MetricsManager& operator=(const MetricsManager& other) = delete;
    MetricsManager(MetricsManager&& other) = delete;
    MetricsManager& operator=(MetricsManager&& other) = delete;
    
    template <typename T, typename... Args>
    requires (std::is_base_of_v<Metrics::IMetric, T>)
    T* CreateMetric(Args&&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        metrics_.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
        
        return static_cast<T*>(metrics_.back().get());
    }
    
    template <typename T>
    T* GetMetric(size_t index) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (index >= metrics_.size()) {
            throw std::out_of_range("Index out of range.");
        }
        
        Metrics::IMetric* base_ptr = metrics_[index].get();
        T* metric_ptr = dynamic_cast<T*>(base_ptr);
        
        if (metric_ptr == nullptr) {
            std::string actual_type_name = "";
            if (base_ptr) {
                actual_type_name = demangle(typeid(*base_ptr).name());
            }
            throw std::runtime_error("Type inconsistency for metric at index " +
                                        std::to_string(index) +
                                        ". Expected type: " + demangle(typeid(T).name()) +
                                        ", actual type: " + actual_type_name);
        }
            
        return metric_ptr;
    }
    
    template <typename T = MetricTags::DefaultMetricTag>
    requires (std::is_base_of_v<MetricTags::DefaultMetricTag, T>)
    void Log() {
        std::vector<Metrics::IMetric*> metrics_to_process;
        metrics_to_process.reserve(metrics_.size());
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& metric_ptr_unique : metrics_) {
                if (!metric_ptr_unique) continue;

                if (dynamic_cast<T*>(metric_ptr_unique.get()) != nullptr) {
                    metrics_to_process.push_back(metric_ptr_unique.get());
                }
            }
        }

        for (const auto& metric_ptr_raw : metrics_to_process) {
            metric_ptr_raw->Evaluate();
            NonBlockingWriter::WriterUtils::WriteMetricWithTimestamp(async_writer_, metric_ptr_raw->GetName(), metric_ptr_raw->GetValueAsString());
            metric_ptr_raw->Reset();
        }
    }
    
    void Log(size_t index) {
        Metrics::IMetric* metric_ptr_raw;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (index >= metrics_.size()) {
                throw std::out_of_range("Index out of range.");
            }
            metric_ptr_raw = metrics_[index].get();
        }

        metric_ptr_raw->Evaluate();
        NonBlockingWriter::WriterUtils::WriteMetricWithTimestamp(async_writer_, metric_ptr_raw->GetName(), metric_ptr_raw->GetValueAsString());
        metric_ptr_raw->Reset();
    }
    
private:

    NonBlockingWriter::AsyncWriter async_writer_;
    std::vector<std::unique_ptr<Metrics::IMetric>, Alloc> metrics_;

    std::mutex mutex_;
};