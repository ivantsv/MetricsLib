include(FetchContent)

set(BUILD_SHARED_LIBS OFF)

cmake_minimum_required(VERSION 3.10)

FetchContent_Declare(
    hdr_histogram
    GIT_REPOSITORY https://github.com/HdrHistogram/HdrHistogram_c
    GIT_TAG 0.11.8
)
FetchContent_MakeAvailable(hdr_histogram)

add_library(
    NonBlockingWriter
    MultiThreadWriter/Writer.cpp
    MultiThreadWriter/Writer.h
)

add_library(
    IMetrics
    IMetrics/IMetrics.h
    IMetrics/CPUMetric.h
    IMetrics/HTTPIncomeMetric.h
    IMetrics/CPUMetric.cpp
    IMetrics/HTTPIncomeMetric.cpp
)

target_include_directories(
    NonBlockingWriter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(
    IMetrics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(Threads REQUIRED)

target_link_libraries(NonBlockingWriter PUBLIC Threads::Threads)

target_compile_features(NonBlockingWriter PUBLIC cxx_std_23)
target_compile_features(IMetrics PUBLIC cxx_std_23)

target_link_libraries(IMetrics PUBLIC hdr_histogram)

target_compile_options(
    NonBlockingWriter PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)