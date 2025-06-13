#include "Writer.h"
#include <iostream>

namespace NonBlockingWriter {

AsyncWriter::AsyncWriter(const std::string& filename)
    : filename_(filename)
    , running_(false)
    , should_stop_(false) {
}

AsyncWriter::~AsyncWriter() {
    Stop();
}

bool AsyncWriter::Start() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    if (running_) {
        return true;
    }
    
    file_.open(filename_, std::ios::out | std::ios::app);
    if (!file_.is_open()) {
        std::cerr << "Failed to open file: " << filename_ << std::endl;
        return false;
    }
    
    should_stop_ = false;
    running_ = true;
    
    try {
        writer_thread_ = std::thread(&AsyncWriter::WriterLoop, this);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to start writer thread: " << e.what() << std::endl;
        running_ = false;
        file_.close();
        return false;
    }
}

void AsyncWriter::Stop() {
    should_stop_ = true;

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        queue_condition_.notify_all();
    }
    
    if (writer_thread_.joinable()) {
        writer_thread_.join();
    }
    
    running_ = false;
    
    if (file_.is_open()) {
        file_.close();
    }
}

bool AsyncWriter::Write(const std::string& text) {
    if (!running_ || should_stop_) {
        return false;
    }
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        text_queue_.push(text);
    }
    
    queue_condition_.notify_one();
    return true;
}

bool AsyncWriter::IsRunning() const noexcept {
    return running_;
}

void AsyncWriter::WriterLoop() noexcept {
    while (!should_stop_) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        if (text_queue_.empty()) {
            queue_condition_.wait(lock);
        }
        
        while (!text_queue_.empty() && !should_stop_) {
            std::string text = std::move(text_queue_.front());
            text_queue_.pop();
            
            lock.unlock();
            
            if (file_.is_open()) {
                file_ << text << std::endl;
                file_.flush();
            }
            
            lock.lock();
        }
    }
    
    std::lock_guard<std::mutex> lock(queue_mutex_);
    while (!text_queue_.empty()) {
        if (file_.is_open()) {
            file_ << text_queue_.front() << std::endl;
        }
        text_queue_.pop();
    }
    
    if (file_.is_open()) {
        file_.flush();
    }
}

}