# SEA:ME Instrument Cluster Display

A comprehensive digital instrument cluster system for automotive applications developed as part of the SEA:ME course. This project demonstrates the integration of sensor data collection, joystick controls, and a modern digital display for vehicle instrumentation.

## Project Overview

This system consists of three main components:

1. **Controller** - Handles joystick input for user interaction
2. **Middleware** - Collects and processes sensor data (speed, battery, etc.)
3. **ClusterDisplay** - Qt-based UI that displays the vehicle information

## Architecture

```
┌─────────────┐      ┌────────────┐      ┌────────────────┐
│ Controller  │────▶ │ Middleware │────▶│ ClusterDisplay │
└─────────────┘      └────────────┘      └────────────────┘
   Joystick           Data Processing      Qt-based UI
   Input              & Publishing         Display
```

- Communication between components uses the ZeroMQ messaging library
- Publisher-Subscriber pattern for efficient data transfer
- Modular design allows independent development and testing

## Building the Project

Prerequisites:
- C++17 compatible compiler
- CMake 3.10+
- ZeroMQ library
- Qt 5.9+ (for ClusterDisplay)
- pthread

Build steps:
```bash
# Clone the repository
git clone https://github.com/your-organization/Team06-SEAME-DES_Instrument-Cluster.git
cd Team06-SEAME-DES_Instrument-Cluster

# Build using the script
./build.sh

# For code coverage report
./build.sh --coverage
```

## Running the Application

After building:
```bash
# From the build/bin directory
./Middleware  # Start the middleware first
./ClusterDisplay  # Start the display
```

Connect a compatible joystick to interact with the system.

## Project Structure

- `/Controller` - Joystick interface implementation
- `/Middleware` - Sensor data collection and processing
- `/ClusterDisplay` - Qt/QML-based user interface
- `/zmq` - ZeroMQ integration files
- `/build` - Build output directory (created during build)

## Testing & Code Coverage

The project includes comprehensive unit tests for all components. Tests can be run using:

```bash
cd build
ctest
```

### Code Coverage Report

Code coverage reports can be generated to visualize test coverage:

```bash
./build.sh --coverage
```

This generates an HTML coverage report in `build/coverage/html/index.html` with:

- Line coverage statistics
- Branch coverage statistics
- Function coverage statistics
- Uncovered code highlighting

The coverage report helps identify areas of the codebase that need additional testing and ensures the reliability of the system.

| Component    | Coverage Metrics Tracked   |
|--------------|----------------------------|
| Controller   | Lines, branches, functions |
| Middleware   | Lines, branches, functions |
| ZMQ Library  | Lines, branches, functions |

For actual coverage percentages, run the coverage report generation command and view the detailed HTML report.

## Documentation

Each component has its own README with detailed information:
- [Controller Documentation](Controller/README.md)
- [Middleware Documentation](Middleware/README.md)
- [ClusterDisplay Documentation](ClusterDisplay/README.md)
- [ZeroMQ Library Documentation](zmq/README.md)
- [Middleware Tests Documentation](Middleware/test/README.md)
