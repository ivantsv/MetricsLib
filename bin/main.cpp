#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <atomic>
#include <vector>

#include "IMetrics/CPUUtilMetric.h"
#include "IMetrics/CPUUsageMetric.h"
#include "IMetrics/LatencyMetric.h"
#include "MultiThreadWriter/Writer.h"
#include "MultiThreadWriter/WriterUtils.h"
#include "MetricsManager/MetricsManager.h"
#include "IMetrics/Metrics.h"


int main() {
    MetricsManager manager;
    auto* metric = manager.CreateMetric<Metrics::HTTPIncomeMetric>(100);
    
    for (int i = 0; i < 25; ++i) {
        ++(*metric);
    }
    
    manager.Log(0);
    
    return 0;
}