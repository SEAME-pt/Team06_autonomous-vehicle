# Lane Keeping Handler Integration

## Overview

The `LaneKeepingHandler` has been successfully integrated into the main autonomous vehicle application. It receives lane keeping assistance data from an AI model via ZeroMQ and forwards this information to the cluster display system.

## Architecture

```
AI Lane Keeping Model  →  LaneKeepingHandler  →  Cluster Display
     (tcp:5558)              (Middleware)         (tcp:5556)
```

## Integration Details

### 1. ZeroMQ Addresses

- **Lane Keeping AI Input**: `tcp://127.0.0.1:5558`
  - The AI model publishes lane keeping data here
  - Handler subscribes to this address

- **Cluster Output**: `tcp://127.0.0.1:5556` (non-critical channel)
  - Handler publishes processed data here
  - Cluster display subscribes to this address

### 2. Data Format

The handler supports the following data format:

**Input from AI Model:**
- `lane:0;` - Vehicle is properly positioned in lane
- `lane:1;` - Vehicle is deviating left (crossing left line)
- `lane:2;` - Vehicle is deviating right (crossing right line)

**Output to Cluster:**
- Same format as input, ensuring semicolon termination
- Example: `lane:1;` for left deviation

### 3. Integration Points

The `LaneKeepingHandler` is integrated into `main.cpp` alongside other system components:

1. **Initialization**: Created after `ControlAssembly`
2. **Startup**: Started after other components
3. **Shutdown**: Stopped and cleaned up during graceful shutdown

## Usage

### Running the Main Application

```bash
# Build the project
cd /home/jetson/Team06_autonomous-vehicle
mkdir -p build && cd build
cmake ..
make

# Run the main application
./bin/Middleware
```

The application will output:
```
Initializing sensor handler...
Initializing control assembly...
Initializing lane keeping handler...
Starting sensor handler...
Starting control assembly...
Starting lane keeping handler...
System running. Press Ctrl+C to stop.
```

### Testing the Integration

A test program is provided to demonstrate the integration:

```bash
# Compile the test (from project root)
g++ -std=c++17 -I. -I/usr/include test_lane_integration.cpp \
    Middleware/src/LaneKeepingHandler.cpp \
    -lzmq -lpthread -o test_lane_integration

# Run the test
./test_lane_integration
```

## AI Model Integration

Your AI model should:

1. **Connect as Publisher**: Bind to `tcp://*:5558`
2. **Send Data Format**: Use `lane:X;` format where X is 0, 1, or 2
3. **Continuous Updates**: Send updates as lane status changes

Example AI model publisher code:
```cpp
#include <zmq.hpp>
#include <string>

zmq::context_t context(1);
zmq::socket_t publisher(context, zmq::socket_type::pub);
publisher.bind("tcp://*:5558");

// Send lane status
std::string message = "lane:1;";  // Left deviation
zmq::message_t zmq_msg(message.data(), message.size());
publisher.send(zmq_msg, zmq::send_flags::none);
```

## Cluster Display Integration

The cluster display should subscribe to the non-critical channel (`tcp://127.0.0.1:5556`) to receive:
- Sensor data from `SensorHandler`
- Lane keeping data from `LaneKeepingHandler`
- Other non-critical system information

## Thread Safety

The `LaneKeepingHandler` is fully thread-safe:
- Uses atomic flags for stop conditions
- Mutex protection for shared data
- Condition variables for efficient waiting
- Safe startup/shutdown procedures

## Error Handling

The handler includes comprehensive error handling:
- ZMQ connection errors are caught and logged
- Invalid data formats default to "no deviation" (lane:0)
- Processing thread continues running despite individual message errors
- Graceful shutdown on system termination

## Monitoring

The handler provides console output for monitoring:
- Startup/shutdown messages
- Data processing logs showing received and parsed data
- Error messages for debugging
- Publishing confirmation messages

Example output:
```
Starting Lane Keeping Handler...
Lane Keeping Handler started successfully.
Processing lane keeping data: received='lane:1;' parsed_status=1
Publishing lane data: lane:1; (internal status=1)
```

## Configuration

Key configuration parameters in `LaneKeepingHandler.hpp`:
- `processing_interval_ms = 50`: Processing frequency (50ms = 20Hz)
- Addresses are configurable in `main.cpp`
- Test mode can be enabled for unit testing

## Files Modified/Added

### New Files:
- `Middleware/inc/LaneKeepingHandler.hpp`
- `Middleware/src/LaneKeepingHandler.cpp`

### Modified Files:
- `Middleware/src/main.cpp` - Integration with main application

### Test Files:
- `test_lane_integration.cpp` - Integration test program
- `LANE_KEEPING_INTEGRATION.md` - This documentation

The integration is complete and ready for production use with your AI lane keeping model.
