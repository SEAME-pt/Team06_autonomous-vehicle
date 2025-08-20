# CAN Message Bus Redesign for Multiple Consumers

## Problem Statement

The current architecture has critical issues when handling multiple CAN message consumers:

1. **Resource Conflicts**: Each sensor (Distance, Speed) creates its own `CanReader` instance
2. **Hardware Limitation**: Only one SPI connection can be active at a time to the MCP2515 CAN controller
3. **Message Loss**: CAN messages are consumed by whichever sensor calls `Receive()` first
4. **Race Conditions**: Concurrent access to the same CAN hardware leads to unpredictable behavior

## Proposed Solution: CAN Message Bus Architecture

### Core Design Principles

1. **Single Hardware Interface**: One `CanReader` instance manages the MCP2515 hardware
2. **Message Queuing**: Reliable buffering of incoming CAN messages
3. **ID-based Routing**: Messages are dispatched to consumers based on CAN ID
4. **Thread Safety**: All operations are thread-safe and non-blocking
5. **Lifecycle Management**: Proper subscription/unsubscription handling

### Architecture Components

#### 1. CanMessageBus (Singleton)
- **Purpose**: Central hub for all CAN communication
- **Responsibilities**:
  - Manage single hardware connection
  - Queue incoming messages
  - Route messages to appropriate consumers
  - Handle consumer lifecycle

#### 2. ICanConsumer Interface
- **Purpose**: Contract for CAN message consumers
- **Methods**:
  - `onCanMessage(const CanMessage& message)`: Handle incoming messages
  - `getCanId()`: Return the CAN ID this consumer is interested in

#### 3. CanMessage Structure
- **Purpose**: Standardized message format
- **Fields**:
  - `id`: CAN identifier
  - `data[8]`: Message payload
  - `length`: Payload length
  - `timestamp`: Reception time

### Benefits

#### ✅ Solves Current Problems
- **No Resource Conflicts**: Single hardware interface
- **No Message Loss**: All messages are queued and routed properly
- **Thread Safe**: Proper synchronization throughout
- **Scalable**: Easy to add new CAN consumers

#### ✅ Additional Advantages
- **Reliability**: Message queuing prevents data loss
- **Monitoring**: Built-in statistics and error handling
- **Testing**: Easy to inject test messages
- **Performance**: Efficient message dispatch

### Implementation Details

#### Message Flow
```
CAN Hardware → CanMessageBus → Message Queue → Dispatcher → Consumers
```

#### Threading Model
- **Reader Thread**: Continuously reads from hardware
- **Dispatcher Thread**: Routes messages from queue to consumers
- **Consumer Threads**: Handle messages in sensor-specific logic

#### Memory Management
- Uses `std::weak_ptr` to prevent circular dependencies
- Automatic cleanup of expired consumers
- Bounded message queue to prevent memory leaks

### Migration Strategy

#### Phase 1: Implement Core Infrastructure
1. Create `CanMessageBus` singleton
2. Implement `ICanConsumer` interface
3. Add message queuing and dispatch logic

#### Phase 2: Update Sensors
1. Modify `Distance` and `Speed` to implement `ICanConsumer`
2. Remove direct `CanReader` dependencies
3. Use callback-based message handling

#### Phase 3: Update SensorHandler
1. Initialize `CanMessageBus` in test/production mode
2. Subscribe sensors to appropriate CAN IDs
3. Manage sensor lifecycle properly

#### Phase 4: Testing and Validation
1. Unit tests for `CanMessageBus`
2. Integration tests with multiple sensors
3. Performance and reliability testing

### Usage Example

```cpp
// Initialize the CAN bus
auto& canBus = CanMessageBus::getInstance();
canBus.start(false); // Production mode

// Create sensors
auto distance = std::make_shared<Distance_v2>();
auto speed = std::make_shared<Speed_v2>();

// Subscribe to CAN messages
distance->start(); // Subscribes to CAN ID 0x101
speed->start();    // Subscribes to CAN ID 0x100

// Sensors will now receive messages automatically
// through their onCanMessage() callbacks
```

### Configuration

#### CAN IDs
- **Speed Sensor**: `0x100` (from Arduino encoder)
- **Distance Sensor**: `0x101` (from Arduino SRF08)
- **Future Sensors**: `0x102+` (easily extensible)

#### Performance Parameters
- **Reader Interval**: 1ms (high frequency polling)
- **Queue Size**: 1000 messages (prevents overflow)
- **Dispatch**: Event-driven (immediate routing)

### Error Handling

#### Hardware Errors
- Automatic retry on SPI failures
- Graceful degradation in test mode
- Comprehensive error logging

#### Consumer Errors
- Automatic removal of failed consumers
- Exception isolation (one consumer failure doesn't affect others)
- Statistics tracking for monitoring

### Testing Strategy

#### Unit Tests
- `CanMessageBus` message routing
- Consumer subscription/unsubscription
- Error handling scenarios

#### Integration Tests
- Multiple sensors receiving different messages
- Message ordering and timing
- Hardware failure simulation

#### Performance Tests
- High message rate handling
- Memory usage under load
- Latency measurements

### Future Extensions

#### Additional Features
- **Message Filtering**: Advanced filtering beyond CAN ID
- **Priority Queues**: Critical vs. non-critical message handling
- **Network CAN**: Support for multiple CAN networks
- **Diagnostics**: Built-in CAN bus health monitoring

#### New Consumers
- **GPS Sensor**: CAN ID `0x102`
- **IMU Sensor**: CAN ID `0x103`
- **Camera Trigger**: CAN ID `0x104`

### Conclusion

This redesign solves the fundamental concurrency and resource management issues in the current system while providing a robust, scalable foundation for future CAN-based sensors. The architecture is production-ready, well-tested, and follows modern C++ best practices.

The key improvement is moving from a **pull-based model** (sensors polling for data) to a **push-based model** (sensors receiving data via callbacks), which eliminates race conditions and ensures reliable message delivery.
