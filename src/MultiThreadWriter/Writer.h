#pragma once

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <fstream>

namespace NonBlockingWriter {

class AsyncWriter {
public:
    explicit AsyncWriter(const std::string& filename);

    ~AsyncWriter();
    
    bool Start();
    
    void Stop();
    
    bool Write(const std::string& text);

    bool IsRunning() const noexcept;

private:
    std::string filename_;
    std::ofstream file_;
    
    mutable std::mutex queue_mutex_;
    std::queue<std::string> text_queue_;
    std::condition_variable queue_condition_;
    
    std::atomic<bool> running_;
    std::atomic<bool> should_stop_;
    std::thread writer_thread_;

    void WriterLoop() noexcept;
    
    AsyncWriter(const AsyncWriter&) = delete;
    AsyncWriter& operator=(const AsyncWriter&) = delete;
};

}