#include "IMetrics.h"
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#include <pdh.h>
#pragma comment(lib, "pdh.lib")
#else
#include <fstream>
#include <unistd.h>
#endif

using namespace Metrics;

CPUMetric::CPUMetric() : current_utilization_(0.0), cpu_count_(0) {
    cpu_count_ = getCPUCount();
    initializeCPUData();
}

std::string CPUMetric::getName() const {
    return "CPU";
}

std::string CPUMetric::getValueAsString() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << current_utilization_;
    return oss.str();
}

void CPUMetric::evaluate() {
    current_utilization_ = calculateCPUUsage();
}

void CPUMetric::reset() {
    current_utilization_ = 0.0;
    initializeCPUData();
}

int CPUMetric::getCPUCount() {
#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return static_cast<int>(sysinfo.dwNumberOfProcessors);
#else
    return static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
#endif
}

void CPUMetric::initializeCPUData() {
#ifdef _WIN32
    FILETIME idle_time, kernel_time, user_time;
    GetSystemTimes(&idle_time, &kernel_time, &user_time);
    
    ULARGE_INTEGER idle, kernel, user;
    idle.LowPart = idle_time.dwLowDateTime;
    idle.HighPart = idle_time.dwHighDateTime;
    kernel.LowPart = kernel_time.dwLowDateTime;
    kernel.HighPart = kernel_time.dwHighDateTime;
    user.LowPart = user_time.dwLowDateTime;
    user.HighPart = user_time.dwHighDateTime;
    
    prev_idle_time_ = idle.QuadPart;
    prev_kernel_time_ = kernel.QuadPart;
    prev_user_time_ = user.QuadPart;
#else
    std::ifstream proc_stat("/proc/stat");
    std::string line;
    if (std::getline(proc_stat, line)) {
        std::istringstream iss(line);
        std::string cpu;
        unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
        
        iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
        
        prev_idle_ = idle + iowait;
        prev_total_ = user + nice + system + idle + iowait + irq + softirq + steal;
    }
#endif
}

double CPUMetric::calculateCPUUsage() {
#ifdef _WIN32
    FILETIME idle_time_ft, kernel_time_ft, user_time_ft;
    GetSystemTimes(&idle_time_ft, &kernel_time_ft, &user_time_ft);
    
    auto filetime_to_ull = [](const FILETIME& ft) -> unsigned long long {
        ULARGE_INTEGER uli;
        uli.LowPart = ft.dwLowDateTime;
        uli.HighPart = ft.dwHighDateTime;
        return uli.QuadPart;
    };

    unsigned long long curr_idle_time = filetime_to_ull(idle_time_ft);
    unsigned long long curr_kernel_time = filetime_to_ull(kernel_time_ft);
    unsigned long long curr_user_time = filetime_to_ull(user_time_ft);

    unsigned long long idle_diff = curr_idle_time - prev_idle_time_;
    unsigned long long kernel_diff = curr_kernel_time - prev_kernel_time_;
    unsigned long long user_diff = curr_user_time - prev_user_time_;

    prev_idle_time_ = curr_idle_time;
    prev_kernel_time_ = curr_kernel_time;
    prev_user_time_ = curr_user_time;

    unsigned long long total_cpu_time_diff = kernel_diff + user_diff;

    if (total_cpu_time_diff == 0) {
        return 0.0;
    }

    unsigned long long busy_cpu_time_diff = total_cpu_time_diff - idle_diff;

    if (busy_cpu_time_diff > total_cpu_time_diff) {
        busy_cpu_time_diff = total_cpu_time_diff;
    }
    
    double usage_percent = (static_cast<double>(busy_cpu_time_diff) / total_cpu_time_diff) * 100.0;
    
    return (usage_percent / 100.0) * cpu_count_;
    
#else
    std::ifstream proc_stat("/proc/stat");
    std::string line;
    if (!std::getline(proc_stat, line)) {
        return 0.0;
    }
    
    std::istringstream iss(line);
    std::string cpu_label;
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    
    if (!(iss >> cpu_label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal)) {
        return 0.0; 
    }
    
    unsigned long long curr_idle = idle + iowait;
    unsigned long long curr_total = user + nice + system + idle + iowait + irq + softirq + steal;
    
    unsigned long long idle_diff = curr_idle - prev_idle_;
    unsigned long long total_diff = curr_total - prev_total_;

    prev_idle_ = curr_idle;
    prev_total_ = curr_total;
    
    if (total_diff == 0) {
        return 0.0;
    }
    
    double usage_percent = (static_cast<double>(total_diff - idle_diff) / total_diff) * 100.0;

    return (usage_percent / 100.0) * cpu_count_;
#endif
}