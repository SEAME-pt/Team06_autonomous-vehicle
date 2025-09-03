# Middleware Component

A comprehensive component that handles sensor data processing, control signals, and communication between system components for the autonomous vehicle system.

## Overview

The Middleware provides:
- **Sensor data collection and processing** - Real-time sensor data from CAN bus, I2C, and other interfaces
- **Control signal handling** - Motor control, steering, and emergency braking systems
- **Data logging** - Comprehensive logging of sensor data and control commands
- **ZeroMQ-based communication** - Inter-process communication with other system components
- **CAN bus integration** - High-performance CAN message bus with consumer pattern
- **Emergency brake optimization** - Direct callback communication for critical safety functions

## Core Components

### Sensors
- **`Battery`** - Battery level monitoring with charging detection
- **`Speed`** - Speed sensor handling with odometer calculation
- **`Distance`** - Distance sensor with collision detection and emergency brake triggering
- **`BackMotors`** - Motor control and feedback with PWM management
- **`FServo`** - Front servo control interface for steering
- **`CanReader`** - CAN bus communication with MCP2515 controller

### Processing & Control
- **`SensorHandler`** - Manages sensor data collection and publishing
- **`ControlAssembly`** - Processes control signals and handles emergency braking
- **`BatteryReader`** - Battery data processing with voltage and current monitoring
- **`CanMessageBus`** - Singleton CAN message bus with consumer pattern
- **`LaneKeepingHandler`** - Lane keeping assistance data processing
- **`TrafficSignHandler`** - Traffic sign detection and speed limit processing

### Logging
- **`SensorLogger`** - Sensor data logging with timestamp and value tracking
- **`ControlLogger`** - Control command logging for debugging and analysis

### Interfaces
- **`ISensor`** - Base sensor interface with thread-safe data access
- **`IPublisher`** - Publisher interface for ZeroMQ communication
- **`ICanConsumer`** - CAN message consumer interface
- **`IBackMotors`** - Motor control interface
- **`IFServo`** - Servo control interface
- **`IBatteryReader`** - Battery reading interface

### Mock Implementations (Testing)
- **`MockBackMotors`** - Mock implementation for motor control testing
- **`MockFServo`** - Mock implementation for servo control testing
- **`MockBatteryReader`** - Mock implementation for battery testing
- **`MockCanReader`** - Mock implementation for CAN bus testing
- **`MockPublisher`** - Mock implementation for publisher testing

## Key Features

### Emergency Brake Optimization
The system implements a high-performance emergency brake system using direct callback communication instead of ZeroMQ messaging:

- **Zero Latency**: Direct function calls instead of message passing
- **Performance**: Emergency brake response time improved from ~1ms to ~0.01ms
- **Type Safety**: Compile-time checking instead of runtime string parsing
- **Resource Efficient**: No additional threads or network sockets


### CAN Bus Integration
- **Singleton Pattern**: Centralized CAN message bus for efficient resource usage
- **Consumer Pattern**: Multiple sensors can subscribe to CAN messages
- **Thread Safety**: Atomic operations and mutex protection
- **Message Queuing**: Reliable message delivery with overflow protection
- **Statistics**: Message received/dispatched/dropped counters

### Intelligent Control
- **Speed-based Emergency Braking**: Emergency brake intensity based on current speed
- **Autonomous Mode Support**: Handles both manual and autonomous driving modes
- **Real-time Processing**: High-frequency sensor updates (50ms intervals)

## Dependencies

- **C++17 compiler** - Modern C++ features and standard library
- **CMake 3.10+** - Build system configuration
- **ZeroMQ library (libzmq)** - High-performance messaging
- **cppzmq headers** - C++ ZeroMQ bindings
- **pthread** - POSIX threads for multi-threading
- **Custom ZeroMQ wrapper library** - Project-specific ZMQ utilities
- **Linux I2C/SPI support** - Hardware interface libraries

