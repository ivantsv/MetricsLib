include(FetchContent)

set(BUILD_SHARED_LIBS OFF)

cmake_minimum_required(VERSION 3.10)

add_library(
    NonBlockingWriter
    MultiThreadWriter/Writer.cpp
    MultiThreadWriter/Writer.h
)

add_library(
    IMetrics
    IMetrics/IMetrics.cpp
    IMetrics/IMetrics.h
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

target_compile_options(
    NonBlockingWriter PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)
