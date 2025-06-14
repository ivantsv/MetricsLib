#include "CPUUsageMetric.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

using namespace Metrics;

CPUUsageMetric::CPUUsageMetric() : cpu_usage_percent_(0.0)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (getCurrentCPUTimes(prev_cpu_times_)) {
        prev_time_point_ = std::chrono::steady_clock::now();
    } else {
        prev_cpu_times_ = {};
        prev_time_point_ = std::chrono::steady_clock::now();
        std::cerr << "Warning: Failed to get initial CPU times." << std::endl;
    }
}

std::string CPUUsageMetric::GetName() const noexcept {
    return "\"CPU Usage\"";
}

std::string CPUUsageMetric::GetValueAsString() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << cpu_usage_percent_ << "%";
    return oss.str();
}

void CPUUsageMetric::Evaluate() {
    std::lock_guard<std::mutex> lock(mutex_);

    CPUTimes current_cpu_times;
    if (!getCurrentCPUTimes(current_cpu_times)) {
        cpu_usage_percent_ = -1.0;
        return;
    }

    if (prev_cpu_times_.getTotal() == 0 || prev_time_point_.time_since_epoch().count() == 0) {
        prev_cpu_times_ = current_cpu_times;
        prev_time_point_ = std::chrono::steady_clock::now();
        cpu_usage_percent_ = 0.0;
        return;
    }

    unsigned long long prev_total_time = prev_cpu_times_.getTotal();
    unsigned long long current_total_time = current_cpu_times.getTotal();

    unsigned long long prev_idle_time = prev_cpu_times_.idle + prev_cpu_times_.iowait;
    unsigned long long current_idle_time = current_cpu_times.idle + current_cpu_times.iowait;

    unsigned long long total_delta = current_total_time - prev_total_time;
    unsigned long long idle_delta = current_idle_time - prev_idle_time;

    if (total_delta > 0) {
        cpu_usage_percent_ = static_cast<double>(total_delta - idle_delta) / total_delta * 100.0;
    } else {
        cpu_usage_percent_ = 0.0;
    }

    prev_cpu_times_ = current_cpu_times;
    prev_time_point_ = std::chrono::steady_clock::now();
}

void CPUUsageMetric::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    cpu_usage_percent_ = 0.0;
    prev_cpu_times_ = {};
    prev_time_point_ = std::chrono::steady_clock::time_point();
}

#ifdef __linux__
bool CPUUsageMetric::getCurrentCPUTimes(CPUTimes& times) const {
    std::ifstream file("/proc/stat");
    if (!file.is_open()) {
        std::cerr << "Failed to open /proc/stat" << std::endl;
        return false;
    }

    std::string line;
    std::getline(file, line);

    std::istringstream iss(line);
    std::string cpu_label;
    if (!(iss >> cpu_label >> times.user 
          >> times.nice >> times.system >> times.idle 
          >> times.iowait >> times.irq >> times.softirq 
          >> times.steal >> times.guest >> times.guest_nice)) {
        std::cerr << "Failed to parse /proc/stat line." << std::endl;
        return false;
    }
    return true;
}
#elif _WIN32
bool CPUUsageMetric::getCurrentCPUTimes(CPUTimes& times) const {
    FILETIME idleTime, kernelTime, userTime;

    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        std::cerr << "Failed to get system times. Error: " << GetLastError() << std::endl;
        return false;
    }

    auto filetimeToULL = [](const FILETIME& ft) -> unsigned long long {
        return (static_cast<unsigned long long>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    };

    unsigned long long idle_ull = filetimeToULL(idleTime);
    unsigned long long kernel_ull = filetimeToULL(kernelTime);
    unsigned long long user_ull = filetimeToULL(userTime);

    times.idle = idle_ull;
    times.system = kernel_ull - idle_ull; 
    times.user = user_ull;
    times.nice = 0;
    times.iowait = 0;
    times.irq = 0;
    times.softirq = 0;
    times.steal = 0;
    times.guest = 0;
    times.guest_nice = 0;
    
    return true;
}
#endif