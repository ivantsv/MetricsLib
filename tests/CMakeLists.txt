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

add_executable(
    http_income_tests
    http_income_tests.cpp
)

target_link_libraries(
    http_income_tests
    GTest::gtest_main
    GTest::gmock_main
    IMetrics
)

target_include_directories(http_income_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)

add_executable(
    latency_metric_tests
    latency_metric_tests.cpp
)

target_link_libraries(
    latency_metric_tests
    GTest::gtest_main
    GTest::gmock_main
    IMetrics
)

target_include_directories(latency_metric_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)

add_executable(
    cpu_usage_metric_tests
    cpu_usage_metric_tests.cpp
)

target_link_libraries(
    cpu_usage_metric_tests
    GTest::gtest_main
    GTest::gmock_main
    IMetrics
)

target_include_directories(cpu_usage_metric_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)

add_executable(
    code_time_tests
    code_time_tests.cpp
)

target_link_libraries(
    code_time_tests
    GTest::gtest_main
    GTest::gmock_main
    IMetrics
)

target_include_directories(code_time_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)

add_executable(
    increment_metric_tests
    increment_metric_tests.cpp
)

target_link_libraries(
    increment_metric_tests
    GTest::gtest_main
    GTest::gmock_main
    IMetrics
)

target_include_directories(increment_metric_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)

add_executable(
    my_any_tests
    my_any_tests.cpp
)

target_link_libraries(
    my_any_tests
    GTest::gtest_main
    GTest::gmock_main
    IMetrics
)

target_include_directories(my_any_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)

add_executable(
    cardinality_metric_any_tests
    cardinality_metric_any_tests.cpp
)

target_link_libraries(
    cardinality_metric_any_tests
    GTest::gtest_main
    GTest::gmock_main
    IMetrics
)

target_include_directories(cardinality_metric_any_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)

include(GoogleTest)

gtest_discover_tests(cpu_metric_tests)
gtest_discover_tests(writer_tests)
gtest_discover_tests(http_income_tests)
gtest_discover_tests(latency_metric_tests)
gtest_discover_tests(cpu_usage_metric_tests)
gtest_discover_tests(code_time_tests)
gtest_discover_tests(increment_metric_tests)
gtest_discover_tests(my_any_tests)
gtest_discover_tests(cardinality_metric_any_tests)