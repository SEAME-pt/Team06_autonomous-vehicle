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
      zmq_context(zmq_context),
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

  // Initialize CAN Message Bus
  auto& canBus = CanMessageBus::getInstance();
  if (!canBus.start(false)) { // false = production mode
    std::cerr << "Failed to start CAN Message Bus!" << std::endl;
    throw std::runtime_error("CAN Message Bus initialization failed");
  }

  // Create sensors
  _sensors["battery"] = std::make_shared<Battery>();

  auto speed_sensor = std::make_shared<Speed>();
  auto distance_sensor = std::make_shared<Distance>();

  _sensors["speed"] = speed_sensor;
  _sensors["distance"] = distance_sensor;

  // Set up speed data accessor for distance sensor collision detection
  distance_sensor->setSpeedDataAccessor([speed_sensor]() -> std::shared_ptr<SensorData> {
    auto speed_data_map = speed_sensor->getSensorData();
    auto it = speed_data_map.find("speed");
    return (it != speed_data_map.end()) ? it->second : nullptr;
  });

    // Set up emergency brake publisher for distance sensor
  // Create a ZMQ publisher for emergency brake commands to ControlAssembly
  // Use a dedicated port to avoid conflicts with manual control messages
  auto emergency_brake_publisher = std::make_shared<ZmqPublisher>("tcp://127.0.0.1:5561", zmq_context);
  distance_sensor->setEmergencyBrakePublisher(emergency_brake_publisher);

  // Start CAN sensors (this subscribes them to the bus)
  speed_sensor->start();
  distance_sensor->start();

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

  // Create temporary maps to avoid potential use-after-free
  std::unordered_map<std::string, std::shared_ptr<SensorData>> newCriticalData;
  std::unordered_map<std::string, std::shared_ptr<SensorData>>
      newNonCriticalData;

  for (const auto &[name, sensor] : _sensors) {
    if (!sensor) {
      std::cerr << "Warning: Null sensor in sensors map: " << name << std::endl;
      continue;
    }

    // Get a copy of the sensor data map to avoid iterator invalidation
    auto sensorDataMap = sensor->getSensorData();
    for (const auto &[data_name, data] : sensorDataMap) {
      if (!data) {
        std::cerr << "Warning: Null SensorData in sensor: " << name
                  << ", data: " << data_name << std::endl;
        continue;
      }

      // Make a local copy to ensure we have a strong reference
      auto dataCopy = data;
      if (!dataCopy) {
        continue; // Skip if somehow the copy is null
      }

      // Use emplace to avoid unnecessary copies and potential analyzer
      // confusion
      if (dataCopy->critical) {
        newCriticalData.emplace(data_name, dataCopy);
      } else {
        newNonCriticalData.emplace(data_name, dataCopy);
      }
    }
  }

  // Atomic replacement of the maps
  _criticalData = std::move(newCriticalData);
  _nonCriticalData = std::move(newNonCriticalData);
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

  // Stop CAN sensors first
  {
    std::lock_guard<std::mutex> lock(sensors_mutex);
    for (auto& [name, sensor] : _sensors) {
      // Try to cast to CAN-enabled sensors and stop them
      if (auto speed_sensor = std::dynamic_pointer_cast<Speed>(sensor)) {
        speed_sensor->stop();
      } else if (auto distance_sensor = std::dynamic_pointer_cast<Distance>(sensor)) {
        distance_sensor->stop();
      }
    }
  }

  // Stop CAN Message Bus
  auto& canBus = CanMessageBus::getInstance();
  if (canBus.isRunning()) {
    canBus.stop();
  }

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
