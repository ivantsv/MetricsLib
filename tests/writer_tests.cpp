#include <gtest/gtest.h>
#include "MultiThreadWriter/Writer.h"
#include "MultiThreadWriter/WriterUtils.h"
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>
#include <filesystem>
#include <random>

using namespace NonBlockingWriter;
using namespace std::chrono_literals;

class AsyncWriterTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_filename_ = "test_output_" + std::to_string(test_counter_++) + ".txt";
        std::filesystem::remove(test_filename_);
    }

    void TearDown() override {
        std::filesystem::remove(test_filename_);
    }

    std::string ReadFileContent() {
        std::ifstream file(test_filename_);
        if (!file.is_open()) {
            return "";
        }
        
        std::ostringstream oss;
        oss << file.rdbuf();
        return oss.str();
    }

    std::vector<std::string> ReadFileLines() {
        std::ifstream file(test_filename_);
        std::vector<std::string> lines;
        std::string line;
        
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        
        return lines;
    }

    std::string test_filename_;
    static int test_counter_;
};

int AsyncWriterTest::test_counter_ = 0;

TEST_F(AsyncWriterTest, BasicConstruction) {
    AsyncWriter writer(test_filename_);
    EXPECT_FALSE(writer.IsRunning());
}

TEST_F(AsyncWriterTest, StartAndStop) {
    AsyncWriter writer(test_filename_);
    
    EXPECT_TRUE(writer.Start());
    EXPECT_TRUE(writer.IsRunning());
    
    writer.Stop();
    EXPECT_FALSE(writer.IsRunning());
}

TEST_F(AsyncWriterTest, MultipleStartCalls) {
    AsyncWriter writer(test_filename_);
    
    EXPECT_TRUE(writer.Start());
    EXPECT_TRUE(writer.IsRunning());
    
    EXPECT_TRUE(writer.Start());
    EXPECT_TRUE(writer.IsRunning());
    
    writer.Stop();
}

TEST_F(AsyncWriterTest, WriteBeforeStart) {
    AsyncWriter writer(test_filename_);
    
    EXPECT_FALSE(writer.Write("Should not be written"));
    
    std::string content = ReadFileContent();
    EXPECT_TRUE(content.empty());
}

TEST_F(AsyncWriterTest, SingleWrite) {
    AsyncWriter writer(test_filename_);
    writer.Start();
    
    const std::string test_message = "Hello, World!";
    EXPECT_TRUE(writer.Write(test_message));
    
    writer.Stop();
    
    std::string content = ReadFileContent();
    EXPECT_EQ(content, test_message + "\n");
}

TEST_F(AsyncWriterTest, MultipleWrites) {
    AsyncWriter writer(test_filename_);
    writer.Start();
    
    std::vector<std::string> messages = {
        "First message",
        "Second message", 
        "Third message"
    };
    
    for (const auto& msg : messages) {
        EXPECT_TRUE(writer.Write(msg));
    }
    
    writer.Stop();
    
    auto lines = ReadFileLines();
    ASSERT_EQ(lines.size(), messages.size());
    
    for (size_t i = 0; i < messages.size(); ++i) {
        EXPECT_EQ(lines[i], messages[i]);
    }
}

TEST_F(AsyncWriterTest, WriteAfterStop) {
    AsyncWriter writer(test_filename_);
    writer.Start();
    
    EXPECT_TRUE(writer.Write("Before stop"));
    writer.Stop();
    
    EXPECT_FALSE(writer.Write("After stop"));
    
    auto lines = ReadFileLines();
    EXPECT_EQ(lines.size(), 1);
    EXPECT_EQ(lines[0], "Before stop");
}

