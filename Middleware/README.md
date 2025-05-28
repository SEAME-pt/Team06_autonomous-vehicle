# Middleware Component

A component that handles sensor data processing, control signals, and communication between system components.

## Overview

The Middleware provides:
- Sensor data collection and processing
- Control signal handling
- Data logging
- ZeroMQ-based communication

## Core Components

### Sensors
- `Battery` - Battery level monitoring
- `Speed` - Speed sensor handling
- `BackMotors` - Motor control and feedback
- `FServo` - Servo control interface
- `CanReader` - CAN bus communication

### Processing
- `SensorHandler` - Manages sensor data collection
- `ControlAssembly` - Processes control signals
- `BatteryReader` - Battery data processing

### Logging
- `SensorLogger` - Sensor data logging
- `ControlLogger` - Control signal logging

### Interfaces
- `ISensor` - Base sensor interface
- `IPublisher` - Publisher interface

## Dependencies

- C++17 compiler
- CMake 3.10+
- ZeroMQ library (libzmq)
- cppzmq headers
- pthread
- Our ZeroMQ wrapper library

## Building

Build as part of the main project:
```bash
./scripts/build.sh
```

For Middleware-specific development:
```bash
cd Middleware
mkdir -p build && cd build
cmake ..
make
```

## Directory Structure

- `inc/`
  - Core interfaces (`ISensor.hpp`, `IPublisher.hpp`)
  - Sensor headers (`Battery.hpp`, `Speed.hpp`, etc.)
  - Processing headers (`SensorHandler.hpp`, etc.)
  - Mock implementations for testing
- `src/` - Implementation files
- `test/` - Unit tests
- `CMakeLists.txt` - Build configuration

## Components

The Middleware consists of:
- Static library (`libmiddleware.a`)
- Main executable (`Middleware`)
- Test suite

## Testing

Tests are run as part of the main test suite:
```bash
./scripts/run_tests.sh
```

For Middleware-specific tests:
```bash
cd build
ctest
```

## Build Options

CMake options:
- `BUILD_TESTS` - Enable/disable test building (ON by default)
- `CODE_COVERAGE` - Enable coverage reporting (OFF by default)
