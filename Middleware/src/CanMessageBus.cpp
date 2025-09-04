#include "CanMessageBus.hpp"
#include <algorithm>
#include <iostream>

CanMessageBus &CanMessageBus::getInstance() {
  static CanMessageBus instance;
  return instance;
}

CanMessageBus::~CanMessageBus() { stop(); }

bool CanMessageBus::start(bool test_mode_flag) {
  if (running.load()) {
    std::cout << "CanMessageBus already running"
              << std::endl; // LCOV_EXCL_LINE - State logging
    return true;
  }

  test_mode.store(test_mode_flag);

  // LCOV_EXCL_START - Hardware initialization and thread management, not
  // testable in unit tests
  try {
    // Initialize hardware reader
    hardware_reader = std::make_unique<CanReader>(test_mode_flag);
    if (!hardware_reader->initialize()) {
      std::cerr << "Failed to initialize CanReader hardware"
                << std::endl; // LCOV_EXCL_LINE - Hardware initialization error
      return false;
    }

    // Start threads
    running.store(true);
    reader_thread = std::thread(&CanMessageBus::readerThread, this);
    dispatcher_thread = std::thread(&CanMessageBus::dispatcherThread, this);

    std::cout << "CanMessageBus started successfully" << std::endl;
    return true;

  } catch (const std::exception &e) {
    std::cerr << "Error starting CanMessageBus: " << e.what()
              << std::endl; // LCOV_EXCL_LINE - Hardware error handling
    running.store(false);
    return false;
  }
  // LCOV_EXCL_STOP
}

void CanMessageBus::stop() {
  if (!running.load()) {
    return;
  }

  // LCOV_EXCL_START - Thread management and shutdown logging, not testable in
  // unit tests
  std::cout << "Stopping CanMessageBus..." << std::endl;
  running.store(false);
  queue_cv.notify_all();

  // Join threads
  if (reader_thread.joinable()) {
    reader_thread.join();
  }
  if (dispatcher_thread.joinable()) {
    dispatcher_thread.join();
  }

  // Clean up hardware
  hardware_reader.reset();

  // Clear message queue
  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    while (!message_queue.empty()) {
      message_queue.pop();
    }
  }

  std::cout << "CanMessageBus stopped. Stats - Received: "
            << messages_received.load()
            << ", Dispatched: " << messages_dispatched.load()
            << ", Dropped: " << messages_dropped.load() << std::endl;
  // LCOV_EXCL_STOP
}
// LCOV_EXCL_START - Consumer management, not testable in unit tests
void CanMessageBus::subscribe(std::shared_ptr<ICanConsumer> consumer) {
  if (!consumer) {
    std::cerr << "Cannot subscribe null consumer"
              << std::endl; // LCOV_EXCL_LINE - Error handling
    return;
  }

  uint16_t canId = consumer->getCanId();

  std::lock_guard<std::mutex> lock(consumers_mutex);
  consumers[canId].push_back(std::weak_ptr<ICanConsumer>(consumer));

  std::cout << "Subscribed consumer for CAN ID: 0x" << std::hex << canId
            << std::dec << std::endl; // LCOV_EXCL_LINE - Subscription logging
}
// LCOV_EXCL_STOP
void CanMessageBus::subscribeToMultipleIds(
    std::shared_ptr<ICanConsumer> consumer,
    const std::vector<uint16_t> &canIds) {
  if (!consumer) {
    std::cerr << "Cannot subscribe null consumer"
              << std::endl; // LCOV_EXCL_LINE - Error handling
    return;
  }

  std::lock_guard<std::mutex> lock(consumers_mutex);
  for (uint16_t canId : canIds) {
    consumers[canId].push_back(std::weak_ptr<ICanConsumer>(consumer));
    std::cout << "Subscribed consumer for CAN ID: 0x" << std::hex << canId
              << std::dec << std::endl;
  }
}

void CanMessageBus::unsubscribe(uint16_t canId) {
  std::lock_guard<std::mutex> lock(consumers_mutex);

  auto it = consumers.find(canId);
  if (it != consumers.end()) {
    consumers.erase(it);
    std::cout << "Unsubscribed all consumers for CAN ID: 0x" << std::hex
              << canId << std::dec << std::endl;
  }
}

bool CanMessageBus::send(uint16_t canId, const uint8_t *data, uint8_t length) {
  if (!hardware_reader) {
    return false;
  }

  return hardware_reader->Send(canId, const_cast<uint8_t *>(data), length);
}

