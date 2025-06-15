#include <iostream>
#include <thread>
#include <string>
#include <chrono>

#include "MetricsManager/MetricsManager.h"
#include "IMetrics/Metrics.h"
#include "MultiThreadWriter/MultiThreadWriter.h"

struct SomeStruct {
    std::string s;
    
    bool operator==(const SomeStruct& other) const = default;
};

int main() {
    MetricsManager manager_work_examples("../examples.log");
    MetricsManager manager_log_work_examples("../diff_logs_examples.log");
    MetricsManager manager_multithread_examples("../multithread_examples.log");
    
    // How to use: CardinalityMetricType
    {
        auto* cardinality_metric_type = manager_work_examples.CreateMetric<Metrics::CardinalityMetricType>();
        
        int a = 5, b = 5, c = 5, d = 6, e = 6, f = 5;
        double d_a = 3.14, d_b = 5.15;
        SomeStruct x{"x"}, y{"x"}, z{"x"};
        
        cardinality_metric_type->Observe(a);
        cardinality_metric_type->Observe(b);
        cardinality_metric_type->Observe(c);
        cardinality_metric_type->Observe(d);
        cardinality_metric_type->Observe(e);
        cardinality_metric_type->Observe(f);
        cardinality_metric_type->Observe(d_a);
        cardinality_metric_type->Observe(d_b);
        cardinality_metric_type->Observe(x);
        cardinality_metric_type->Observe(y);
        cardinality_metric_type->Observe(z);
    }
    
    // How to use: CardinalityMetricValue
    {
        auto* cardinality_metric_value = manager_work_examples.CreateMetric<Metrics::CardinalityMetricValue<int, double, std::string>>();
        
        int a = 5, b = 5, c = 5, d = 6, e = 6, f = 5;
        double d_a = 3.14, d_b = 5.15;
        std::string x{"x"}, y{"x"}, z{"x"};
        
        cardinality_metric_value->Observe(a);
        cardinality_metric_value->Observe(b);
        cardinality_metric_value->Observe(c);
        cardinality_metric_value->Observe(d);
        cardinality_metric_value->Observe(e);
        cardinality_metric_value->Observe(f);
        cardinality_metric_value->Observe(d_a);
        cardinality_metric_value->Observe(d_b);
        cardinality_metric_value->Observe(x);
        cardinality_metric_value->Observe(y);
        cardinality_metric_value->Observe(z);
    }
    
    // How to use: CodeTimeMetric
    {
        auto* code_time_metric = manager_work_examples.CreateMetric<Metrics::CodeTimeMetric>();
        
        code_time_metric->Start();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        code_time_metric->Stop();
    }
    
    // How to use: CPUUsageMetric
    {
        auto* cpu_usage_metric = manager_work_examples.CreateMetric<Metrics::CPUUsageMetric>();
        
        for (int i = 0; i < 1000000; ++i) {
            volatile int dummy = i * i;
        }     
    }
    
    // How to use: CPUUtilMetric
    {
        auto* cpu_util_metric = manager_work_examples.CreateMetric<Metrics::CPUMetric>();
   
        std::vector<std::thread> workers;
        for (int i = 0; i < 4; ++i) {
            workers.emplace_back([]() {
                auto start = std::chrono::steady_clock::now();
                while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(200)) {
                    volatile int dummy = 0;
                    for (int j = 0; j < 10000; ++j) dummy += j;
                }
            });
        }
        
        for (auto& worker : workers) {
            worker.join();
        }
    }
    
    // How to use: HTTPIncomeMetric
    {
        auto* http_metric = manager_work_examples.CreateMetric<Metrics::HTTPIncomeMetric>(0);
        
        for (int i = 0; i < 50; ++i) {
            ++(*http_metric);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    // How to use: IncrementMetric
    {
        auto* increment_metric = manager_work_examples.CreateMetric<Metrics::IncrementMetric>();
        
        for (int i = 0; i < 10; ++i) {
            ++(*increment_metric);
            (*increment_metric)++;
        }
    }
    
    // How to use: LatencyMetric
    {
        auto* latency_metric = manager_work_examples.CreateMetric<Metrics::LatencyMetric>();
   
        std::vector<std::chrono::nanoseconds> simulated_latencies = {
            std::chrono::milliseconds(10),
            std::chrono::milliseconds(15),
            std::chrono::milliseconds(5),
            std::chrono::milliseconds(25),
            std::chrono::milliseconds(8),
            std::chrono::milliseconds(30),
            std::chrono::milliseconds(12),
            std::chrono::milliseconds(18),
            std::chrono::milliseconds(22),
            std::chrono::milliseconds(50),
        };
        
        for (const auto& latency : simulated_latencies) {
            latency_metric->Observe(latency);
        }
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto actual_latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        latency_metric->Observe(actual_latency);
        
        for (int i = 0; i < 20; ++i) {
            auto random_latency = std::chrono::milliseconds(5 + (i % 15));
            latency_metric->Observe(random_latency);
        }
    }
    
    manager_work_examples.Log();
    
    // Different Log variants
    {       
        auto* counter = manager_log_work_examples.CreateMetric<Metrics::IncrementMetric>("GeneralCounter", 0);
        auto* cpu_metric = manager_log_work_examples.CreateMetric<Metrics::CPUUsageMetric>();
        auto* http_metric = manager_log_work_examples.CreateMetric<Metrics::HTTPIncomeMetric>(0);
        auto* algo_timer = manager_log_work_examples.CreateMetric<Metrics::CodeTimeMetric>("SortAlgorithm");
        auto* latency_metric = manager_log_work_examples.CreateMetric<Metrics::LatencyMetric>();

        for (int i = 0; i < 10; ++i) {
            ++(*counter);
            ++(*http_metric);
        }
        
        algo_timer->Start();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        algo_timer->Stop();
        
        latency_metric->Observe(std::chrono::milliseconds(15));
        
        // Log all metrics (default)
        manager_log_work_examples.Log();
        
        // Log only Server metrics
        manager_log_work_examples.Log<MetricTags::ServerMetricTag>();
        
        // Log only Computer metrics
        manager_log_work_examples.Log<MetricTags::ComputerMetricTag>();
        
        // Log only HTTPIncomeMetric metrics
        manager_log_work_examples.Log<Metrics::HTTPIncomeMetric>();
    }
    
    // MultiThreadWorking
    {
        auto* shared_counter = manager_multithread_examples.CreateMetric<Metrics::IncrementMetric>("SharedCounter", 0);
        auto* request_counter = manager_multithread_examples.CreateMetric<Metrics::HTTPIncomeMetric>(0);
        auto* latency_tracker = manager_multithread_examples.CreateMetric<Metrics::LatencyMetric>();
        
        const int num_threads = 8;
        const int operations_per_thread = 100;
        
        // Конкурентная модификация метрик
        {
            std::vector<std::thread> workers;
            std::atomic<int> completed_threads{0};
            
            for (int i = 0; i < num_threads; ++i) {
                workers.emplace_back([&, thread_id = i]() {
                    for (int j = 0; j < operations_per_thread; ++j) {
                        ++(*shared_counter);
                        ++(*request_counter);

                        auto latency = std::chrono::microseconds(100 + (thread_id * 10) + (j % 50));
                        latency_tracker->Observe(latency);
                        
                        std::this_thread::sleep_for(std::chrono::microseconds(10));
                    }
                    completed_threads++;
                });
            }
 
            for (auto& worker : workers) {
                worker.join();
            }

            int expected_increments = num_threads * operations_per_thread;
            std::cout << "Expected increments: " << expected_increments << std::endl;
            std::cout << "Actual shared counter: " << shared_counter->GetValueAsString() << std::endl;
            
            request_counter->Evaluate();
            std::cout << "HTTP requests processed: " << request_counter->GetValueAsString() << std::endl;
            std::cout << "Latency percentiles: " << latency_tracker->GetValueAsString() << std::endl;
        }
        
        // Конкурентное логирование
        {
            
            std::vector<std::thread> log_threads;
            std::atomic<int> log_operations{0};
            
            for (int i = 0; i < 5; ++i) {
                log_threads.emplace_back([&, thread_id = i]() {
                    for (int j = 0; j < 3; ++j) {
                        switch (j % 3) {
                            case 0:
                                manager_multithread_examples.Log();
                                std::cout << "Thread " << thread_id << " logged all metrics" << std::endl;
                                break;
                            case 1:
                                manager_multithread_examples.Log<MetricTags::ServerMetricTag>();
                                std::cout << "Thread " << thread_id << " logged server metrics" << std::endl;
                                break;
                            case 2:
                                manager_multithread_examples.Log(0);
                                std::cout << "Thread " << thread_id << " logged metric by index" << std::endl;
                                break;
                        }
                        log_operations++;
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    }
                });
            }
            
            for (auto& log_thread : log_threads) {
                log_thread.join();
            }
            
            std::cout << "Concurrent logging completed. Total log operations: " 
                      << log_operations.load() << std::endl;
        }
        
        // Смешанные операции (создание, модификация, логирование)
        {   
            std::vector<std::thread> mixed_threads;
            std::atomic<int> metrics_created{0};
            std::atomic<int> modifications_done{0};
   
            mixed_threads.emplace_back([&]() {
                for (int i = 0; i < 5; ++i) {
                    auto* new_counter = manager_multithread_examples.CreateMetric<Metrics::IncrementMetric>(
                        "DynamicCounter" + std::to_string(i), i * 10);
                    if (new_counter) {
                        metrics_created++;
                        std::cout << "Created metric: DynamicCounter" << i << std::endl;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            });

            for (int i = 0; i < 3; ++i) {
                mixed_threads.emplace_back([&, thread_id = i]() {
                    for (int j = 0; j < 20; ++j) {
                        ++(*shared_counter);
                        ++(*request_counter);
                        modifications_done++;
                        std::this_thread::sleep_for(std::chrono::milliseconds(25));
                    }
                });
            }

            mixed_threads.emplace_back([&]() {
                for (int i = 0; i < 4; ++i) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(150));
                    manager_multithread_examples.Log<MetricTags::DefaultMetricTag>();
                    std::cout << "Periodic log #" << (i + 1) << " completed" << std::endl;
                }
            });
            
            for (auto& thread : mixed_threads) {
                thread.join();
            }
            
            std::cout << "Mixed operations completed:" << std::endl;
            std::cout << "- Metrics created: " << metrics_created.load() << std::endl;
            std::cout << "- Modifications done: " << modifications_done.load() << std::endl;
            
            manager_multithread_examples.Log();
        }
    }
    
    return 0;
}