TEST_F(AsyncWriterTest, ConcurrentWrites) {
    AsyncWriter writer(test_filename_);
    writer.Start();
    
    const int num_threads = 5;
    const int messages_per_thread = 20;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&writer, i, messages_per_thread]() {
            for (int j = 0; j < messages_per_thread; ++j) {
                std::string message = "Thread_" + std::to_string(i) + "_Message_" + std::to_string(j);
                EXPECT_TRUE(writer.Write(message));
                std::this_thread::sleep_for(1ms); // Небольшая задержка для имитации работы
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    writer.Stop();
    
    auto lines = ReadFileLines();
    EXPECT_EQ(lines.size(), num_threads * messages_per_thread);
    
    std::map<std::string, int> thread_counts;
    for (const auto& line : lines) {
        if (line.find("Thread_") == 0) {
            std::string thread_id = line.substr(0, line.find("_Message_"));
            thread_counts[thread_id]++;
        }
    }
    
    EXPECT_EQ(thread_counts.size(), num_threads);
    for (const auto& [thread_id, count] : thread_counts) {
        EXPECT_EQ(count, messages_per_thread);
    }
}

TEST_F(AsyncWriterTest, HighLoadWrite) {
    AsyncWriter writer(test_filename_);
    writer.Start();
    
    const int total_messages = 1000;
    
    for (int i = 0; i < total_messages; ++i) {
        std::string message = "Message_" + std::to_string(i);
        EXPECT_TRUE(writer.Write(message));
    }
    
    writer.Stop();
    
    auto lines = ReadFileLines();
    EXPECT_EQ(lines.size(), total_messages);
    
    for (int i = 0; i < total_messages; ++i) {
        std::string expected = "Message_" + std::to_string(i);
        EXPECT_EQ(lines[i], expected);
    }
}

TEST_F(AsyncWriterTest, RapidStartStop) {
    AsyncWriter writer(test_filename_);
    
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(writer.Start());
        EXPECT_TRUE(writer.Write("Message_" + std::to_string(i)));
        writer.Stop();
        EXPECT_FALSE(writer.IsRunning());
    }
    
    auto lines = ReadFileLines();
    EXPECT_EQ(lines.size(), 10);
}

TEST_F(AsyncWriterTest, InvalidFilePath) {
    AsyncWriter writer("/nonexistent/path/file.txt");
    EXPECT_FALSE(writer.Start());
    EXPECT_FALSE(writer.IsRunning());
}

TEST_F(AsyncWriterTest, EmptyStringWrite) {
    AsyncWriter writer(test_filename_);
    writer.Start();
    
    EXPECT_TRUE(writer.Write(""));
    EXPECT_TRUE(writer.Write("Non-empty"));
    EXPECT_TRUE(writer.Write(""));
    
    writer.Stop();
    
    auto lines = ReadFileLines();
    EXPECT_EQ(lines.size(), 3);
    EXPECT_EQ(lines[0], "");
    EXPECT_EQ(lines[1], "Non-empty");
    EXPECT_EQ(lines[2], "");
}

TEST_F(AsyncWriterTest, LargeStringWrite) {
    AsyncWriter writer(test_filename_);
    writer.Start();
    
    std::string large_string(1024 * 1024, 'A');
    EXPECT_TRUE(writer.Write(large_string));
    
    writer.Stop();
    
    std::string content = ReadFileContent();
    EXPECT_EQ(content.size(), large_string.size() + 1); // +1 для \n
    EXPECT_EQ(content.substr(0, large_string.size()), large_string);
}

TEST_F(AsyncWriterTest, SpecialCharacters) {
    AsyncWriter writer(test_filename_);
    writer.Start();
    
    std::vector<std::string> special_messages = {
        "Русский текст",
        "Line with\ttab",
        "Line\nwith\nnewlines",
        "Special chars: !@#$%^&*()",
        "Unicode: 🚀 📝 ✅"
    };
    
    for (const auto& msg : special_messages) {
        EXPECT_TRUE(writer.Write(msg));
    }
    
    writer.Stop();
    
    std::string content = ReadFileContent();
    EXPECT_FALSE(content.empty());
}

