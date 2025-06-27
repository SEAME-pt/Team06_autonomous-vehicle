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
  - `build.sh` - Main build script
  - `run_tests.sh` - Test execution script
  - `run_linters.sh` - Code quality checks
  - `run_coverage.sh` - Coverage report generation
- `/zmq` - ZeroMQ communication layer
- `/CICD` - Continuous Integration configuration

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

## Code Quality

### Linting

Run the linter to check code quality:
```bash
# Check for formatting issues only (default, safe for CI)
./scripts/run_linters.sh

# Fix formatting issues automatically
./scripts/run_linters.sh --fix

# Run full static analysis (clang-tidy) for local development
./scripts/run_linters.sh --with-tidy

# Combine flags: fix formatting and run static analysis
./scripts/run_linters.sh --fix --with-tidy
```

The linter checks:
- **Code formatting** (clang-format) - Always enabled
- **Static analysis** (clang-tidy) - Enabled with `--with-tidy` flag

**Note**: By default, the linter runs in format-only mode to ensure compatibility with CI environments. Use `--with-tidy` for comprehensive static analysis during local development.

### Testing

Run the test suite:
```bash
./scripts/run_tests.sh
```

### Test Coverage

To generate and view test coverage reports:
```bash
# Generate basic coverage report
./scripts/run_coverage.sh

# Generate HTML coverage report
./scripts/run_coverage.sh --html

# Specify custom output directory
./scripts/run_coverage.sh --output=./my-coverage-reports --html
```

The coverage report provides detailed information about:
- Line coverage
- Branch coverage
- Function coverage
- Uncovered code regions

#### Accessing CI Coverage Reports

Coverage reports from CI runs are available:
1. Go to your GitHub repository
2. Click on "Actions"
3. Select a workflow run
4. Scroll to "Artifacts"
5. Download "coverage-reports-[run-number]"

## Continuous Integration

The project uses GitHub Actions for automated:
- Code linting and style checking
- Building on multiple platforms
- Running unit and integration tests
- Code coverage analysis
- Deployment to Jetson Nano hardware

See [CICD Documentation](CICD/README.md) for details.

## Module Documentation

Each module maintains its own detailed documentation:
- [Controller](Controller/README.md)
- [Middleware](Middleware/README.md)
- [Cluster Display](https://github.com/SEAME-pt/Team06_DES_Instrument-Cluster/blob/main/README.md)
- [Lane Detection](https://github.com/SEAME-pt/Team06_ADS_Autonomous-Lane-Detection/blob/main/README.md)
- [Object Detection](https://github.com/SEAME-pt/Team06_ADS_Object-Detection-and-Avoidance/blob/main/README.md)
