#include "SensorHandler.hpp"
#include <iostream>

SensorHandler::SensorHandler(const std::string &zmq_c_address,
                             const std::string &zmq_nc_address,
                             zmq::context_t &zmq_context,
                             std::shared_ptr<IPublisher> c_publisher,
                             std::shared_ptr<IPublisher> nc_publisher,
                             bool use_real_sensors)
    : stop_flag(false),
      zmq_c_publisher(c_publisher ? c_publisher
                                  : std::make_shared<ZmqPublisher>(
                                        zmq_c_address, zmq_context)),
      zmq_nc_publisher(nc_publisher ? nc_publisher
                                    : std::make_shared<ZmqPublisher>(
                                          zmq_nc_address, zmq_context)),
      _logger("sensor_updates.log") {

  if (use_real_sensors) {
    addSensors();
  }

  zmq_c_publisher->send("init;");
  zmq_nc_publisher->send("init;");

  // Log initial sensor setup
  std::lock_guard<std::mutex> lock(sensors_mutex);
  for (const auto &[name, sensor] : _sensors) {
    std::cout << "Sensor: " << sensor->getName() << std::endl;
    for (const auto &[data_name, data] : sensor->getSensorData()) {
      std::cout << "SensorData: " << data_name << std::endl;
      _logger.logSensorUpdate(data);
    }
  }
}

SensorHandler::~SensorHandler() { stop(); }

void SensorHandler::addSensors() {
  std::lock_guard<std::mutex> lock(sensors_mutex);
  _sensors["battery"] = std::make_shared<Battery>();
  _sensors["speed"] = std::make_shared<Speed>(nullptr);
  sortSensorData();
}

void SensorHandler::addSensor(const std::string &name,
                              std::shared_ptr<ISensor> sensor) {
  std::lock_guard<std::mutex> lock(sensors_mutex);
  if (sensor == nullptr) {
    // Remove the sensor if it exists
    auto it = _sensors.find(name);
    if (it != _sensors.end()) {
      _sensors.erase(it);
      std::cout << "Removed sensor: " << name << std::endl;
    }
  } else {
    // Add or replace the sensor
    _sensors[name] = sensor;
    std::cout << "Added/updated sensor: " << name << std::endl;
  }
  sortSensorData();
}

std::unordered_map<std::string, std::shared_ptr<ISensor>>
SensorHandler::getSensors() const {
  std::lock_guard<std::mutex> lock(sensors_mutex);
  return _sensors;
}

void SensorHandler::sortSensorData() {
  // Note: This method is called from addSensors() which already holds the
  // sensors_mutex
  std::lock_guard<std::mutex> critical_lock(critical_mutex);
  std::lock_guard<std::mutex> non_critical_lock(non_critical_mutex);

  _criticalData.clear();
  _nonCriticalData.clear();

  for (const auto &[name, sensor] : _sensors) {
    if (!sensor) {
      std::cerr << "Warning: Null sensor in sensors map: " << name << std::endl;
      continue;
    }

    auto sensorDataMap = sensor->getSensorData();
    for (const auto &[data_name, data] : sensorDataMap) {
      if (!data) {
        std::cerr << "Warning: Null SensorData in sensor: " << name
                  << ", data: " << data_name << std::endl;
        continue;
      }

      if (data->critical) {
        _criticalData[data_name] = data;
      } else {
        _nonCriticalData[data_name] = data;
      }
    }
  }
}

