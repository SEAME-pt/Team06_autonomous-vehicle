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

# Add test executables
add_executable(battery_test BatteryTest.cpp)
target_link_libraries(battery_test gtest gtest_main middleware ${ZMQ_LIB} pthread)

add_executable(sensor_handler_test SensorHandlerTest.cpp)
target_link_libraries(sensor_handler_test gtest gtest_main middleware ${ZMQ_LIB} pthread)

add_executable(speed_test SpeedTest.cpp)
target_link_libraries(speed_test gtest gtest_main middleware ${ZMQ_LIB} pthread)

add_executable(back_motors_test BackMotorsTest.cpp)
target_link_libraries(back_motors_test gtest gtest_main middleware ${ZMQ_LIB} pthread)

add_executable(back_motors_direct_test BackMotorsDirectTest.cpp)
target_link_libraries(back_motors_direct_test gtest gtest_main middleware ${ZMQ_LIB} pthread)

add_executable(f_servo_test FServoTest.cpp)
target_link_libraries(f_servo_test gtest gtest_main middleware ${ZMQ_LIB} pthread)

add_executable(f_servo_direct_test FServoDirectTest.cpp)
target_link_libraries(f_servo_direct_test gtest gtest_main middleware ${ZMQ_LIB} pthread)

add_executable(control_assembly_test ControlAssemblyTest.cpp)
target_link_libraries(control_assembly_test gtest gtest_main middleware ${ZMQ_LIB} pthread)

add_executable(sensor_logger_test SensorLoggerTest.cpp)
target_link_libraries(sensor_logger_test gtest gtest_main middleware ${ZMQ_LIB} pthread)

add_executable(control_logger_test ControlLoggerTest.cpp)
target_link_libraries(control_logger_test gtest gtest_main middleware ${ZMQ_LIB} pthread)

add_executable(battery_reader_test BatteryReaderTest.cpp)
target_link_libraries(battery_reader_test gtest gtest_main middleware ${ZMQ_LIB} pthread)

add_executable(battery_reader_direct_test BatteryReaderDirectTest.cpp)
target_link_libraries(battery_reader_direct_test gtest gtest_main middleware ${ZMQ_LIB} pthread)

add_executable(can_reader_test CanReaderTest.cpp)
target_link_libraries(can_reader_test gtest gtest_main middleware ${ZMQ_LIB} pthread)

add_executable(can_reader_direct_test CanReaderDirectTest.cpp)
target_link_libraries(can_reader_direct_test gtest gtest_main middleware ${ZMQ_LIB} pthread)

add_executable(zmq_publisher_test ZmqPublisherTest.cpp)
target_link_libraries(zmq_publisher_test gtest gtest_main middleware ${ZMQ_LIB} pthread)

# Ensure all tests have the C++17 standard
foreach(TEST_TARGET
    battery_test sensor_handler_test speed_test back_motors_test
    back_motors_direct_test f_servo_test f_servo_direct_test
    control_assembly_test sensor_logger_test control_logger_test
    battery_reader_test battery_reader_direct_test can_reader_test
    can_reader_direct_test zmq_publisher_test)

    target_compile_features(${TEST_TARGET} PRIVATE cxx_std_17)
endforeach()

# Enable testing
enable_testing()

# Register tests
add_test(NAME battery_test COMMAND battery_test)
add_test(NAME sensor_handler_test COMMAND sensor_handler_test)
add_test(NAME speed_test COMMAND speed_test)
add_test(NAME back_motors_test COMMAND back_motors_test)
add_test(NAME back_motors_direct_test COMMAND back_motors_direct_test)
add_test(NAME f_servo_test COMMAND f_servo_test)
add_test(NAME f_servo_direct_test COMMAND f_servo_direct_test)
add_test(NAME control_assembly_test COMMAND control_assembly_test)
add_test(NAME sensor_logger_test COMMAND sensor_logger_test)
add_test(NAME control_logger_test COMMAND control_logger_test)
add_test(NAME battery_reader_test COMMAND battery_reader_test)
add_test(NAME battery_reader_direct_test COMMAND battery_reader_direct_test)
add_test(NAME can_reader_test COMMAND can_reader_test)
add_test(NAME can_reader_direct_test COMMAND can_reader_direct_test)
add_test(NAME zmq_publisher_test COMMAND zmq_publisher_test)

# Coverage target
if(CODE_COVERAGE)
  # Add coverage target
  add_custom_target(coverage
    # Clean previous coverage data
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/coverage
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage

    # Run the tests
    COMMAND ${CMAKE_BINARY_DIR}/bin/battery_test
    COMMAND ${CMAKE_BINARY_DIR}/bin/sensor_handler_test
    COMMAND ${CMAKE_BINARY_DIR}/bin/speed_test
    COMMAND ${CMAKE_BINARY_DIR}/bin/back_motors_test
    COMMAND ${CMAKE_BINARY_DIR}/bin/back_motors_direct_test
    COMMAND ${CMAKE_BINARY_DIR}/bin/f_servo_test
    COMMAND ${CMAKE_BINARY_DIR}/bin/f_servo_direct_test
    COMMAND ${CMAKE_BINARY_DIR}/bin/control_assembly_test
    COMMAND ${CMAKE_BINARY_DIR}/bin/sensor_logger_test
    COMMAND ${CMAKE_BINARY_DIR}/bin/control_logger_test
    COMMAND ${CMAKE_BINARY_DIR}/bin/battery_reader_test
    COMMAND ${CMAKE_BINARY_DIR}/bin/battery_reader_direct_test
    COMMAND ${CMAKE_BINARY_DIR}/bin/can_reader_test
    COMMAND ${CMAKE_BINARY_DIR}/bin/can_reader_direct_test
    COMMAND ${CMAKE_BINARY_DIR}/bin/zmq_publisher_test

    # Generate coverage report without problematic flags
    COMMAND lcov --capture --directory ${CMAKE_BINARY_DIR}/Middleware/CMakeFiles/middleware.dir --output-file ${CMAKE_BINARY_DIR}/coverage/coverage.info
    COMMAND lcov --remove ${CMAKE_BINARY_DIR}/coverage/coverage.info '/usr/*' --output-file ${CMAKE_BINARY_DIR}/coverage/coverage.info
    COMMAND genhtml ${CMAKE_BINARY_DIR}/coverage/coverage.info --output-directory ${CMAKE_BINARY_DIR}/coverage/html

    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating code coverage report"
  )
endif()
