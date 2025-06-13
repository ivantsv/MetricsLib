include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
    writer_tests
    writer_tests.cpp
)

target_link_libraries(
    writer_tests
    NonBlockingWriter
    GTest::gtest_main
)

target_include_directories(writer_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)

include(GoogleTest)

gtest_discover_tests(writer_tests)
