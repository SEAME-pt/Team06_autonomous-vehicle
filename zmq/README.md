# SEA:ME Instrument Cluster - ZeroMQ Library

This component provides a C++ wrapper around the ZeroMQ (ØMQ) messaging library for the Instrument Cluster system. It implements the publisher-subscriber pattern to facilitate communication between the different components of the system.

## Features

- Abstracted ZeroMQ functionality through clean interfaces
- Publisher implementation for sending data
- Subscriber implementation for receiving data
- Support for test mode to facilitate unit testing
- Thread-safe communication

## Classes

### `ZmqPublisher`

A class that implements the `IPublisher` interface to send messages through ZeroMQ sockets.

```cpp
// Create a publisher
zmq::context_t context;
ZmqPublisher publisher("tcp://*:5555", context);

// Send a message
publisher.send("Hello World");
```

### `ZmqSubscriber`

A class that implements the `ISubscriber` interface to receive messages from ZeroMQ sockets.

```cpp
// Create a subscriber
zmq::context_t context;
ZmqSubscriber subscriber("tcp://localhost:5555", context);

// Receive a message (with timeout in ms)
std::string message = subscriber.receive(1000);
```

## Test Mode

Both publisher and subscriber classes support a test mode that can be used for unit testing:

```cpp
// Create a subscriber in test mode
ZmqSubscriber subscriber("", context, true);

// Set a test message
subscriber.setTestMessage("Test Message");

// Receive the test message
std::string message = subscriber.receive();
```

## Dependencies

- C++17 compatible compiler
- CMake 3.10+
- ZeroMQ library (libzmq)
- cppzmq C++ binding

## Building

The ZeroMQ wrapper library is built as part of the main build process:

```bash
./build.sh
```

This produces a static library that is linked by other components of the system.

## Integration

To use this library in a component:

1. Include the appropriate header:
   ```cpp
   #include "ZmqPublisher.hpp"
   #include "ZmqSubscriber.hpp"
   ```

2. Link against the ZeroMQLib in CMakeLists.txt:
   ```cmake
   target_link_libraries(your_target PRIVATE ZeroMQLib)
   ```