TEST_F(AsyncWriterTest, WriterUtilsBasicMetric) {
    AsyncWriter writer(test_filename_);
    writer.Start();
    
    EXPECT_TRUE(WriterUtils::writeMetric(writer, "CPU_Usage", 45.7));
    EXPECT_TRUE(WriterUtils::writeMetric(writer, "Memory_MB", 1024));
    EXPECT_TRUE(WriterUtils::writeMetric(writer, "Active", true));
    
    writer.Stop();
    
    auto lines = ReadFileLines();
    EXPECT_EQ(lines.size(), 3);
    EXPECT_EQ(lines[0], "CPU_Usage: 45.7");
    EXPECT_EQ(lines[1], "Memory_MB: 1024");
    EXPECT_EQ(lines[2], "Active: 1");
}

TEST_F(AsyncWriterTest, WriterUtilsWithTimestamp) {
    AsyncWriter writer(test_filename_);
    writer.Start();
    
    EXPECT_TRUE(WriterUtils::writeWithTimestamp(writer, "Test message"));
    EXPECT_TRUE(WriterUtils::writeMetricWithTimestamp(writer, "CPU", 50.0));
    
    writer.Stop();
    
    auto lines = ReadFileLines();
    EXPECT_EQ(lines.size(), 2);
    
    EXPECT_TRUE(lines[0].find("Test message") != std::string::npos);
    EXPECT_TRUE(lines[1].find("CPU: 50") != std::string::npos);
    
    EXPECT_TRUE(lines[0].find("-") != std::string::npos);
    EXPECT_TRUE(lines[1].find("-") != std::string::npos);
}

TEST_F(AsyncWriterTest, PerformanceTest) {
    AsyncWriter writer(test_filename_);
    writer.Start();
    
    const int message_count = 10000;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < message_count; ++i) {
        EXPECT_TRUE(writer.Write("Performance test message " + std::to_string(i)));
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    writer.Stop();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    auto lines = ReadFileLines();
    EXPECT_EQ(lines.size(), message_count);
    
    EXPECT_LT(duration.count(), 5000);
    
    std::cout << "Performance: " << message_count << " messages in " << duration.count() << "ms" << std::endl;
}

TEST_F(AsyncWriterTest, ResourceCleanup) {
    for (int i = 0; i < 100; ++i) {
        std::string filename = "cleanup_test_" + std::to_string(i) + ".txt";
        
        {
            AsyncWriter writer(filename);
            writer.Start();
            writer.Write("Test message " + std::to_string(i));
            writer.Stop();
        }
        
        std::filesystem::remove(filename);
    }
    
    SUCCEED();
}

TEST_F(AsyncWriterTest, EdgeCases) {
    AsyncWriter writer(test_filename_);
    
    writer.Start();
    writer.Stop();
    writer.Stop();
    
    EXPECT_FALSE(writer.Write("Should not work"));
    
    EXPECT_TRUE(writer.Start());
    EXPECT_TRUE(writer.Write("Should work"));
    writer.Stop();
    
    auto lines = ReadFileLines();
    EXPECT_EQ(lines.size(), 1);
    EXPECT_EQ(lines[0], "Should work");
}

TEST_F(AsyncWriterTest, StressTestConcurrentStartStop) {
    AsyncWriter writer(test_filename_);
    std::atomic<bool> stop_test{false};
    std::atomic<int> successful_writes{0};
    
    std::thread start_stop_thread([&]() {
        for (int i = 0; i < 50 && !stop_test; ++i) {
            writer.Start();
            std::this_thread::sleep_for(10ms);
            writer.Stop();
            std::this_thread::sleep_for(5ms);
        }
    });
    
    std::thread write_thread([&]() {
        for (int i = 0; i < 100 && !stop_test; ++i) {
            if (writer.Write("Message " + std::to_string(i))) {
                successful_writes++;
            }
            std::this_thread::sleep_for(2ms);
        }
    });
    
    std::this_thread::sleep_for(1000ms);
    stop_test = true;
    
    start_stop_thread.join();
    write_thread.join();
    
    writer.Stop();
    
    EXPECT_GT(successful_writes.load(), 0);
    
    auto lines = ReadFileLines();
    EXPECT_EQ(lines.size(), successful_writes.load());
}

class WriterUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_filename_ = "utils_test_" + std::to_string(test_counter_++) + ".txt";
        std::filesystem::remove(test_filename_);
    }

    void TearDown() override {
        std::filesystem::remove(test_filename_);
    }

    std::vector<std::string> ReadFileLines() {
        std::ifstream file(test_filename_);
        std::vector<std::string> lines;
        std::string line;
        
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        
        return lines;
    }

    bool ContainsTimestamp(const std::string& line) {
        return line.find("-") != std::string::npos && 
               line.find(":") != std::string::npos && 
               line.find(".") != std::string::npos;
    }

    std::string test_filename_;
    static int test_counter_;
};

int WriterUtilsTest::test_counter_ = 0;

TEST_F(WriterUtilsTest, WriteMetricDifferentTypes) {
    AsyncWriter writer(test_filename_);
    writer.Start();
    
    EXPECT_TRUE(WriterUtils::writeMetric(writer, "IntValue", 42));
    EXPECT_TRUE(WriterUtils::writeMetric(writer, "DoubleValue", 3.14159));
    EXPECT_TRUE(WriterUtils::writeMetric(writer, "FloatValue", 2.71f));
    EXPECT_TRUE(WriterUtils::writeMetric(writer, "BoolTrue", true));
    EXPECT_TRUE(WriterUtils::writeMetric(writer, "BoolFalse", false));
    EXPECT_TRUE(WriterUtils::writeMetric(writer, "LongValue", 1234567890L));
    EXPECT_TRUE(WriterUtils::writeMetric(writer, "StringValue", std::string("test_string")));
    EXPECT_TRUE(WriterUtils::writeMetric(writer, "CharValue", 'A'));
    
    writer.Stop();
    
    auto lines = ReadFileLines();
    EXPECT_EQ(lines.size(), 8);
    EXPECT_EQ(lines[0], "IntValue: 42");
    EXPECT_EQ(lines[1], "DoubleValue: 3.14159");
    EXPECT_EQ(lines[2], "FloatValue: 2.71");
    EXPECT_EQ(lines[3], "BoolTrue: 1");
    EXPECT_EQ(lines[4], "BoolFalse: 0");
    EXPECT_EQ(lines[5], "LongValue: 1234567890");
    EXPECT_EQ(lines[6], "StringValue: test_string");
    EXPECT_EQ(lines[7], "CharValue: A");
}

TEST_F(WriterUtilsTest, WriteMetricSpecialValues) {
    AsyncWriter writer(test_filename_);
    writer.Start();
    
    EXPECT_TRUE(WriterUtils::writeMetric(writer, "Zero", 0));
    EXPECT_TRUE(WriterUtils::writeMetric(writer, "NegativeInt", -42));
    EXPECT_TRUE(WriterUtils::writeMetric(writer, "NegativeDouble", -3.14));
    EXPECT_TRUE(WriterUtils::writeMetric(writer, "LargeNumber", 999999999999LL));
    EXPECT_TRUE(WriterUtils::writeMetric(writer, "EmptyString", std::string("")));
    EXPECT_TRUE(WriterUtils::writeMetric(writer, "StringWithSpaces", std::string("hello world")));
    EXPECT_TRUE(WriterUtils::writeMetric(writer, "StringWithSpecialChars", std::string("!@#$%^&*()")));
    
    writer.Stop();
    
    auto lines = ReadFileLines();
    EXPECT_EQ(lines.size(), 7);
    EXPECT_EQ(lines[0], "Zero: 0");
    EXPECT_EQ(lines[1], "NegativeInt: -42");
    EXPECT_EQ(lines[2], "NegativeDouble: -3.14");
    EXPECT_EQ(lines[3], "LargeNumber: 999999999999");
    EXPECT_EQ(lines[4], "EmptyString: ");
    EXPECT_EQ(lines[5], "StringWithSpaces: hello world");
    EXPECT_EQ(lines[6], "StringWithSpecialChars: !@#$%^&*()");
}

