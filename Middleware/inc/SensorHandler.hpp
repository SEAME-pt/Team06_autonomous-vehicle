#ifndef SENSORHANDLER_HPP
#define SENSORHANDLER_HPP

#include "Battery.hpp"
#include "CanMessageBus.hpp"
#include "Distance.hpp"
#include "ISensor.hpp"
#include "SensorLogger.hpp"
#include "Speed.hpp"
#include "ZmqPublisher.hpp"
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <zmq.hpp>

class SensorHandler {
public:
  explicit SensorHandler(const std::string &zmq_c_address,
                         const std::string &zmq_nc_address,
                         zmq::context_t &zmq_context,
                         std::shared_ptr<IPublisher> c_publisher = nullptr,
                         std::shared_ptr<IPublisher> nc_publisher = nullptr,
                         bool use_real_sensors = true,
                         const std::string &zmq_cluster_address = "tcp://127.0.0.1:5555");
  ~SensorHandler();

  // Delete copy and move operations
  SensorHandler(const SensorHandler &) = delete;
  SensorHandler &operator=(const SensorHandler &) = delete;
  SensorHandler(SensorHandler &&) = delete;
  SensorHandler &operator=(SensorHandler &&) = delete;

  void start();
  void stop();

  // For testing
  void addSensor(const std::string &name, std::shared_ptr<ISensor> sensor);
  std::unordered_map<std::string, std::shared_ptr<ISensor>> getSensors() const;

private:
  void addSensors();
  void sortSensorData();
  void readSensors();
  void publishCritical();
  void publishNonCritical();
  void publishSensorData(const std::shared_ptr<SensorData> &sensorData);

  std::atomic<bool> stop_flag;
  std::thread critical_thread;
  std::thread non_critical_thread;
  std::thread sensor_read_thread;

  mutable std::mutex sensors_mutex;
  mutable std::mutex critical_mutex;
  mutable std::mutex non_critical_mutex;
  std::condition_variable data_cv;

  std::unordered_map<std::string, std::shared_ptr<ISensor>> _sensors;
  std::unordered_map<std::string, std::shared_ptr<SensorData>> _criticalData;
  std::unordered_map<std::string, std::shared_ptr<SensorData>> _nonCriticalData;

  std::shared_ptr<IPublisher> zmq_c_publisher;
  std::shared_ptr<IPublisher> zmq_nc_publisher;
  std::shared_ptr<IPublisher> zmq_cluster_publisher;
  zmq::context_t& zmq_context;
  SensorLogger _logger;

  static constexpr int critical_update_interval_ms = 50;
  static constexpr int non_critical_update_interval_ms = 200;
  static constexpr int sensor_read_interval_ms = 100;
};

#endif
