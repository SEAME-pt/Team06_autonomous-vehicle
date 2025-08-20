#ifndef CANMESSAGEBUS_HPP
#define CANMESSAGEBUS_HPP

#include "CanReader.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

struct CanMessage {
    uint16_t id;
    uint8_t data[8];
    uint8_t length;
    std::chrono::steady_clock::time_point timestamp;

    CanMessage(uint16_t id, const uint8_t* data, uint8_t len)
        : id(id), length(len), timestamp(std::chrono::steady_clock::now()) {
        std::memcpy(this->data, data, std::min(len, (uint8_t)8));
    }
};

class ICanConsumer {
public:
    virtual ~ICanConsumer() = default;
    virtual void onCanMessage(const CanMessage& message) = 0;
    virtual uint16_t getCanId() const = 0;
};

class CanMessageBus {
public:
    static CanMessageBus& getInstance();

    // Consumer management
    void subscribe(std::shared_ptr<ICanConsumer> consumer);
    void subscribeToMultipleIds(std::shared_ptr<ICanConsumer> consumer, const std::vector<uint16_t>& canIds);
    void unsubscribe(uint16_t canId);

    // Message sending
    bool send(uint16_t canId, const uint8_t* data, uint8_t length);

    // Lifecycle management
    bool start(bool test_mode = false);
    void stop();
    bool isRunning() const { return running.load(); }

    // For testing
    void injectTestMessage(const CanMessage& message);

private:
    CanMessageBus() = default;
    ~CanMessageBus();

    // Delete copy and move operations (singleton)
    CanMessageBus(const CanMessageBus&) = delete;
    CanMessageBus& operator=(const CanMessageBus&) = delete;
    CanMessageBus(CanMessageBus&&) = delete;
    CanMessageBus& operator=(CanMessageBus&&) = delete;

    void readerThread();
    void dispatcherThread();
    void dispatchMessage(const CanMessage& message);

    // Hardware interface
    std::unique_ptr<CanReader> hardware_reader;

    // Threading
    std::thread reader_thread;
    std::thread dispatcher_thread;
    std::atomic<bool> running{false};
    std::atomic<bool> test_mode{false};

    // Consumer management
    std::mutex consumers_mutex;
    std::unordered_map<uint16_t, std::vector<std::weak_ptr<ICanConsumer>>> consumers;

    // Message queuing for reliability
    std::queue<CanMessage> message_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;

    // Statistics and monitoring
    std::atomic<uint64_t> messages_received{0};
    std::atomic<uint64_t> messages_dispatched{0};
    std::atomic<uint64_t> messages_dropped{0};

    static constexpr size_t MAX_QUEUE_SIZE = 1000;
    static constexpr int READER_INTERVAL_MS = 1;
};

#endif
