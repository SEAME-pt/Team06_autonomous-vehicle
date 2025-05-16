# SEA:ME Instrument Cluster Display

A comprehensive digital instrument cluster system for automotive applications developed as part of the SEA:ME course. This project demonstrates the integration of sensor data collection, joystick controls, and a modern digital display for vehicle instrumentation.

![CI/CD Pipeline](https://github.com/your-organization/ClusterDisplay/actions/workflows/ci-cd-with-tests.yml/badge.svg)

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
- ZeroMQ library (libzmq3-dev)
- ZeroMQ C++ bindings (cppzmq)
- Qt 5.9+ (for ClusterDisplay)
- pthread

Build steps:
```bash
# Clone the repository
git clone https://github.com/your-organization/Team06-SEAME-DES_Instrument-Cluster.git
cd Team06-SEAME-DES_Instrument-Cluster

# The run_tests.sh script will automatically install dependencies
# Or you can install them manually:
# sudo apt-get install -y libzmq3-dev
# and follow instructions at https://github.com/zeromq/cppzmq to install cppzmq

# Build using the script
./build.sh

# For code coverage report
./build.sh --coverage
```

### Using Docker for Development

We provide a pre-configured Docker image with all dependencies pre-installed, which ensures consistency between local development and CI/CD:

```bash
# Pull the Docker image
docker pull jmoreiraseame/jetson-nano-ubuntu:bionic

# Run a container with the current directory mounted
docker run -it --rm --platform linux/arm64 -v $(pwd):/app -w /app jmoreiraseame/jetson-nano-ubuntu:bionic bash

# Inside the container, you can build and test
mkdir -p build && cd build
cmake .. -DCODE_COVERAGE=ON
make
cd bin && ./battery_test  # Run a specific test

# Or use the run_tests.sh script to run all tests
cd /app && ./run_tests.sh
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
# Run all tests
./run_tests.sh

# From the build directory
cd build
ctest
```

### Continuous Integration/Continuous Deployment

This project uses GitHub Actions for a comprehensive CI/CD pipeline:

- **Build**: Compiles the code with test and coverage options
- **Test**: Runs all unit tests and generates test reports
- **Coverage**: Generates code coverage reports
- **Deploy**: Deploys to target hardware if tests pass (main/dev branches only)

The pipeline prevents deployment of broken code by ensuring that all tests pass before deployment occurs.

You can view the latest pipeline results in the [GitHub Actions tab](https://github.com/your-organization/ClusterDisplay/actions).

### Code Coverage Report

Code coverage reports can be generated to visualize test coverage:

```bash
./build.sh --coverage
```

This generates an HTML coverage report in `build/coverage/html/index.html` with:

- Line coverage statistics
- Function coverage statistics
- Uncovered code highlighting

The coverage report helps identify areas of the codebase that need additional testing and ensures the reliability of the system.


## Documentation

Each component has its own README with detailed information:
- [Controller Documentation](Controller/README.md)
- [Middleware Documentation](Middleware/README.md)
- [ClusterDisplay Documentation](ClusterDisplay/README.md)
- [ZeroMQ Library Documentation](zmq/README.md)
- [Middleware Tests Documentation](Middleware/test/README.md)
