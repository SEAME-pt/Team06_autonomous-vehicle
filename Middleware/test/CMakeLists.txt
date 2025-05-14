# Test source files
file(GLOB TEST_SOURCES "unit/*.cpp")

# Create test executable
add_executable(middleware_tests ${TEST_SOURCES})

# Link against GTest, GMock and our library
target_link_libraries(middleware_tests
    PRIVATE
    Middleware
    GTest::GTest
    GTest::Main
    gmock
    gmock_main
    gcov
    Threads::Threads
    ${ZMQ_LIB}
    ${ZMQ_LIBRARIES}
)

# Include directories
target_include_directories(middleware_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/inc
    ${CMAKE_SOURCE_DIR}/../zmq/inc
)

# Add tests to CTest
add_test(NAME middleware_tests COMMAND middleware_tests)

# Set environment variables for tests if needed
set_tests_properties(middleware_tests PROPERTIES
    ENVIRONMENT "TEST_DATA_DIR=${CMAKE_CURRENT_SOURCE_DIR}/data"
)
