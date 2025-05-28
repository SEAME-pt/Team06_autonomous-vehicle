# Team06 Autonomous Vehicle Project

An autonomous vehicle system that combines instrument cluster display, lane detection, and object detection capabilities. The system provides both visual feedback through a modern digital dashboard and autonomous driving features.

## Project Overview

The system integrates several key components:

1. **Instrument Cluster** - Digital dashboard displaying vehicle telemetry and status
2. **Lane Detection** - Real-time lane detection and tracking
3. **Object Detection** - Detection and avoidance of obstacles
4. **Controller** - Handles vehicle control inputs
5. **Middleware** - Central data processing and distribution hub

## Architecture

```
┌─────────────┐     ┌────────────┐     ┌──────────────────┐
│ Controller  │────▶│ Middleware │────▶│ Cluster Display  │
└─────────────┘     └────────────┘     └──────────────────┘
                         │
                         │
              ┌─────────┴──────────┐
              ▼                    ▼
    ┌─────────────────┐  ┌─────────────────┐
    │ Lane Detection  │  │ Object Detection│
    └─────────────────┘  └─────────────────┘
```

- ZeroMQ-based communication between components
- Modular architecture with independent, replaceable components
- Real-time data processing and visualization

## Project Structure

- `/Controller` - Vehicle control input processing
- `/Middleware` - Central data hub and message broker
- `/modules` - Core autonomous driving modules
  - `cluster-display` - Digital instrument cluster interface
  - `lane-detection` - Lane detection and tracking
  - `object-detection` - Object detection and avoidance
- `/scripts` - Build and test automation
- `/zmq` - ZeroMQ communication layer

## Building

Prerequisites:
- C++17 compiler
- CMake 3.10+
- ZeroMQ (libzmq3-dev)
- Qt 5.9+ (for display interface)
- OpenCV (for computer vision)
- CUDA (for GPU acceleration)

Build steps:
```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y libzmq3-dev qt5-default libopencv-dev

# Initialize and update submodules
git submodule update --init --recursive

# Build the project
./scripts/build.sh
```

## Running

Start components in order:
```bash
./build/bin/Middleware
./build/bin/ClusterDisplay
./build/bin/Controller
```

## Testing

Run the test suite:
```bash
./scripts/run_tests.sh
```

## Continuous Integration

The project uses GitHub Actions for automated:
- Building on multiple platforms
- Running unit and integration tests
- Code coverage analysis
- Deployment to test environments

## Module Documentation

Each module maintains its own detailed documentation:
- [Controller](Controller/README.md)
- [Middleware](Middleware/README.md)
- [Cluster Display](modules/cluster-display/README.md)
- [Lane Detection](modules/lane-detection/README.md)
- [Object Detection](modules/object-detection/README.md)
