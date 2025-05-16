# SEA:ME Instrument Cluster - Middleware

The Middleware component is the central data processing hub for the Instrument Cluster system. It collects sensor data, processes it, and publishes it to the display component using ZeroMQ.

## Features

- Real-time sensor data collection (speed, battery level, etc.)
- Data processing and normalization
- ZeroMQ-based publisher for efficient data distribution
- Modular design with separation of concerns
- Comprehensive test coverage

## Dependencies

- C++17 compatible compiler
- CMake 3.10+
- ZeroMQ library
- GoogleTest (for testing, automatically downloaded)
- pthread

## Building

```bash
# From the project root
mkdir -p build && cd build
cmake ..
make
```

Or use the main build script:

```bash
./build.sh
```

## Running

```bash
# From the build/bin directory
./Middleware
```

## Testing

Tests are built by default. To run the tests:

```bash
# From the build directory
ctest

# Or run individual tests
./bin/battery_test
./bin/sensor_handler_test
```

To disable building tests, configure with:

```bash
cmake -DBUILD_TESTS=OFF ..
```

## Code Structure

- `inc/`: Header files
  - Interfaces for sensors and publishers
  - Data structures for sensor readings
- `src/`: Implementation files
  - Concrete sensor implementations
  - ZeroMQ publishers
  - Main application logic
- `test/`: Unit tests

## Design for Testability

The codebase is designed with testability in mind:

1. **Dependency Injection**: Components accept their dependencies through constructors
2. **Interfaces**: Hardware components implement interfaces that can be mocked
3. **Mock Classes**: Mock implementations are provided for testing
4. **Unit Tests**: Tests verify component behavior in isolation

## Communication Protocol

The Middleware uses ZeroMQ's publisher-subscriber pattern to distribute data:

- Each sensor type has its own topic (e.g., "speed", "battery")
- Data is serialized before publishing
- Components can subscribe to specific topics they need

## Adding New Sensors

1. Define a new sensor interface or extend an existing one
2. Implement the sensor logic
3. Register the sensor with the main sensor handler
4. Add appropriate tests
