# Controller Component

A component that handles input processing and communication with the system using ZeroMQ.

## Overview

The Controller consists of:
- `Controller` - Main input processing class
- `ControlTransmitter` - Handles ZeroMQ communication

## Dependencies

- C++17 compiler
- CMake 3.10+
- ZeroMQ library (libzmq)
- pthread
- Our ZeroMQ wrapper library

## Building

Build as part of the main project:
```bash
./scripts/build.sh
```

For Controller-specific development:
```bash
cd Controller
mkdir -p build && cd build
cmake ..
make
```

## Directory Structure

- `inc/`
  - `Controller.hpp` - Main controller interface
  - `ControlTransmitter.hpp` - Communication interface
- `src/`
  - `Controller.cpp` - Input processing implementation
  - `ControlTransmitter.cpp` - Communication implementation
  - `main.cpp` - Program entry point
- `CMakeLists.txt` - Build configuration

## Integration

The Controller integrates with other components through:
- ZeroMQ messaging (via ControlTransmitter)
- Event-based input processing
- Thread-safe interfaces

## Testing

Tests are run as part of the main test suite:
```bash
./scripts/run_tests.sh
```

## Configuration

Control parameters are configurable through:
- Configuration files
- Runtime commands
- Calibration interface

## Code Structure

- `inc/`: Header files
  - Controller class definition
  - Joystick event structures
- `src/`: Implementation files
- `CMakeLists.txt`: Build configuration
