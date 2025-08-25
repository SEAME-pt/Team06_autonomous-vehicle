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

  // Initialize distance history
  for (size_t i = 0; i < HISTORY_SIZE; ++i) {
    distance_history[i].distance_cm = 0;
    distance_history[i].timestamp = latest_timestamp;
  }
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

void Distance::setSpeedDataAccessor(std::function<std::shared_ptr<SensorData>()> accessor) {
    speed_data_accessor = accessor;
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

        // Update distance history for approach detection
        distance_history[history_index].distance_cm = new_distance;
        distance_history[history_index].timestamp = latest_timestamp;

        history_index = (history_index + 1) % HISTORY_SIZE;
        if (history_index == 0) {
            history_full = true;
        }

        current_distance_cm.store(new_distance);
        std::cout << "Distance sensor reading: " << new_distance << " cm" << std::endl;
    } else {
        std::cerr << "Distance: Invalid CAN message length: " << (int)latest_length << std::endl;
    }

    new_data_available.store(false);
}

bool Distance::isObjectApproaching() const {
    if (!history_full && history_index < 3) {
        return false; // Need at least 3 readings
    }

    // Calculate average rate of change over recent history
    size_t readings_to_check = std::min(static_cast<size_t>(4), history_full ? HISTORY_SIZE : history_index);
    if (readings_to_check < 3) return false;

    double total_rate = 0.0;
    int valid_rates = 0;

    for (size_t i = 1; i < readings_to_check; ++i) {
        size_t current_idx = (history_index - 1 - i + HISTORY_SIZE) % HISTORY_SIZE;
        size_t prev_idx = (history_index - i + HISTORY_SIZE) % HISTORY_SIZE;

        auto time_diff = std::chrono::duration_cast<std::chrono::duration<double>>(
            distance_history[current_idx].timestamp - distance_history[prev_idx].timestamp).count();

        if (time_diff > 0.1) { // Only consider if time difference is meaningful
            double distance_change = static_cast<double>(distance_history[prev_idx].distance_cm) -
                                   static_cast<double>(distance_history[current_idx].distance_cm);
            double rate = distance_change / time_diff; // cm/s (positive = approaching)
            total_rate += rate;
            valid_rates++;
        }
    }

    if (valid_rates > 0) {
        double avg_rate = total_rate / valid_rates;
        return avg_rate > 2.0; // Object approaching at > 2 cm/s
    }

    return false;
}

double Distance::calculateStoppingDistance(double speed_mps) const {
    // Stopping distance = reaction_distance + braking_distance
    // reaction_distance = speed * reaction_time
    // braking_distance = speed² / (2 * deceleration)

    double reaction_distance = speed_mps * REACTION_TIME_S;
    double braking_distance = (speed_mps * speed_mps) / (2.0 * DECELERATION_MPS2);

    return reaction_distance + braking_distance; // in meters
}

double Distance::calculateTimeToCollision(double distance_cm, double speed_mps, double closing_rate_mps) const {
    double distance_m = distance_cm / 100.0; // Convert to meters
    double relative_speed = speed_mps + closing_rate_mps; // Total closing speed

    if (relative_speed <= 0) {
        return std::numeric_limits<double>::infinity(); // No collision
    }

    return distance_m / relative_speed; // Time in seconds
}

void Distance::calculateCollisionRisk() {
    // Get current distance and check if object is approaching
    uint16_t distance_cm = current_distance_cm.load();
    bool approaching = isObjectApproaching();
    object_approaching.store(approaching);

    // Get current speed
    uint32_t speed_kmh = 0;
    if (speed_data_accessor) {
        auto speed_data = speed_data_accessor();
        if (speed_data) {
            speed_kmh = speed_data->value.load();
        }
    }

    int new_risk_level = 0; // Default: safe

    // Only calculate collision risk if we have valid distance reading within sensor range
    if (distance_cm > 0 && distance_cm <= MAX_DISTANCE_CM) {
        if (speed_kmh > 0 && approaching) {
            // Convert speed to m/s
            double speed_mps = (speed_kmh * 1000.0) / 3600.0; // km/h to m/s

            // Calculate required stopping distances
            double warning_stopping_distance = calculateStoppingDistance(speed_mps) * WARNING_SAFETY_FACTOR;
            double emergency_stopping_distance = calculateStoppingDistance(speed_mps) * EMERGENCY_SAFETY_FACTOR;

            // Convert to centimeters for comparison
            double warning_distance_cm = warning_stopping_distance * 100.0;
            double emergency_distance_cm = emergency_stopping_distance * 100.0;

            if (distance_cm <= emergency_distance_cm) {
                new_risk_level = 2; // Emergency braking required
                std::cout << "EMERGENCY BRAKING REQUIRED! Distance: " << distance_cm
                         << "cm, Speed: " << speed_kmh << "km/h, Emergency threshold: "
                         << emergency_distance_cm << "cm" << std::endl;
            } else if (distance_cm <= warning_distance_cm) {
                new_risk_level = 1; // Warning - driver should brake
                std::cout << "COLLISION WARNING! Distance: " << distance_cm
                         << "cm, Speed: " << speed_kmh << "km/h, Warning threshold: "
                         << warning_distance_cm << "cm" << std::endl;
            } else {
                std::cout << "Safe distance maintained. Distance: " << distance_cm
                         << "cm, Speed: " << speed_kmh << "km/h, Approaching: "
                         << (approaching ? "YES" : "NO") << std::endl;
            }
        } else if (distance_cm < 30) {
            // Very close object regardless of speed or movement
            new_risk_level = 1; // Warning for very close objects
            std::cout << "Close proximity warning: " << distance_cm << "cm" << std::endl;
        } else {
            std::cout << "No collision risk. Distance: " << distance_cm
                     << "cm, Speed: " << speed_kmh << "km/h, Approaching: "
                     << (approaching ? "YES" : "NO") << std::endl;
        }
    }

    // Update risk level and sensor data
    int old_risk = risk_level.exchange(new_risk_level);

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