TEST_F(WriterUtilsTest, WriteWithTimestampBasic) {
    AsyncWriter writer(test_filename_);
    writer.Start();
    
    EXPECT_TRUE(WriterUtils::writeWithTimestamp(writer, "Simple message"));
    EXPECT_TRUE(WriterUtils::writeWithTimestamp(writer, ""));
    EXPECT_TRUE(WriterUtils::writeWithTimestamp(writer, "Message with special chars: !@#"));
    
    writer.Stop();
    
    auto lines = ReadFileLines();
    EXPECT_EQ(lines.size(), 3);
    
    for (const auto& line : lines) {
        EXPECT_TRUE(ContainsTimestamp(line));
    }
    
    EXPECT_TRUE(lines[0].find("Simple message") != std::string::npos);
    EXPECT_TRUE(lines[2].find("Message with special chars: !@#") != std::string::npos);
}

TEST_F(WriterUtilsTest, WriteMetricWithTimestampTypes) {
    AsyncWriter writer(test_filename_);
    writer.Start();
    
    EXPECT_TRUE(WriterUtils::writeMetricWithTimestamp(writer, "CPU", 45.7));
    EXPECT_TRUE(WriterUtils::writeMetricWithTimestamp(writer, "Memory", 1024));
    EXPECT_TRUE(WriterUtils::writeMetricWithTimestamp(writer, "Active", true));
    EXPECT_TRUE(WriterUtils::writeMetricWithTimestamp(writer, "Status", std::string("running")));
    
    writer.Stop();
    
    auto lines = ReadFileLines();
    EXPECT_EQ(lines.size(), 4);
    
    for (const auto& line : lines) {
        EXPECT_TRUE(ContainsTimestamp(line));
        EXPECT_TRUE(line.find(":") != std::string::npos);
    }
    
    EXPECT_TRUE(lines[0].find("CPU: 45.7") != std::string::npos);
    EXPECT_TRUE(lines[1].find("Memory: 1024") != std::string::npos);
    EXPECT_TRUE(lines[2].find("Active: 1") != std::string::npos);
    EXPECT_TRUE(lines[3].find("Status: running") != std::string::npos);
}

TEST_F(WriterUtilsTest, WriteFormattedEdgeCases) {
    AsyncWriter writer(test_filename_);
    writer.Start();
    
    EXPECT_TRUE(WriterUtils::writeFormatted(writer, "{}", 42));
    EXPECT_TRUE(WriterUtils::writeFormatted(writer, "No placeholders", 42));
    EXPECT_TRUE(WriterUtils::writeFormatted(writer, "Multiple {} {} {} {}", 1, 2, 3, 4));
    EXPECT_TRUE(WriterUtils::writeFormatted(writer, "{}{}{})", 1, 2, 3));
    EXPECT_TRUE(WriterUtils::writeFormatted(writer, "Empty string: {}", std::string("")));
    
    writer.Stop();
    
    auto lines = ReadFileLines();
    EXPECT_EQ(lines.size(), 5);
    EXPECT_EQ(lines[0], "42");
    EXPECT_EQ(lines[1], "No placeholders");
    EXPECT_EQ(lines[2], "Multiple 1 2 3 4");
    EXPECT_EQ(lines[3], "123)");
    EXPECT_EQ(lines[4], "Empty string: ");
}

TEST_F(WriterUtilsTest, TimestampFormat) {
    AsyncWriter writer(test_filename_);
    writer.Start();
    
    auto start_time = std::chrono::system_clock::now();
    EXPECT_TRUE(WriterUtils::writeWithTimestamp(writer, "Time test"));
    auto end_time = std::chrono::system_clock::now();
    
    writer.Stop();
    
    auto lines = ReadFileLines();
    EXPECT_EQ(lines.size(), 1);
    
    std::string line = lines[0];
    
    EXPECT_TRUE(line.length() >= 23);
    
    EXPECT_TRUE(line.find("-") != std::string::npos);
    EXPECT_TRUE(line.find(":") != std::string::npos);
    EXPECT_TRUE(line.find(".") != std::string::npos);
    EXPECT_TRUE(line.find("Time test") != std::string::npos);
    
    std::string timestamp_part = line.substr(0, 23);
    EXPECT_EQ(timestamp_part[4], '-');
    EXPECT_EQ(timestamp_part[7], '-');
    EXPECT_EQ(timestamp_part[10], ' ');
    EXPECT_EQ(timestamp_part[13], ':');
    EXPECT_EQ(timestamp_part[16], ':');
    EXPECT_EQ(timestamp_part[19], '.');
}