void SensorHandler::start() {
  // Set stop_flag to false regardless of previous value and check if we need to
  // start threads
  bool was_stopped = stop_flag.exchange(false);
  std::cout << "SensorHandler::start() called, was_stopped="
            << (was_stopped ? "true" : "false") << std::endl;

  // Always re-send initialization messages
  std::cout << "SensorHandler::start() - Sending init messages" << std::endl;
  zmq_c_publisher->send("init;");
  zmq_nc_publisher->send("init;");

  // Only create threads if they're not already running
  if (was_stopped || !sensor_read_thread.joinable() ||
      !non_critical_thread.joinable() || !critical_thread.joinable()) {

    std::cout << "SensorHandler::start() - Creating threads" << std::endl;

    // Join any existing threads first (just to be safe)
    if (sensor_read_thread.joinable())
      sensor_read_thread.join();
    if (non_critical_thread.joinable())
      non_critical_thread.join();
    if (critical_thread.joinable())
      critical_thread.join();

    // Create threads
    sensor_read_thread = std::thread(&SensorHandler::readSensors, this);
    non_critical_thread = std::thread(&SensorHandler::publishNonCritical, this);
    critical_thread = std::thread(&SensorHandler::publishCritical, this);
  }
}

void SensorHandler::stop() {
  std::cout << "SensorHandler::stop() called, current stop_flag="
            << (stop_flag ? "true" : "false") << std::endl;

  // Set stop flag regardless of previous value
  stop_flag = true;
  data_cv.notify_all();

  // Join threads if they're running
  if (sensor_read_thread.joinable()) {
    std::cout << "Joining sensor_read_thread" << std::endl;
    sensor_read_thread.join();
  }
  if (non_critical_thread.joinable()) {
    std::cout << "Joining non_critical_thread" << std::endl;
    non_critical_thread.join();
  }
  if (critical_thread.joinable()) {
    std::cout << "Joining critical_thread" << std::endl;
    critical_thread.join();
  }

  std::cout << "SensorHandler::stop() completed" << std::endl;
}

void SensorHandler::readSensors() {
  while (!stop_flag) {
    {
      std::lock_guard<std::mutex> lock(sensors_mutex);
      for (auto &[name, sensor] : _sensors) {
        try {
          sensor->updateSensorData();
        } catch (const std::exception &e) {
          std::cerr << "Error updating sensor [" << sensor->getName()
                    << "]: " << e.what() << std::endl;
          _logger.logError(sensor->getName(), e.what());
        } catch (...) {
          std::cerr << "Unknown error occurred while updating sensor ["
                    << sensor->getName() << "]!" << std::endl;
          _logger.logError(sensor->getName(),
                           "Unknown error occurred during update");
        }
      }
    }
    data_cv.notify_all();
    std::this_thread::sleep_for(
        std::chrono::milliseconds(sensor_read_interval_ms));
  }
}

void SensorHandler::publishNonCritical() {
  while (!stop_flag) {
    std::unique_lock<std::mutex> lock(non_critical_mutex);
    data_cv.wait_for(
        lock, std::chrono::milliseconds(non_critical_update_interval_ms));

    if (!stop_flag) {
      for (const auto &[name, data] : _nonCriticalData) {
        if (data && data->updated) {
          publishSensorData(data);
        }
      }
    }
  }
}

void SensorHandler::publishCritical() {
  while (!stop_flag) {
    std::unique_lock<std::mutex> lock(critical_mutex);
    data_cv.wait_for(lock,
                     std::chrono::milliseconds(critical_update_interval_ms));

    if (!stop_flag) {
      for (const auto &[name, data] : _criticalData) {
        if (data && data->updated) {
          publishSensorData(data);
        }
      }
    }
  }
}

void SensorHandler::publishSensorData(
    const std::shared_ptr<SensorData> &sensorData) {
  if (!sensorData) {
    std::cerr << "Warning: Attempted to publish null SensorData" << std::endl;
    return;
  }

  std::string dataStr =
      sensorData->name + ":" + std::to_string(sensorData->value) + ";";

  try {
    if (sensorData->critical) {
      std::cout << "Publishing critical data: " << dataStr << std::endl;
      zmq_c_publisher->send(dataStr);
    } else {
      std::cout << "Publishing non-critical data: " << dataStr << std::endl;
      zmq_nc_publisher->send(dataStr);
    }
    _logger.logSensorUpdate(sensorData);
  } catch (const std::exception &e) {
    std::cerr << "Error publishing sensor data: " << e.what() << std::endl;
    _logger.logError(sensorData->name,
                     std::string("Error publishing data: ") + e.what());
  }
}
