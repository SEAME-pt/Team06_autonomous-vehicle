cmake_minimum_required(VERSION 3.10)

# Add GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/release-1.10.0.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(googletest)

# Disable warnings as errors for GoogleTest
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(gtest PRIVATE -Wno-error)
  target_compile_options(gtest_main PRIVATE -Wno-error)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/inc)

# Set the ZeroMQLib path
set(ZMQ_LIB "${CMAKE_BINARY_DIR}/zmq/libZeroMQLib.a")

# Create a test utilities library
add_library(test_utils INTERFACE)
target_include_directories(test_utils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Add test executables
add_executable(battery_test BatteryTest.cpp)
target_link_libraries(battery_test gtest gtest_main middleware ${ZMQ_LIB} pthread test_utils)

add_executable(sensor_handler_test SensorHandlerTest.cpp)
target_link_libraries(sensor_handler_test gtest gtest_main middleware ${ZMQ_LIB} pthread test_utils)

add_executable(speed_test SpeedTest.cpp)
target_link_libraries(speed_test gtest gtest_main middleware ${ZMQ_LIB} pthread test_utils)

add_executable(distance_test DistanceTest.cpp)
target_link_libraries(distance_test gtest gtest_main middleware ${ZMQ_LIB} pthread test_utils)

add_executable(back_motors_test BackMotorsTest.cpp)
target_link_libraries(back_motors_test gtest gtest_main middleware ${ZMQ_LIB} pthread test_utils)

add_executable(back_motors_direct_test BackMotorsDirectTest.cpp)
target_link_libraries(back_motors_direct_test gtest gtest_main middleware ${ZMQ_LIB} pthread test_utils)

add_executable(f_servo_test FServoTest.cpp)
target_link_libraries(f_servo_test gtest gtest_main middleware ${ZMQ_LIB} pthread test_utils)

add_executable(f_servo_direct_test FServoDirectTest.cpp)
target_link_libraries(f_servo_direct_test gtest gtest_main middleware ${ZMQ_LIB} pthread test_utils)

add_executable(control_assembly_test ControlAssemblyTest.cpp)
target_link_libraries(control_assembly_test gtest gtest_main middleware ${ZMQ_LIB} pthread test_utils)

add_executable(sensor_logger_test SensorLoggerTest.cpp)
target_link_libraries(sensor_logger_test gtest gtest_main middleware ${ZMQ_LIB} pthread test_utils)

add_executable(control_logger_test ControlLoggerTest.cpp)
target_link_libraries(control_logger_test gtest gtest_main middleware ${ZMQ_LIB} pthread test_utils)

add_executable(battery_reader_test BatteryReaderTest.cpp)
target_link_libraries(battery_reader_test gtest gtest_main middleware ${ZMQ_LIB} pthread test_utils)

add_executable(battery_reader_direct_test BatteryReaderDirectTest.cpp)
target_link_libraries(battery_reader_direct_test gtest gtest_main middleware ${ZMQ_LIB} pthread test_utils)

add_executable(can_reader_test CanReaderTest.cpp)
target_link_libraries(can_reader_test gtest gtest_main middleware ${ZMQ_LIB} pthread test_utils)

add_executable(can_reader_direct_test CanReaderDirectTest.cpp)
target_link_libraries(can_reader_direct_test gtest gtest_main middleware ${ZMQ_LIB} pthread test_utils)

add_executable(zmq_publisher_test ZmqPublisherTest.cpp)
target_link_libraries(zmq_publisher_test gtest gtest_main middleware ${ZMQ_LIB} pthread test_utils)

# Ensure all tests have the C++17 standard
foreach(TEST_TARGET
    battery_test sensor_handler_test speed_test distance_test back_motors_test
    back_motors_direct_test f_servo_test f_servo_direct_test
    control_assembly_test sensor_logger_test control_logger_test
    battery_reader_test battery_reader_direct_test can_reader_test
    can_reader_direct_test zmq_publisher_test)

    target_compile_features(${TEST_TARGET} PRIVATE cxx_std_17)
endforeach()

# Enable testing
enable_testing()

# Set test properties for parallel execution
set(GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)
set(GTEST_DISCOVER_TESTS_TIMEOUT 60)

# Register tests with parallel execution
foreach(TEST_NAME
    battery_test sensor_handler_test speed_test distance_test back_motors_test
    back_motors_direct_test f_servo_test f_servo_direct_test
    control_assembly_test sensor_logger_test control_logger_test
    battery_reader_test battery_reader_direct_test can_reader_test
    can_reader_direct_test zmq_publisher_test)

    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME} --gtest_shuffle --gtest_repeat=1)
    set_tests_properties(${TEST_NAME} PROPERTIES
        ENVIRONMENT "GTEST_COLOR=1"
        TIMEOUT 300
        PROCESSORS 1
    )
endforeach()

# Add a custom target to run all tests in parallel
add_custom_target(run_tests_parallel
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --parallel $(nproc) --schedule-random
    DEPENDS ${TEST_TARGETS}
    COMMENT "Running tests in parallel"
)

# Coverage target
if(CODE_COVERAGE)
  # Add coverage target
  add_custom_target(coverage
    # Clean previous coverage data
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/coverage
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage

    # Run the tests in parallel
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --parallel $(nproc) --schedule-random

    # Generate coverage report without problematic flags
    COMMAND lcov --capture --directory ${CMAKE_BINARY_DIR}/Middleware/CMakeFiles/middleware.dir --output-file ${CMAKE_BINARY_DIR}/coverage/coverage.info
    COMMAND lcov --remove ${CMAKE_BINARY_DIR}/coverage/coverage.info '/usr/*' --output-file ${CMAKE_BINARY_DIR}/coverage/coverage.info
    COMMAND genhtml ${CMAKE_BINARY_DIR}/coverage/coverage.info --output-directory ${CMAKE_BINARY_DIR}/coverage/html

    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating code coverage report"
  )
endif()
