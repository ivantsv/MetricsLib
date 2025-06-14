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

std::chrono::nanoseconds simulate_work_with_latency(int base_latency_ms, int variance_ms) {
    auto start = std::chrono::high_resolution_clock::now();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> latency_dist(
        base_latency_ms - variance_ms, 
        base_latency_ms + variance_ms
    );
    
    int target_latency = std::max(1, latency_dist(gen));
    std::this_thread::sleep_for(std::chrono::milliseconds(target_latency));

    volatile long dummy = 0;
    for (int i = 0; i < target_latency * 100; ++i) {
        dummy += gen() % 1000;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
}

void run_cpu_util_demo() {
    std::cout << "\n=== CPU UTILIZATION METRICS DEMO ===" << std::endl;
    std::cout << "Starting CPU Utilization Metrics Demo..." << std::endl;
    std::cout << "This metric shows CPU utilization as a multiplier of CPU count." << std::endl;
    
    NonBlockingWriter::AsyncWriter writer("cpu_util_metrics.log");
    if (!writer.Start()) {
        std::cerr << "Failed to start CPU Utilization writer!" << std::endl;
        return;
    }
    
    Metrics::CPUMetric cpu_metric;
    std::atomic<bool> should_stop{false};
    
    cpu_metric.Evaluate();
    NonBlockingWriter::WriterUtils::WriteMetricWithTimestamp(writer, "INITIAL_" + cpu_metric.GetName(), cpu_metric.GetValueAsString());
    std::cout << "Initial CPU Utilization: " << cpu_metric.GetValueAsString() << std::endl;
    
    std::thread light_load_thread([&]() {
        std::cout << "Starting light load thread..." << std::endl;
        for (int i = 0; i < 8 && !should_stop; ++i) {
            cpu_intensive_work(200, should_stop);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            
            cpu_metric.Evaluate();
            NonBlockingWriter::WriterUtils::WriteMetricWithTimestamp(
                writer, 
                "LIGHT_LOAD_" + cpu_metric.GetName(), 
                cpu_metric.GetValueAsString()
            );
            std::cout << "Light load " << (i+1) << "/8: " << cpu_metric.GetValueAsString() << std::endl;
        }
    });
    
    std::thread heavy_load_thread([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::cout << "Starting heavy load thread..." << std::endl;
        
        for (int i = 0; i < 3 && !should_stop; ++i) {
            cpu_intensive_work(800, should_stop);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            cpu_metric.Evaluate();
            NonBlockingWriter::WriterUtils::WriteMetricWithTimestamp(
                writer, 
                "HEAVY_LOAD_" + cpu_metric.GetName(), 
                cpu_metric.GetValueAsString()
            );
            std::cout << "Heavy load " << (i+1) << "/3: " << cpu_metric.GetValueAsString() << std::endl;
        }
    });

    std::thread monitor_thread([&]() {
        for (int i = 0; i < 12 && !should_stop; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            cpu_metric.Evaluate();
            NonBlockingWriter::WriterUtils::WriteMetric(
                writer, 
                "MONITOR_" + cpu_metric.GetName(), 
                cpu_metric.GetValueAsString()
            );

            if (i % 3 == 0) {
                std::cout << "=== UTIL MONITOR === " << cpu_metric.GetValueAsString() << std::endl;
            }
        }
    });
  
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> work_dist(50, 400);
    std::uniform_int_distribution<> rest_dist(100, 600);
    
    for (int i = 0; i < 6; ++i) {
        int work_time = work_dist(gen);
        int rest_time = rest_dist(gen);
        
        std::cout << "Variable load " << (i+1) << "/6: work=" << work_time << "ms, rest=" << rest_time << "ms" << std::endl;
        cpu_intensive_work(work_time, should_stop);
        std::this_thread::sleep_for(std::chrono::milliseconds(rest_time));
        
        cpu_metric.Evaluate();
        NonBlockingWriter::WriterUtils::WriteMetricWithTimestamp(
            writer, 
            "VARIABLE_LOAD_" + cpu_metric.GetName(), 
            cpu_metric.GetValueAsString()
        );
    }
    
    should_stop = true;
    
    light_load_thread.join();
    heavy_load_thread.join();
    monitor_thread.join();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    cpu_metric.Evaluate();
    NonBlockingWriter::WriterUtils::WriteMetricWithTimestamp(writer, "FINAL_" + cpu_metric.GetName(), cpu_metric.GetValueAsString());
    
    std::cout << "Testing reset functionality..." << std::endl;
    cpu_metric.Reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    cpu_metric.Evaluate();
    
    std::cout << "Final CPU Utilization after reset: " << cpu_metric.GetName() 
              << " = " << cpu_metric.GetValueAsString() << std::endl;
    
    NonBlockingWriter::WriterUtils::WriteMetricWithTimestamp(writer, "IDLE_" + cpu_metric.GetName(), cpu_metric.GetValueAsString());

    writer.Stop();
    
    std::cout << "CPU Utilization Demo completed. Check cpu_util_metrics.log for detailed results." << std::endl;
}

void run_latency_demo() {
    std::cout << "\n=== LATENCY METRICS DEMO ===" << std::endl;
    std::cout << "Starting Latency Metrics Demo..." << std::endl;
    std::cout << "This metric shows percentile latency distributions (P90, P95, P99, P999)." << std::endl;
    
    NonBlockingWriter::AsyncWriter writer("latency_metrics.log");
    if (!writer.Start()) {
        std::cerr << "Failed to start Latency writer!" << std::endl;
        return;
    }
    
    Metrics::LatencyMetric latency_metric;
    std::atomic<bool> should_stop{false};
    std::atomic<int> request_counter{0};
 
    std::vector<std::thread> worker_threads;

    worker_threads.emplace_back([&]() {
        std::cout << "Starting fast requests simulation..." << std::endl;
        for (int i = 0; i < 80 && !should_stop; ++i) {
            auto latency = simulate_work_with_latency(10, 5);
            latency_metric.Observe(latency);
            request_counter++;
            
            if (i % 10 == 0) {
                NonBlockingWriter::WriterUtils::WriteMetricWithTimestamp(
                    writer, 
                    "FAST_REQUESTS_" + latency_metric.GetName(), 
                    latency_metric.GetValueAsString()
                );
                std::cout << "Fast requests progress: " << (i+1) << "/80, Current: " << latency_metric.GetValueAsString() << std::endl;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(60));
        }
    });

    worker_threads.emplace_back([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::cout << "Starting medium requests simulation..." << std::endl;
        for (int i = 0; i < 40 && !should_stop; ++i) {
            auto latency = simulate_work_with_latency(100, 30);
            latency_metric.Observe(latency);
            request_counter++;
            
            if (i % 5 == 0) {
                NonBlockingWriter::WriterUtils::WriteMetricWithTimestamp(
                    writer, 
                    "MEDIUM_REQUESTS_" + latency_metric.GetName(), 
                    latency_metric.GetValueAsString()
                );
                std::cout << "Medium requests progress: " << (i+1) << "/40, Current: " << latency_metric.GetValueAsString() << std::endl;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    });
 
    worker_threads.emplace_back([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        std::cout << "Starting slow requests simulation..." << std::endl;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> spike_dist(1, 10);
        
        for (int i = 0; i < 15 && !should_stop; ++i) {
            int base_latency = 300;
            int variance = 50;
            
            if (spike_dist(gen) == 1) {
                base_latency = 1000;
                variance = 200;
                std::cout << "Simulating latency spike!" << std::endl;
            }
            
            auto latency = simulate_work_with_latency(base_latency, variance);
            latency_metric.Observe(latency);
            request_counter++;
            
            NonBlockingWriter::WriterUtils::WriteMetricWithTimestamp(
                writer, 
                "SLOW_REQUESTS_" + latency_metric.GetName(), 
                latency_metric.GetValueAsString()
            );
            std::cout << "Slow requests progress: " << (i+1) << "/15, Current: " << latency_metric.GetValueAsString() << std::endl;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(600));
        }
    });

    std::thread monitor_thread([&]() {
        for (int i = 0; i < 25 && !should_stop; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            
            NonBlockingWriter::WriterUtils::WriteMetric(
                writer, 
                "MONITOR_" + latency_metric.GetName(), 
                latency_metric.GetValueAsString()
            );

            if (i % 4 == 0) {
                std::cout << "=== LATENCY MONITOR === Total requests: " << request_counter.load() 
                          << ", Latency: " << latency_metric.GetValueAsString() << std::endl;
            }
        }
    });

    for (auto& thread : worker_threads) {
        thread.join();
    }
    
    should_stop = true;
    monitor_thread.join();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    NonBlockingWriter::WriterUtils::WriteMetricWithTimestamp(
        writer, 
        "FINAL_" + latency_metric.GetName(), 
        latency_metric.GetValueAsString()
    );
    
    std::cout << "Final latency reading: " << latency_metric.GetName() 
              << " = " << latency_metric.GetValueAsString() << std::endl;
    std::cout << "Total requests processed: " << request_counter.load() << std::endl;
    
    std::cout << "Testing reset functionality..." << std::endl;
    latency_metric.Reset();
    NonBlockingWriter::WriterUtils::WriteMetricWithTimestamp(
        writer, 
        "RESET_" + latency_metric.GetName(), 
        latency_metric.GetValueAsString()
    );
    
    std::cout << "After reset: " << latency_metric.GetName() 
              << " = " << latency_metric.GetValueAsString() << std::endl;

    writer.Stop();
    
    std::cout << "Latency Demo completed. Check latency_metrics.log for detailed results." << std::endl;
}

void run_cpu_usage_demo() {
    std::cout << "\n=== CPU USAGE METRICS DEMO ===" << std::endl;
    std::cout << "Starting CPU Usage Metrics Demo..." << std::endl;
    std::cout << "This metric shows CPU usage as a percentage (0-100%)." << std::endl;
    
    NonBlockingWriter::AsyncWriter writer("cpu_usage_metrics.log");
    if (!writer.Start()) {
        std::cerr << "Failed to start CPU Usage writer!" << std::endl;
        return;
    }
    
    Metrics::CPUUsageMetric cpu_usage_metric;
    std::atomic<bool> should_stop{false};
    
    cpu_usage_metric.Evaluate();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    cpu_usage_metric.Evaluate();
    NonBlockingWriter::WriterUtils::WriteMetricWithTimestamp(writer, "INITIAL_" + cpu_usage_metric.GetName(), cpu_usage_metric.GetValueAsString());
    std::cout << "Initial CPU Usage: " << cpu_usage_metric.GetValueAsString() << std::endl;
    
    std::thread light_work_thread([&]() {
        std::cout << "Starting light workload simulation..." << std::endl;
        for (int i = 0; i < 10 && !should_stop; ++i) {
            cpu_intensive_work(150, should_stop);
            std::this_thread::sleep_for(std::chrono::milliseconds(400));
            
            cpu_usage_metric.Evaluate();
            NonBlockingWriter::WriterUtils::WriteMetricWithTimestamp(
                writer, 
                "LIGHT_WORK_" + cpu_usage_metric.GetName(), 
                cpu_usage_metric.GetValueAsString()
            );
            std::cout << "Light workload " << (i+1) << "/10: " << cpu_usage_metric.GetValueAsString() << std::endl;
        }
    });
    
    std::thread heavy_work_thread([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        std::cout << "Starting heavy workload bursts..." << std::endl;
        
        for (int i = 0; i < 4 && !should_stop; ++i) {
            std::cout << "Heavy burst " << (i+1) << "/4 starting..." << std::endl;
            cpu_intensive_work(1000, should_stop);
            std::this_thread::sleep_for(std::chrono::milliseconds(800));
            
            cpu_usage_metric.Evaluate();
            NonBlockingWriter::WriterUtils::WriteMetricWithTimestamp(
                writer, 
                "HEAVY_BURST_" + cpu_usage_metric.GetName(), 
                cpu_usage_metric.GetValueAsString()
            );
            std::cout << "Heavy burst " << (i+1) << " result: " << cpu_usage_metric.GetValueAsString() << std::endl;
        }
    });
    
    std::thread monitor_thread([&]() {
        for (int i = 0; i < 20 && !should_stop; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(700));
            
            cpu_usage_metric.Evaluate();
            NonBlockingWriter::WriterUtils::WriteMetric(
                writer, 
                "MONITOR_" + cpu_usage_metric.GetName(), 
                cpu_usage_metric.GetValueAsString()
            );
            
            if (i % 3 == 0) {
                std::cout << "=== USAGE MONITOR === " << cpu_usage_metric.GetValueAsString() << std::endl;
            }
        }
    });
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> work_dist(100, 500);
    std::uniform_int_distribution<> rest_dist(200, 800);
    
    for (int i = 0; i < 8; ++i) {
        int work_time = work_dist(gen);
        int rest_time = rest_dist(gen);
        
        std::cout << "Variable load " << (i+1) << "/8: work=" << work_time << "ms, rest=" << rest_time << "ms" << std::endl;
        cpu_intensive_work(work_time, should_stop);
        std::this_thread::sleep_for(std::chrono::milliseconds(rest_time));
        
        cpu_usage_metric.Evaluate();
        NonBlockingWriter::WriterUtils::WriteMetricWithTimestamp(
            writer, 
            "VARIABLE_LOAD_" + cpu_usage_metric.GetName(), 
            cpu_usage_metric.GetValueAsString()
        );
        std::cout << "Variable load " << (i+1) << " result: " << cpu_usage_metric.GetValueAsString() << std::endl;
    }
    
    should_stop = true;
    
    light_work_thread.join();
    heavy_work_thread.join();
    monitor_thread.join();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    cpu_usage_metric.Evaluate();
    NonBlockingWriter::WriterUtils::WriteMetricWithTimestamp(writer, "FINAL_" + cpu_usage_metric.GetName(), cpu_usage_metric.GetValueAsString());
    std::cout << "Final CPU Usage: " << cpu_usage_metric.GetValueAsString() << std::endl;
    
    std::cout << "Testing reset functionality..." << std::endl;
    cpu_usage_metric.Reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    cpu_usage_metric.Evaluate();
    
    std::cout << "CPU Usage after reset: " << cpu_usage_metric.GetValueAsString() << std::endl;
    NonBlockingWriter::WriterUtils::WriteMetricWithTimestamp(writer, "AFTER_RESET_" + cpu_usage_metric.GetName(), cpu_usage_metric.GetValueAsString());

    writer.Stop();
    
    std::cout << "CPU Usage Demo completed. Check cpu_usage_metrics.log for detailed results." << std::endl;
}

int main() {
    std::cout << "=== COMPREHENSIVE METRICS DEMO APPLICATION ===" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "This demo application showcases three different metric types:" << std::endl;
    std::cout << "1. CPU Utilization Metrics - Shows CPU load as a multiplier" << std::endl;
    std::cout << "2. Latency Metrics - Shows request latency percentiles" << std::endl;
    std::cout << "3. CPU Usage Metrics - Shows CPU usage as percentage" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    try {
        run_cpu_util_demo();
        
        std::cout << "\nWaiting 3 seconds before starting latency demo..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        run_latency_demo();
        
        std::cout << "\nWaiting 3 seconds before starting CPU usage demo..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        run_cpu_usage_demo();
        
        std::cout << "\n=========================================" << std::endl;
        std::cout << "=== ALL DEMOS COMPLETED SUCCESSFULLY ===" << std::endl;
        std::cout << "=========================================" << std::endl;
        std::cout << "Check the following log files for detailed results:" << std::endl;
        std::cout << "📊 cpu_util_metrics.log   - CPU utilization metrics" << std::endl;
        std::cout << "📊 latency_metrics.log    - Latency percentile metrics" << std::endl;
        std::cout << "📊 cpu_usage_metrics.log  - CPU usage percentage metrics" << std::endl;
        std::cout << "=========================================" << std::endl;
        std::cout << "🎉 Demo completed successfully! 🎉" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Demo failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Demo failed with unknown exception!" << std::endl;
        return 1;
    }
    
    return 0;
}