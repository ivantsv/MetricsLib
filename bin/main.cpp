#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <atomic>

#include "IMetrics/CPUMetric.h"
#include "MultiThreadWriter/Writer.h"
#include "MultiThreadWriter/WriterUtils.h"

void cpu_intensive_work(int duration_ms, std::atomic<bool>& should_stop) {
    auto start = std::chrono::high_resolution_clock::now();
    volatile long dummy = 0;
    std::random_device rd;
    std::mt19937 gen(rd());
    
    while (!should_stop && 
           std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::high_resolution_clock::now() - start).count() < duration_ms) {
        
        for (int i = 0; i < 10000; ++i) {
            dummy += gen() % 1000;
            dummy *= (i % 7 + 1);
            dummy = dummy % 1000000;
        }
    }
}

int main() {
    std::cout << "Starting CPU Metrics Demo..." << std::endl;
    
    NonBlockingWriter::AsyncWriter writer("cpu_metrics.log");
    if (!writer.Start()) {
        std::cerr << "Failed to start writer!" << std::endl;
        return 1;
    }
    
    Metrics::CPUMetric cpu_metric;
    std::atomic<bool> should_stop{false};
    
    cpu_metric.evaluate();
    NonBlockingWriter::WriterUtils::writeMetricWithTimestamp(writer, "INITIAL_" + cpu_metric.getName(), cpu_metric.getValueAsString());
    
    std::thread light_load_thread([&]() {
        for (int i = 0; i < 20 && !should_stop; ++i) {
            cpu_intensive_work(200, should_stop);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            
            cpu_metric.evaluate();
            NonBlockingWriter::WriterUtils::writeMetricWithTimestamp(
                writer, 
                "LIGHT_LOAD_" + cpu_metric.getName(), 
                cpu_metric.getValueAsString()
            );
        }
    });
    
    std::thread heavy_load_thread([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        for (int i = 0; i < 5 && !should_stop; ++i) {
            cpu_intensive_work(800, should_stop);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            cpu_metric.evaluate();
            NonBlockingWriter::WriterUtils::writeMetricWithTimestamp(
                writer, 
                "HEAVY_LOAD_" + cpu_metric.getName(), 
                cpu_metric.getValueAsString()
            );
        }
    });

    std::thread monitor_thread([&]() {
        for (int i = 0; i < 30 && !should_stop; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            cpu_metric.evaluate();
            NonBlockingWriter::WriterUtils::writeMetric(
                writer, 
                "MONITOR_" + cpu_metric.getName(), 
                cpu_metric.getValueAsString()
            );

            std::cout << "Current CPU: " << cpu_metric.getValueAsString() << std::endl;
        }
    });
  
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> work_dist(50, 400);
    std::uniform_int_distribution<> rest_dist(100, 600);
    
    for (int i = 0; i < 15; ++i) {
        int work_time = work_dist(gen);
        int rest_time = rest_dist(gen);
        
        cpu_intensive_work(work_time, should_stop);
        std::this_thread::sleep_for(std::chrono::milliseconds(rest_time));
        
        cpu_metric.evaluate();
        NonBlockingWriter::WriterUtils::writeMetricWithTimestamp(
            writer, 
            "VARIABLE_LOAD_" + cpu_metric.getName(), 
            cpu_metric.getValueAsString()
        );
    }
    
    should_stop = true;
    
    light_load_thread.join();
    heavy_load_thread.join();
    monitor_thread.join();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    cpu_metric.evaluate();
    NonBlockingWriter::WriterUtils::writeMetricWithTimestamp(writer, "FINAL_" + cpu_metric.getName(), cpu_metric.getValueAsString());
    
    cpu_metric.reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    cpu_metric.evaluate();
    
    std::cout << "Final CPU reading after reset: " << cpu_metric.getName() 
              << " = " << cpu_metric.getValueAsString() << std::endl;
    
    NonBlockingWriter::WriterUtils::writeMetricWithTimestamp(writer, "IDLE_" + cpu_metric.getName(), cpu_metric.getValueAsString());

    writer.Stop();
    
    std::cout << "Demo completed. Check cpu_metrics.log for detailed results." << std::endl;
    return 0;
}
