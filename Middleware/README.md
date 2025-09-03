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
- **`Distance`** - Distance sensor with fixed-threshold collision detection and emergency brake triggering
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

- **Low Latency**: Direct function calls instead of message passing
- **Performance**: Emergency brake response time improved
- **Type Safety**: Compile-time checking instead of runtime string parsing
- **Resource Efficient**: No additional threads or network sockets

### Distance Sensor Implementation
The Distance sensor provides robust collision detection with the following features:

- **CAN Message Processing**: Receives distance data via CAN bus from ultrasonic sensors
- **Multi-ID Support**: Handles CAN IDs 0x101, 0x181, and 0x581 for hardware compatibility
- **Distance Extraction**: Parses little-endian 16-bit distance values (in centimeters)
- **Risk Assessment**: Three-level collision risk system:
  - Level 0 (Safe): Distance > 25cm
  - Level 1 (Warning): Distance 20-25cm
  - Level 2 (Emergency): Distance < 20cm
- **Emergency Brake Triggering**: Automatic emergency brake activation at emergency threshold that allows reverse throttle
- **Thread Safety**: Atomic operations and mutex protection for concurrent access
- **Data Validation**: Checks CAN message length and distance range (0-100cm)
- **State Management**: Tracks emergency brake state to prevent duplicate triggers


### CAN Bus Integration
- **Singleton Pattern**: Centralized CAN message bus for efficient resource usage
- **Consumer Pattern**: Multiple sensors can subscribe to CAN messages
- **Thread Safety**: Atomic operations and mutex protection
- **Message Queuing**: Reliable message delivery with overflow protection
- **Statistics**: Message received/dispatched/dropped counters

### Intelligent Control
- **Fixed-threshold Collision Detection**: Simple and reliable distance-based collision detection
  - Emergency threshold: 20cm (immediate emergency brake activation)
  - Warning threshold: 25cm (collision warning alert)
  - Safe distance: >25cm (normal operation)
- **Multi-CAN ID Support**: Handles multiple CAN IDs (0x101, 0x181, 0x581) for crystal frequency tolerance
- **Emergency Brake Integration**: Direct callback-based emergency brake triggering for zero-latency response
- **Real-time Processing**: High-frequency sensor updates with thread-safe data handling

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
- **Comprehensive test suite** - 81.3% line coverage, 93.2% function coverage

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

### Test Coverage
The test suite achieves excellent coverage:
- **Overall line coverage**: 81.3% (998 of 1228 lines)
- **Overall function coverage**: 93.2% (124 of 133 functions)
- **Comprehensive component testing** across all sensor and control modules
- **High-quality test coverage** ensuring system reliability and maintainability

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