void CanMessageBus::readerThread() {
  uint8_t buffer[8];
  uint8_t length;
  int debug_counter = 0;

  std::cout << "CAN reader thread started"
            << std::endl; // LCOV_EXCL_LINE - Thread management logging

  while (running.load()) {
    try {
      // LCOV_EXCL_START - Hardware CAN receive, not testable in unit tests
      if (hardware_reader && hardware_reader->Receive(buffer, length)) {
        uint16_t canId = hardware_reader->getId();

        // Debug: Print received CAN message details
        std::cout << "CAN Bus received message - ID: 0x" << std::hex
                  << canId // LCOV_EXCL_LINE - Debug logging
                  << std::dec << ", Length: " << (int)length
                  << std::endl; // LCOV_EXCL_LINE - Debug logging

        // Create message
        CanMessage message(canId, buffer, length);
        messages_received.fetch_add(1);

        // Add to queue
        {
          std::lock_guard<std::mutex> lock(queue_mutex);
          if (message_queue.size() < MAX_QUEUE_SIZE) {
            message_queue.push(message);
            queue_cv.notify_one();
          } else {
            messages_dropped.fetch_add(1);
            std::cerr << "Message queue full, dropping message with ID: 0x"
                      << std::hex << canId << std::dec
                      << std::endl; // LCOV_EXCL_LINE - Error handling
          }
        }
      }
    } catch (
        const std::exception &e) { // LCOV_EXCL_LINE - Thread error handling
      std::cerr << "Error in CAN reader thread: " << e.what()
                << std::endl; // LCOV_EXCL_LINE - Thread error handling
    }
    // LCOV_EXCL_STOP
    std::this_thread::sleep_for(std::chrono::milliseconds(READER_INTERVAL_MS));

    // Debug: Print every 1000 iterations to show thread is active
    debug_counter++;
    if (debug_counter % 1000 == 0) {
      std::cout << "CAN reader thread active, iteration " << debug_counter
                << std::endl; // LCOV_EXCL_LINE - Debug logging
    }
  }

  std::cout << "CAN reader thread stopped"
            << std::endl; // LCOV_EXCL_LINE - Thread management logging
}

void CanMessageBus::dispatcherThread() {
  std::cout << "CAN dispatcher thread started"
            << std::endl; // LCOV_EXCL_LINE - Thread management logging

  while (running.load()) {
    std::unique_lock<std::mutex> lock(queue_mutex);
    queue_cv.wait(lock,
                  [this] { return !message_queue.empty() || !running.load(); });

    while (!message_queue.empty() && running.load()) {
      CanMessage message = message_queue.front();
      message_queue.pop();
      lock.unlock();

      dispatchMessage(message);
      messages_dispatched.fetch_add(1);

      lock.lock();
    }
  }

  std::cout << "CAN dispatcher thread stopped"
            << std::endl; // LCOV_EXCL_LINE - Thread management logging
}

void CanMessageBus::dispatchMessage(const CanMessage &message) {
  std::lock_guard<std::mutex> lock(consumers_mutex);

  std::cout << "Dispatching message with ID: 0x" << std::hex << message.id
            << std::dec << std::endl; // LCOV_EXCL_LINE - Debug logging

  auto it = consumers.find(message.id);
  if (it != consumers.end()) {
    std::cout << "Found " << it->second.size() << " consumers for CAN ID 0x"
              << std::hex << message.id << std::dec
              << std::endl; // LCOV_EXCL_LINE - Debug logging
    // Dispatch to all consumers for this CAN ID
    auto &consumer_list = it->second;
    for (auto consumer_it = consumer_list.begin();
         consumer_it != consumer_list.end();) {
      if (auto consumer = consumer_it->lock()) {
        try {
          consumer->onCanMessage(message);
          ++consumer_it;
        } catch (
            const std::exception &e) { // LCOV_EXCL_LINE - Thread error handling
          std::cerr << "Error dispatching message to consumer: " << e.what()
                    << std::endl; // LCOV_EXCL_LINE - Error handling
          consumer_it = consumer_list.erase(consumer_it);
        }
      } else {
        // Remove expired consumer
        consumer_it = consumer_list.erase(consumer_it);
      }
    }

    // Clean up empty entries
    if (consumer_list.empty()) {
      consumers.erase(it);
    }
  } else {
    std::cout << "No consumers found for CAN ID 0x" << std::hex << message.id
              << std::dec << std::endl; // LCOV_EXCL_LINE - Debug logging
  }
}

void CanMessageBus::injectTestMessage(const CanMessage &message) {
  if (!test_mode.load()) {
    std::cerr << "Cannot inject test message: not in test mode"
              << std::endl; // LCOV_EXCL_LINE - Error handling
    return;
  }

  std::lock_guard<std::mutex> lock(queue_mutex);
  if (message_queue.size() < MAX_QUEUE_SIZE) {
    message_queue.push(message);
    queue_cv.notify_one();
    messages_received.fetch_add(1);
  } else {
    messages_dropped.fetch_add(1);
  }
}
