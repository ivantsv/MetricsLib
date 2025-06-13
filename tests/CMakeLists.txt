include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)

# Prevent overriding the parent project's compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
    cpu_metric_tests
    cpu_metric_tests.cpp
)

target_link_libraries(
    cpu_metric_tests
    GTest::gtest_main
    IMetrics
)

# Platform-specific libraries for CPUMetric
if(WIN32)
    target_link_libraries(cpu_metric_tests pdh)
endif()

target_include_directories(cpu_metric_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)

add_executable(
    writer_tests
    writer_tests.cpp
)

target_link_libraries(
    writer_tests
    GTest::gtest_main
    GTest::gmock_main
    NonBlockingWriter
)

target_include_directories(writer_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)

include(GoogleTest)

gtest_discover_tests(cpu_metric_tests)
gtest_discover_tests(writer_tests)
