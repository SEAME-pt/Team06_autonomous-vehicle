#include "Distance.hpp"
#include <iostream>
#include <chrono>

Distance::Distance() {
  _name = "distance";
  _sensorData["distance"] = std::make_shared<SensorData>("distance", true);
  _sensorData["distance"]->value.store(0);
    latest_timestamp = std::chrono::steady_clock::now();
}

Distance::~Distance() {
    stop();
}

const std::string& Distance::getName() const {
    return _name;
}

std::unordered_map<std::string, std::shared_ptr<SensorData>>
Distance::getSensorData() const {
    std::lock_guard<std::mutex> lock(data_mutex);
  return _sensorData;
}

void Distance::start() {
    if (!subscribed.load()) {
        auto& bus = CanMessageBus::getInstance();
        std::vector<uint16_t> canIds = {canId, canId2, canId3};
        bus.subscribeToMultipleIds(shared_from_this(), canIds);
        subscribed.store(true);
    }
}

void Distance::stop() {
    if (subscribed.load()) {
        auto& bus = CanMessageBus::getInstance();
        bus.unsubscribe(canId);
        subscribed.store(false);
        std::cout << "Distance unsubscribed from CAN ID: 0x" << std::hex << canId << std::dec << std::endl;
    }
}

void Distance::updateSensorData() {
  readSensor();
  checkUpdated();
}

void Distance::onCanMessage(const CanMessage& message) {
    // Accept messages from multiple CAN IDs (due to crystal frequency differences)
    if (message.id != canId && message.id != canId2 && message.id != canId3) {
        return; // Should not happen, but safety check
    }

    std::lock_guard<std::mutex> lock(data_mutex);

    // Store the latest message data
    std::memcpy(latest_data, message.data, std::min(message.length, (uint8_t)8));
    latest_length = message.length;
    latest_timestamp = message.timestamp;
    new_data_available.store(true);

    std::cout << "Distance received CAN message with " << (int)message.length << " bytes" << std::endl;
}

void Distance::readSensor() {
    if (!new_data_available.load()) {
        return; // No new data
    }

    std::lock_guard<std::mutex> lock(data_mutex);

    if (latest_length >= 2) {
      auto distance_value = _sensorData["distance"]->value.load();
      _sensorData["distance"]->oldValue.store(distance_value);

        // Extract distance value from CAN message (little endian, in cm)
        uint16_t new_distance = latest_data[0] | (latest_data[1] << 8);
        _sensorData["distance"]->value.store(new_distance);
        _sensorData["distance"]->timestamp = latest_timestamp;

      // Always mark distance as updated when we receive valid CAN data
      _sensorData["distance"]->updated.store(true);

        std::cout << "Distance updated: " << new_distance << " cm" << std::endl;
    } else {
        std::cerr << "Distance: Invalid CAN message length: " << (int)latest_length << std::endl;
    }

    new_data_available.store(false);
}

void Distance::checkUpdated() {
    std::lock_guard<std::mutex> lock(data_mutex);
    for (const auto& [name, sensor] : _sensorData) {
        if (sensor->oldValue.load() != sensor->value.load()) {
            sensor->updated.store(true);
        }
        // Note: We don't set updated to false here because readSensor()
        // handles the updated flag based on new CAN data availability
  }
}
