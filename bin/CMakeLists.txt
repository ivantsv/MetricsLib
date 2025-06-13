add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE IMetrics)
target_link_libraries(${PROJECT_NAME} PRIVATE NonBlockingWriter)