#include "Distance.hpp"
#include <iostream>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <limits>

Distance::Distance() {
  _name = "distance";
  // Publish obstacle alerts: 0 = safe, 1 = warning, 2 = emergency
  _sensorData["obs"] = std::make_shared<SensorData>("obs", false); // non-critical for cluster display
  _sensorData["obs"]->value.store(0);
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

void Distance::setEmergencyBrakeCallback(std::function<void(bool)> callback) {
    emergency_brake_callback = callback;
    std::cout << "Emergency brake callback set for Distance sensor" << std::endl;
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
  calculateCollisionRisk();
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
        // Extract distance value from CAN message (little endian, in cm)
        uint16_t new_distance = latest_data[0] | (latest_data[1] << 8);

        current_distance_cm.store(new_distance);
        std::cout << "Distance updated: " << new_distance << " cm" << std::endl;
    } else {
        std::cerr << "Distance: Invalid CAN message length: " << (int)latest_length << std::endl;
    }

    new_data_available.store(false);
}

void Distance::triggerEmergencyBrake(bool emergency_active) {
    if (!emergency_brake_callback) {
        std::cout << "No emergency brake callback available" << std::endl;
        return; // No callback available
    }

    bool was_active = emergency_brake_active.exchange(emergency_active);

    // Only trigger if state changed
    if (was_active != emergency_active) {
        try {
            emergency_brake_callback(emergency_active);
            std::cout << "Triggered emergency brake callback: " << (emergency_active ? "ACTIVATED" : "DEACTIVATED") << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error in emergency brake callback: " << e.what() << std::endl;
        }
    }
}

void Distance::calculateCollisionRisk() {
    // Get current distance
    uint16_t distance_cm = current_distance_cm.load();

    int new_risk_level = 0; // Default: safe

    // Simple distance-based collision risk assessment
    if (distance_cm > 0 && distance_cm <= MAX_DISTANCE_CM) {
        if (distance_cm < 20) {
            // Emergency: Very close proximity
            new_risk_level = 2;
            std::cout << "EMERGENCY: Very close proximity! Distance: " << distance_cm << "cm" << std::endl;
        } else if (distance_cm < 40) {
            // Warning: Close proximity
            new_risk_level = 1;
            std::cout << "WARNING: Close proximity! Distance: " << distance_cm << "cm" << std::endl;
        } else {
            // Safe: Adequate distance
            std::cout << "Safe distance: " << distance_cm << "cm" << std::endl;
        }
    }

    // Update risk level and sensor data
    int old_risk = risk_level.exchange(new_risk_level);

    // Handle emergency braking
    bool should_emergency_brake = (new_risk_level == 2);
    triggerEmergencyBrake(should_emergency_brake);

    std::lock_guard<std::mutex> lock(data_mutex);
    auto old_obs = _sensorData["obs"]->value.load();
    _sensorData["obs"]->oldValue.store(old_obs);
    _sensorData["obs"]->value.store(new_risk_level);
    _sensorData["obs"]->timestamp = latest_timestamp;

    // Mark as updated if risk level changed
    if (old_risk != new_risk_level) {
        _sensorData["obs"]->updated.store(true);
        const char* risk_names[] = {"SAFE", "WARNING", "EMERGENCY"};
        std::cout << "Risk level updated: " << risk_names[new_risk_level]
                 << " (level " << new_risk_level << ")" << std::endl;
    }
}

void Distance::checkUpdated() {
    std::lock_guard<std::mutex> lock(data_mutex);
    for (const auto& [name, sensor] : _sensorData) {
        if (sensor->oldValue.load() != sensor->value.load()) {
            sensor->updated.store(true);
        }
  }
}
