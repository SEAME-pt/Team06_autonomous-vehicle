#ifndef LANEKEEPINGHANDLER_HPP
#define LANEKEEPINGHANDLER_HPP

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

// Structure to represent lane keeping assistance data
struct LaneKeepingData {
  int lane_status; // 0=no deviation, 1=left deviation, 2=right deviation

  // Convert to sensor data string format
  std::string toString() const;

  // Create from received string
  static LaneKeepingData fromString(const std::string& data);
};

class LaneKeepingHandler {
public:
  explicit LaneKeepingHandler(const std::string &lkas_subscriber_address,
                             zmq::context_t &zmq_context,
                             std::shared_ptr<IPublisher> nc_publisher = nullptr,
                             bool test_mode = false,
                             const std::string &nc_address = "tcp://127.0.0.1:5556");
  ~LaneKeepingHandler();

  // Delete copy and move operations
  LaneKeepingHandler(const LaneKeepingHandler &) = delete;
  LaneKeepingHandler &operator=(const LaneKeepingHandler &) = delete;
  LaneKeepingHandler(LaneKeepingHandler &&) = delete;
  LaneKeepingHandler &operator=(LaneKeepingHandler &&) = delete;

  void start();
  void stop();

  // For testing - inject test data
  void setTestLaneKeepingData(const LaneKeepingData& data);

private:
  void receiveAndProcessLaneData();
  void processLaneKeepingData(const std::string& original_data, const LaneKeepingData& parsed_data);
  void publishLaneData(const std::string& original_data, const LaneKeepingData& parsed_data);

  std::atomic<bool> stop_flag;
  std::thread processing_thread;

  mutable std::mutex data_mutex;
  std::condition_variable data_cv;

  std::unique_ptr<ZmqSubscriber> lkas_subscriber;
  std::shared_ptr<IPublisher> nc_publisher;

  // Latest received data
  LaneKeepingData latest_data;
  bool has_new_data;

  bool _test_mode;

  static constexpr int processing_interval_ms = 50; // Process every 50ms
};

#endif
