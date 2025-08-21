#ifndef TRAFFICSIGNHANDLER_HPP
#define TRAFFICSIGNHANDLER_HPP

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <zmq.hpp>

// Forward declarations
class IPublisher;
class ZmqSubscriber;

// Enumeration for traffic sign types
enum class TrafficSignType {
  NONE = 0,
  STOP = 1,
  SPEED_50 = 2,
  SPEED_80 = 3,
  CROSSWALK = 4
};

// Structure to represent traffic sign data
struct TrafficSignData {
  TrafficSignType sign_type; // Type of traffic sign detected

  // Convert to sensor data string format
  std::string toString() const;

  // Create from received string
  static TrafficSignData fromString(const std::string& data);

  // Helper method to get string representation of sign type
  std::string getSignTypeString() const;

  // Helper method to convert string to sign type
  static TrafficSignType stringToSignType(const std::string& sign_str);
};

class TrafficSignHandler {
public:
  explicit TrafficSignHandler(const std::string &traffic_sign_subscriber_address,
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
  void setTestTrafficSignData(const TrafficSignData& data);

private:
  void receiveAndProcessTrafficSignData();
  void processTrafficSignData(const std::string& original_data, const TrafficSignData& parsed_data);
  void publishTrafficSignData(const std::string& original_data, const TrafficSignData& parsed_data);

  std::atomic<bool> stop_flag;
  std::thread processing_thread;

  mutable std::mutex data_mutex;
  std::condition_variable data_cv;

  std::unique_ptr<ZmqSubscriber> traffic_sign_subscriber;
  std::shared_ptr<IPublisher> nc_publisher;

  // Latest received data
  TrafficSignData latest_data;
  bool has_new_data;

  bool _test_mode;

  static constexpr int processing_interval_ms = 50; // Process every 50ms
};

#endif
