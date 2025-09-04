#ifndef TRAFFICSIGNHANDLER_HPP
#define TRAFFICSIGNHANDLER_HPP

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <zmq.hpp>

// Forward declarations
class IPublisher;
class ZmqSubscriber;

class TrafficSignHandler {
public:
  explicit TrafficSignHandler(
      const std::string &traffic_sign_subscriber_address,
      zmq::context_t &zmq_context,
      std::shared_ptr<IPublisher> nc_publisher = nullptr,
      bool test_mode = false);
  ~TrafficSignHandler();

  // Delete copy and move operations
  TrafficSignHandler(const TrafficSignHandler &) = delete;
  TrafficSignHandler &operator=(const TrafficSignHandler &) = delete;
  TrafficSignHandler(TrafficSignHandler &&) = delete;
  TrafficSignHandler &operator=(TrafficSignHandler &&) = delete;

  void start();
  void stop();

  // For testing - inject test data
  void setTestTrafficSignData(const std::string &data);

private:
  void receiveAndProcessTrafficSignData();
  void processTrafficSignMessage(const std::string &data);

  std::atomic<bool> stop_flag;
  std::thread processing_thread;

  std::unique_ptr<ZmqSubscriber> traffic_sign_subscriber;
  std::shared_ptr<IPublisher> nc_publisher;

  bool _test_mode;

  // Map of publishable traffic signs: key = sign name, value = speed limit
  static const std::unordered_map<std::string, std::string> publishable_signs;

  static constexpr int processing_interval_ms = 50; // Process every 50ms
};

#endif
