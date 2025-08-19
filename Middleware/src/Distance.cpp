#include "Distance.hpp"
#include <iostream>
#include <chrono>

Distance::Distance(std::shared_ptr<ICanReader> canReader)
    : can(canReader ? canReader : std::make_shared<CanReader>()) {
  _name = "distance";
  _sensorData["distance"] = std::make_shared<SensorData>("distance", true);
  _sensorData["distance"]->value.store(0);

  // Initialize CanReader if we created it
  if (!canReader && can) {
    auto reader = std::dynamic_pointer_cast<CanReader>(can);
    if (reader) {
      if (!reader->initialize()) {
        throw std::runtime_error("Failed to initialize CanReader");
      }
    }
  }
}

Distance::~Distance() {}

const std::string &Distance::getName() const { return _name; }

std::unordered_map<std::string, std::shared_ptr<SensorData>>
Distance::getSensorData() const {
  return _sensorData;
}

void Distance::updateSensorData() {
  readSensor();
  checkUpdated();
}

void Distance::checkUpdated() {
  for (const auto &[name, sensor] : _sensorData) {
    if (sensor->oldValue.load() != sensor->value.load()) {
      sensor->updated.store(true);
    } else {
      sensor->updated.store(false);
    }
  }
}

void Distance::readSensor() {
  if (can->Receive(buffer, length)) {
    if (can->getId() == canId) {
      auto distance_value = _sensorData["distance"]->value.load();
      _sensorData["distance"]->oldValue.store(distance_value);
      _sensorData["distance"]->value.store(buffer[0] | (buffer[1] << 8));
      _sensorData["distance"]->timestamp = std::chrono::steady_clock::now();

      // Always mark distance as updated when we receive valid CAN data
      // This way even if the value hasn't changed, we still consider it updated
      _sensorData["distance"]->updated.store(true);
    } else {
      std::cerr << "Invalid CAN ID: " << std::hex << can->getId() << std::endl;
    }
  }
}