TEST_F(WriterUtilsTest, ConcurrentUtilsUsage) {
    AsyncWriter writer(test_filename_);
    writer.Start();
    
    const int num_threads = 4;
    const int operations_per_thread = 25;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&writer, i, operations_per_thread]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                switch (j % 4) {
                    case 0:
                        WriterUtils::writeMetric(writer, "Thread" + std::to_string(i) + "_Metric", j);
                        break;
                    case 1:
                        WriterUtils::writeWithTimestamp(writer, "Thread" + std::to_string(i) + "_Msg_" + std::to_string(j));
                        break;
                    case 2:
                        WriterUtils::writeMetricWithTimestamp(writer, "Thread" + std::to_string(i) + "_TimedMetric", j * 1.5);
                        break;
                    case 3:
                        WriterUtils::writeFormatted(writer, "Thread{}_Formatted_{}", i, j);
                        break;
                }
                std::this_thread::sleep_for(1ms);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    writer.Stop();
    
    auto lines = ReadFileLines();
    EXPECT_EQ(lines.size(), num_threads * operations_per_thread);
    
    std::map<std::string, int> thread_activity;
    for (const auto& line : lines) {
        for (int i = 0; i < num_threads; ++i) {
            std::string thread_prefix = "Thread" + std::to_string(i);
            if (line.find(thread_prefix) != std::string::npos) {
                thread_activity[thread_prefix]++;
                break;
            }
        }
    }
    
    EXPECT_EQ(thread_activity.size(), num_threads);
    for (const auto& [thread_name, count] : thread_activity) {
        EXPECT_EQ(count, operations_per_thread);
    }
}

TEST_F(WriterUtilsTest, WriterUtilsWithInactiveWriter) {
    AsyncWriter writer(test_filename_);
    
    EXPECT_FALSE(WriterUtils::writeMetric(writer, "Test", 42));
    EXPECT_FALSE(WriterUtils::writeWithTimestamp(writer, "Test message"));
    EXPECT_FALSE(WriterUtils::writeMetricWithTimestamp(writer, "Test", 42));
    EXPECT_FALSE(WriterUtils::writeFormatted(writer, "Test {}", 42));
    
    auto lines = ReadFileLines();
    EXPECT_TRUE(lines.empty());
}

TEST_F(WriterUtilsTest, WriterUtilsAfterStop) {
    AsyncWriter writer(test_filename_);
    writer.Start();
    
    EXPECT_TRUE(WriterUtils::writeMetric(writer, "BeforeStop", 1));
    writer.Stop();
    
    EXPECT_FALSE(WriterUtils::writeMetric(writer, "AfterStop", 2));
    EXPECT_FALSE(WriterUtils::writeWithTimestamp(writer, "After stop message"));
    
    auto lines = ReadFileLines();
    EXPECT_EQ(lines.size(), 1);
    EXPECT_EQ(lines[0], "BeforeStop: 1");
}

TEST_F(WriterUtilsTest, PerformanceUtilsTest) {
    AsyncWriter writer(test_filename_);
    writer.Start();
    
    const int operations_count = 1000;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < operations_count; ++i) {
        switch (i % 4) {
            case 0:
                WriterUtils::writeMetric(writer, "Perf_Metric", i);
                break;
            case 1:
                WriterUtils::writeWithTimestamp(writer, "Perf message " + std::to_string(i));
                break;
            case 2:
                WriterUtils::writeMetricWithTimestamp(writer, "Perf_TimedMetric", i * 0.1);
                break;
            case 3:
                WriterUtils::writeFormatted(writer, "Perf formatted {}", i);
                break;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    writer.Stop();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    auto lines = ReadFileLines();
    EXPECT_EQ(lines.size(), operations_count);
    
    EXPECT_LT(duration.count(), 3000);
    std::cout << "WriterUtils Performance: " << operations_count << " operations in " << duration.count() << "ms" << std::endl;
}