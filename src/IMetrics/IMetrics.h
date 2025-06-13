#pragma once

#include <string>
#include <memory>

namespace Metrics {
    class IMetric {
    public:
        virtual ~IMetric() = default;
        virtual std::string getName() const = 0;
        virtual std::string getValueAsString() const = 0;
        virtual void evaluate() = 0;
        virtual void reset() = 0;
    };
    
    template <typename Derived>
    class ComputerMetrics : public IMetric {
    public:
        void evaluate() override {
            static_cast<Derived*>(this)->evaluate();
        }
    };
    
    class CPUMetric : public ComputerMetrics<CPUMetric> {
    private:
        double current_utilization_;
        int cpu_count_;
        
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
        std::string getName() const override;
        std::string getValueAsString() const override;
        void evaluate() override;
        void reset() override;
        
    private:
        void initializeCPUData();
        double calculateCPUUsage();
        int getCPUCount();
    };
}