## Building

### Build as part of the main project:
```bash
./scripts/build.sh
```

### For Middleware-specific development:
```bash
cd Middleware
mkdir -p build && cd build
cmake ..
make
```

### Build with specific options:
```bash
cmake -DBUILD_TESTS=ON -DCODE_COVERAGE=ON ..
make
```

## Directory Structure

```
Middleware/
├── inc/                          # Header files
│   ├── Core interfaces          # ISensor.hpp, IPublisher.hpp, etc.
│   ├── Sensor headers           # Battery.hpp, Speed.hpp, Distance.hpp
│   ├── Control headers          # BackMotors.hpp, FServo.hpp
│   ├── Processing headers       # SensorHandler.hpp, ControlAssembly.hpp
│   ├── Communication headers    # CanMessageBus.hpp, CanReader.hpp
│   ├── Handler headers          # LaneKeepingHandler.hpp, TrafficSignHandler.hpp
│   └── Mock implementations     # Mock*.hpp files for testing
├── src/                         # Implementation files
├── test/                        # Comprehensive unit tests
│   ├── Sensor tests            # BatteryTest.cpp, SpeedTest.cpp, etc.
│   ├── Control tests           # BackMotorsTest.cpp, FServoTest.cpp
│   ├── Integration tests       # SensorHandlerTest.cpp, ControlAssemblyTest.cpp
│   ├── Communication tests     # CanReaderTest.cpp, ZmqPublisherTest.cpp
│   └── Test utilities          # TestUtils.hpp
├── CMakeLists.txt              # Build configuration
├── README.md                   # This documentation
├── EMERGENCY_BRAKE_OPTIMIZATION.md  # Emergency brake optimization details
└── test/README.md              # Test suite documentation
```

## Components

The Middleware consists of:
- **Static library** (`libmiddleware.a`) - Core functionality library
- **Main executable** (`Middleware`) - Standalone application
- **Comprehensive test suite** - 81.9% line coverage, 94.6% function coverage

## Testing

### Run all tests:
```bash
./scripts/run_tests.sh
```

### Run Middleware-specific tests:
```bash
cd Middleware/build
ctest
```

### Run specific test categories:
```bash
# Sensor tests
ctest -R "Test$"

# Control tests
ctest -R "Control"

# Integration tests
ctest -R "Handler"
```

### Test Coverage
The test suite achieves excellent coverage:
- **100% line coverage** for Battery and Speed sensor components
- **94.7% line coverage** for ControlAssembly component
- **89.8% line coverage** for SensorHandler component
- **All components** achieve at least 81.8% function coverage

See `test/README.md` for detailed test documentation.

## Build Options

### CMake Configuration Options:
- **`BUILD_TESTS`** - Enable/disable test building (ON by default)
- **`CODE_COVERAGE`** - Enable coverage reporting (OFF by default)

### Example configurations:
```bash
# Debug build with tests
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON ..

# Release build with coverage
cmake -DCMAKE_BUILD_TYPE=Release -DCODE_COVERAGE=ON ..

# Production build without tests
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF ..
```

## Performance Characteristics

- **Sensor Update Frequency**: 50ms for critical sensors, 200ms for non-critical
- **Emergency Brake Response**: <0.01ms (direct callback)
- **CAN Message Processing**: 1ms polling interval
- **Memory Usage**: Optimized with smart pointers and RAII
- **Thread Safety**: Atomic operations and mutex protection throughout

## Architecture Notes

The Middleware follows several key architectural patterns:
- **Dependency Injection** - Components receive dependencies through constructors
- **Interface Segregation** - Small, focused interfaces for each component
- **Singleton Pattern** - CAN message bus for resource efficiency
- **Observer Pattern** - CAN message consumers and sensor data publishing
- **RAII** - Automatic resource management with smart pointers

This design ensures maintainability, testability, and performance while providing a robust foundation for the autonomous vehicle system.
