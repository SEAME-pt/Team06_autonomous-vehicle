# Controller Component

A comprehensive input processing and communication component that handles joystick/gamepad input and translates it into vehicle control commands using ZeroMQ messaging.

## Overview

The Controller provides:
- **Joystick/Gamepad Input Processing** - Real-time input from Linux joystick devices
- **Control Command Translation** - Converts joystick input to throttle and steering commands
- **ZeroMQ Communication** - High-performance messaging to the Middleware component
- **Autonomous Mode Toggle** - Manual/autonomous driving mode switching
- **Emergency Stop Functionality** - Immediate vehicle stop capability
- **Smooth Control Profiles** - Gradual acceleration/deceleration for natural vehicle control

## Core Components

### Input Processing
- **`Controller`** - Main joystick input processing class
  - Linux joystick device interface (`/dev/input/js0`)
  - Real-time event processing with non-blocking I/O
  - Axis normalization and button state management
  - Support for up to 8 axes and 14 buttons

### Communication & Control
- **`ControlTransmitter`** - ZeroMQ communication and control logic
  - Message formatting and transmission
  - Throttle and steering command generation
  - Autonomous mode management
  - Emergency stop handling

## Key Features

### Joystick Control Mapping
- **Right Analog Stick (Y-axis)** - Throttle control (-100 to +100)
- **Left Analog Stick (X-axis)** - Steering control (-45° to +45°)
- **X Button** - Emergency stop (immediate throttle:0)
- **Y Button** - Toggle autonomous/manual mode
- **SELECT Button** - Quit application
- **START/HOME Buttons** - Reserved for future functionality

### Control Algorithms

#### Throttle Control
- **Gradual Acceleration**: Configurable acceleration rate (1.0f) for smooth starts
- **Gradual Deceleration**: Configurable deceleration rate (1.5f) for natural braking
- **Natural Decay**: 2% per cycle when no input (0.98 multiplier)
- **Full Range**: -100 to +100 for maximum power capability
- **Direction Inversion**: Corrected for intuitive forward/backward control

#### Steering Control
- **Exponential Sensitivity**: `pow(abs(input), 1.5) * 5.0` for precise control
- **Dead Zone**: 0.1 threshold to prevent drift
- **Fast Return**: 25% return rate for responsive steering
- **Angle Limiting**: -45° to +45° steering range
- **Smooth Transitions**: Gradual steering changes

#### Autonomous Mode
- **Toggle Functionality**: Y button press toggles between manual/autonomous
- **State Persistence**: Mode maintained until manually changed
- **Middleware Integration**: Sends mode status to Middleware for protection
- **Manual Override**: Manual control always available regardless of mode

### Message Protocol
The Controller sends structured messages to the Middleware:
```
throttle:<value>;steering:<angle>;auto_mode:<0|1>;
```

Example messages:
- `throttle:50;steering:15;` - 50% forward throttle, 15° right steering
- `throttle:0;steering:0;` - Stop command
- `throttle:-25;steering:-30;auto_mode:1;` - 25% reverse, 30° left, autonomous mode

## Dependencies

- **C++17 compiler** - Modern C++ features and standard library
- **CMake 3.10+** - Build system configuration
- **ZeroMQ library (libzmq)** - High-performance messaging
- **pthread** - POSIX threads for multi-threading
- **Custom ZeroMQ wrapper library** - Project-specific ZMQ utilities
- **Linux joystick support** - `/dev/input/js0` device interface

## Building

### Build as part of the main project:
```bash
./scripts/build.sh
```

### For Controller-specific development:
```bash
cd Controller
mkdir -p build && cd build
cmake ..
make
```

### Build with specific options:
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

## Directory Structure

```
Controller/
├── inc/                          # Header files
│   ├── Controller.hpp           # Joystick input processing interface
│   └── ControlTransmitter.hpp   # Communication and control logic
├── src/                         # Implementation files
│   ├── Controller.cpp           # Joystick device handling
│   ├── ControlTransmitter.cpp   # Control logic and messaging
│   └── main.cpp                 # Application entry point
├── CMakeLists.txt              # Build configuration
├── Makefile                    # Generated build files
├── cmake_install.cmake         # Installation configuration
└── README.md                   # This documentation
```

