#pragma once

#include "Writer.h"
#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace NonBlockingWriter {

class WriterUtils {
public:

    static bool WriteWithTimestamp(AsyncWriter& writer, const std::string& text) {
        std::string timestamped = GetCurrentTimestamp() + " " + text;
        return writer.Write(timestamped);
    }
    
    template<typename... Args>
    static bool WriteFormatted(AsyncWriter& writer, const std::string& format, Args... args) {
        std::ostringstream oss;
        WriteFormattedImpl(oss, format, args...);
        return writer.Write(oss.str());
    }
    
    template<typename T>
    static bool WriteMetric(AsyncWriter& writer, const std::string& name, const T& value) {
        std::ostringstream oss;
        oss << name << ": " << value;
        return writer.Write(oss.str());
    }
    

    template<typename T>
    static bool WriteMetricWithTimestamp(AsyncWriter& writer, const std::string& name, const T& value) {
        std::ostringstream oss;
        oss << GetCurrentTimestamp() << " " << name << ": " << value;
        return writer.Write(oss.str());
    }

private:
    static std::string GetCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();
        
        return oss.str();
    }
    
    template<typename T>
    static void WriteFormattedImpl(std::ostringstream& oss, const std::string& format, T&& value) {
        size_t pos = format.find("{}");
        if (pos != std::string::npos) {
            oss << format.substr(0, pos) << value << format.substr(pos + 2);
        } else {
            oss << format;
        }
    }
    
    template<typename T, typename... Args>
    static void WriteFormattedImpl(std::ostringstream& oss, const std::string& format, T&& value, Args&&... args) {
        size_t pos = format.find("{}");
        if (pos != std::string::npos) {
            std::string remaining = format.substr(0, pos) + std::to_string(value) + format.substr(pos + 2);
            WriteFormattedImpl(oss, remaining, args...);
        } else {
            oss << format;
        }
    }
};

}