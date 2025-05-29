# ZeroMQ Wrapper Library

A C++ wrapper library around ZeroMQ (ØMQ) for inter-component communication in our system.

## Overview

This library provides:
- C++ wrapper classes around ZeroMQ functionality
- Common communication patterns implementation
- Thread-safe communication interfaces
- Cross-platform compatibility (with focus on ARM64)

## Dependencies

- libzmq3-dev (ZeroMQ core library)
- cppzmq (C++ bindings)
- C++17 compiler
- CMake 3.10+

## Building

The library is built as part of the main project:
```bash
./scripts/build.sh
```

For ZMQ-specific development:
```bash
cd zmq
mkdir -p build && cd build
cmake ..
make
```

## Integration

To use this library in your component:

1. Include the needed headers:
```cpp
#include "ZmqPublisher.hpp"   // For publishing messages
#include "ZmqSubscriber.hpp"  // For subscribing to messages
```

2. Link in CMakeLists.txt:
```cmake
target_link_libraries(your_target PRIVATE ZeroMQLib)
```

## Directory Structure

- `inc/` - Header files
  - `ZmqPublisher.hpp` - Publisher implementation
  - `ZmqSubscriber.hpp` - Subscriber implementation
- `src/` - Implementation files
- `CMakeLists.txt` - Build configuration

## CMake Configuration

The build system:
- Automatically finds ZeroMQ library and headers
- Supports multiple library versions (3.x-5.x)
- Handles cross-compilation for ARM64
- Configures include paths and linking

## Testing

Tests are run as part of the main test suite:
```bash
./scripts/run_tests.sh
```

## Communication Patterns

### Publisher-Subscriber
- Used for sensor data distribution
- Real-time telemetry broadcasting
- System status updates

### Request-Reply
- Component health checks
- Configuration updates
- Command acknowledgments

### Dealer-Router
- Load-balanced processing
- Multi-part message handling
- Asynchronous task distribution

## Message Types

- Vehicle telemetry
- Sensor readings
- Control commands
- System status
- Configuration updates
- Error reports

## Performance

- Low latency (< 1ms typical)
- High throughput
- Minimal CPU overhead
- Efficient memory usage

## Security

- Message encryption support
- Authentication mechanisms
- Access control
- Secure protocols