## Usage

### Basic Operation
```bash
# Start the controller application
./Controller

# The application will:
# 1. Test ZMQ connectivity to tcp://127.0.0.1:5557
# 2. Initialize joystick device (/dev/input/js0)
# 3. Start the control transmission loop
# 4. Process joystick input and send control commands
```

### Control Commands
- **Movement**: Use right analog stick for throttle control
- **Steering**: Use left analog stick for steering control
- **Emergency Stop**: Press X button for immediate stop
- **Mode Toggle**: Press Y button to toggle autonomous/manual mode
- **Quit**: Press SELECT button to exit application

## Integration

The Controller integrates with other components through:

### ZeroMQ Messaging
- **Publisher Pattern**: Sends control commands to Middleware
- **Address**: `tcp://127.0.0.1:5557` (configurable)
- **Message Format**: Structured control commands with semicolon delimiters
- **Error Handling**: Graceful degradation on communication failures

### Middleware Integration
- **Control Commands**: Throttle and steering values
- **Mode Status**: Autonomous/manual mode indication
- **Emergency Signals**: Immediate stop commands
- **Initialization**: Startup handshake with init message

## Configuration

### Control Parameters
The following parameters can be adjusted in `ControlTransmitter.hpp`:

```cpp
// Acceleration control
float _max_acceleration_rate = 1.0f;    // Throttle increase rate
float _max_deceleration_rate = 1.5f;    // Throttle decrease rate
float _brake_multiplier = 5.0f;         // Emergency brake force

// Natural decay rates
float throttle_decay = 0.98f;           // Throttle decay when no input
float steering_return = 0.25f;          // Steering return rate

// Control ranges
float max_throttle = 100.0f;            // Maximum throttle value
float max_steering_angle = 45.0f;       // Maximum steering angle
```

### Joystick Configuration
- **Device Path**: `/dev/input/js0` (configurable in `Controller.hpp`)
- **Axis Mapping**:
  - Axis 0: Left stick X (steering)
  - Axis 3: Right stick Y (throttle)
- **Button Mapping**: Defined constants for easy remapping

## Performance Characteristics

- **Input Processing**: Non-blocking I/O with 10ms select() timeout
- **Message Rate**: Continuous transmission during active control
- **Latency**: <1ms from joystick input to message transmission
- **CPU Usage**: Minimal with efficient event-driven processing
- **Memory Usage**: Fixed-size buffers for axes and buttons

## Error Handling

### Joystick Device Errors
- **Device Not Found**: Graceful error message and exit
- **Permission Denied**: Clear error indication
- **Device Disconnection**: Automatic detection and shutdown

### Communication Errors
- **ZMQ Connection Failure**: Test message validation on startup
- **Message Send Failure**: Error logging and graceful degradation
- **Network Issues**: Automatic reconnection attempts

### Input Validation
- **Axis Range Checking**: Bounds validation for all axes
- **Button State Validation**: Proper button press/release detection
- **Dead Zone Handling**: Prevents drift from small input values

## Troubleshooting

### Common Issues

#### Joystick Not Detected
```bash
# Check if device exists
ls -la /dev/input/js*

# Check permissions
sudo chmod 666 /dev/input/js0

# Test joystick manually
jstest /dev/input/js0
```

#### ZMQ Connection Issues
```bash
# Verify ZMQ is running
netstat -tlnp | grep 5557

# Test connectivity
telnet 127.0.0.1 5557
```

#### Control Not Responsive
- Check joystick calibration with `jstest`
- Verify axis mapping in code
- Check for conflicting input devices

### Debug Output
The Controller provides extensive debug output:
- Joystick device status
- Input event processing
- Control command generation
- ZMQ message transmission
- Error conditions and recovery

## Architecture Notes

The Controller follows several key design principles:
- **Event-Driven Processing**: Non-blocking I/O for responsive control
- **Separation of Concerns**: Input processing separate from communication
- **Configurable Parameters**: Easy tuning of control characteristics
- **Robust Error Handling**: Graceful degradation on failures
- **Real-Time Performance**: Optimized for low-latency control

This design ensures reliable, responsive vehicle control while maintaining clean separation between input processing and system communication.
