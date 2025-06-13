#pragma once

#include <string>
#include <type_traits>
#include <mutex>
#include <atomic>

namespace Metrics {
    class IMetric {
    public:
        virtual ~IMetric() = default;
        virtual std::string getName() const = 0;
        virtual std::string getValueAsString() const = 0;
        virtual void evaluate() = 0;
        virtual void reset() = 0;
    };
    
    class CPUMetric final : public IMetric {
    private:
        double current_utilization_;
        int cpu_count_;
        mutable std::mutex mutex_;
        
    #ifdef _WIN32
        unsigned long long prev_idle_time_;
        unsigned long long prev_kernel_time_;
        unsigned long long prev_user_time_;
    #else
        unsigned long long prev_idle_;
        unsigned long long prev_total_;
    #endif
        
    public:
        CPUMetric();
        std::string getName() const noexcept override;
        std::string getValueAsString() const override;
        void evaluate() override;
        void reset() override;
        
    private:
        void initializeCPUData();
        double calculateCPUUsage();
        int getCPUCount() noexcept;
    };
    
    
    class HTTPSIncomeMetric final : public IMetric {
    private:
        std::atomic<unsigned long long> counter_;
        std::atomic<double> current_rps_value_;
        std::atomic<unsigned long long> last_evaluated_counter_;
        
    public:
        HTTPSIncomeMetric(unsigned long long start=0);
        std::string getName() const noexcept override;
        std::string getValueAsString() const override;
        void evaluate() noexcept override;
        void reset() noexcept override;
        
        HTTPSIncomeMetric& operator++(int) noexcept;
        HTTPSIncomeMetric& operator++() noexcept;
    };
}

template <typename T>
concept Metric = std::is_base_of_v<Metrics::IMetric, T>;

std::ostream& operator<<(std::ostream& os, Metric auto& metric) {
    os << metric.getName() << ": " << metric.getValueAsString();
    metric.reset();
    
    return os;
}

std::ostream& operator<<(std::ostream& os, const Metric auto& metric) {
    os << metric.getName() << ": " << metric.getValueAsString();
    
    return os;
}