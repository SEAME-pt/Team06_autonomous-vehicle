#include "Speed.hpp"
#include <iostream>
#include <chrono>

Speed::Speed() {
    _name = "speed";
    _sensorData["speed"] = std::make_shared<SensorData>("speed", true);
    _sensorData["speed"]->value.store(0);

    _sensorData["odo"] = std::make_shared<SensorData>("odo", false);
    _sensorData["odo"]->value.store(0);

    latest_timestamp = std::chrono::steady_clock::now();
    last_measurement_time = latest_timestamp;
}

Speed::~Speed() {
    stop();
}

const std::string& Speed::getName() const {
    return _name;
}

std::unordered_map<std::string, std::shared_ptr<SensorData>>
Speed::getSensorData() const {
    std::lock_guard<std::mutex> lock(data_mutex);
    return _sensorData;
}

void Speed::start() {
    if (!subscribed.load()) {
        auto& bus = CanMessageBus::getInstance();

        std::vector<uint16_t> canIds = {canId, canId2, canId3};
        bus.subscribeToMultipleIds(shared_from_this(), canIds);
        subscribed.store(true);
    }
}

void Speed::stop() {
    if (subscribed.load()) {
        auto& bus = CanMessageBus::getInstance();
        bus.unsubscribe(canId);
        subscribed.store(false);
        std::cout << "Speed unsubscribed from CAN ID: 0x" << std::hex << canId << std::dec << std::endl;
    }
}

void Speed::updateSensorData() {
    readSensor();
    checkUpdated();
}

void Speed::onCanMessage(const CanMessage& message) {
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

    std::cout << "Speed received CAN message with " << (int)message.length << " bytes" << std::endl;
}

void Speed::readSensor() {
    if (!new_data_available.load()) {
        return; // No new data
    }

    std::lock_guard<std::mutex> lock(data_mutex);

    if (latest_length >= 6) {
        // Extract pulse data from CAN message (Arduino format)
        // buffer[0-1]: Pulse count in this interval (16-bit, little endian)
        uint16_t pulse_delta = latest_data[0] | (latest_data[1] << 8);

        // buffer[2-5]: Total pulse count since startup (32-bit, little endian)
        uint32_t new_total_pulses = latest_data[2] | (latest_data[3] << 8) |
                                   (latest_data[4] << 16) | (latest_data[5] << 24);

        // Validate pulse data
        if (new_total_pulses < total_pulses) {
            std::cout << "Warning: Total pulse count decreased (possible Arduino reset)" << std::endl;
        }

        last_pulse_delta = pulse_delta;
        total_pulses = new_total_pulses;

        // Calculate speed and odometer
        calculateSpeed();
        calculateOdo();

        std::cout << "Speed updated with " << pulse_delta << " pulses (total: " << total_pulses << ")" << std::endl;
    } else {
        std::cerr << "Speed: Invalid CAN message length: " << (int)latest_length << std::endl;
    }

    new_data_available.store(false);
}

void Speed::calculateSpeed() {
    // Calculate time difference since last measurement
    auto current_time = latest_timestamp;
    auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(
        current_time - last_measurement_time);
    double time_diff_seconds = duration.count();

    if (time_diff_seconds > 0 && last_pulse_delta > 0) {
        // Calculate distance traveled in mm directly from pulses
        // Each pulse = wheelCircumference_mm / pulsesPerRevolution
        // wheelCircumference_mm = π * diameter = π * 67mm ≈ 210.5mm
        static constexpr double wheelCircumference_mm = wheelDiameter_mm * 3.14159;
        static constexpr double mm_per_pulse = wheelCircumference_mm / pulsesPerRevolution;

        double distance_mm = static_cast<double>(last_pulse_delta) * mm_per_pulse;

        // Calculate speed directly in mm/s
        double speed_mms = distance_mm / time_diff_seconds;

        // Store as rounded mm/s (integer resolution)
        uint32_t speed_value = static_cast<uint32_t>(speed_mms + 0.5);

        // Update speed sensor data
        auto old_speed = _sensorData["speed"]->value.load();
        _sensorData["speed"]->oldValue.store(old_speed);
        _sensorData["speed"]->value.store(speed_value);
        _sensorData["speed"]->timestamp = current_time;
        _sensorData["speed"]->updated.store(true);

        std::cout << "Speed calculated: " << speed_value << " mm/s"
                  << " (from " << last_pulse_delta << " pulses in " << time_diff_seconds << "s)" << std::endl;
    } else {
        // No movement or invalid time difference
        auto old_speed = _sensorData["speed"]->value.load();
        _sensorData["speed"]->oldValue.store(old_speed);
        _sensorData["speed"]->value.store(0);
        _sensorData["speed"]->timestamp = current_time;
        _sensorData["speed"]->updated.store(true);

        if (time_diff_seconds <= 0) {
            std::cout << "Speed: Invalid time difference: " << time_diff_seconds << "s" << std::endl;
        } else {
            std::cout << "Speed: No movement detected (0 pulses)" << std::endl;
        }
    }

    last_measurement_time = current_time;
}

void Speed::calculateOdo() {
    // Calculate incremental distance from pulse delta (same as speed calculation)ma
    // Each pulse = wheelCircumference_mm / pulsesPerRevolution
    // wheelCircumference_mm = π * diameter = π * 67mm ≈ 210.5mm
    static constexpr double wheelCircumference_mm = wheelDiameter_mm * 3.14159;
    static constexpr double mm_per_pulse = wheelCircumference_mm / pulsesPerRevolution;

    if (last_pulse_delta > 0) {
        // Calculate incremental distance from this CAN message's pulse delta
        double distance_mm = static_cast<double>(last_pulse_delta) * mm_per_pulse;
        double distance_m = distance_mm / 1000.0; // mm to meters

        // Add to existing odometer value (incremental update)
        auto old_odo = _sensorData["odo"]->value.load();
        uint32_t new_odo_value = old_odo + static_cast<uint32_t>(distance_m + 0.5); // Round to nearest meter

        // Update odometer sensor data
        _sensorData["odo"]->oldValue.store(old_odo);
        _sensorData["odo"]->value.store(new_odo_value);
        _sensorData["odo"]->timestamp = latest_timestamp;

        // Mark as updated if distance was added
        if (distance_m > 0.5) { // Only update if we moved at least 0.5 meters
            _sensorData["odo"]->updated.store(true);
            std::cout << "Odometer updated: " << new_odo_value << " meters"
                      << " (added " << distance_m << "m from " << last_pulse_delta << " pulses)" << std::endl;
        }
    }
}

void Speed::checkUpdated() {
    std::lock_guard<std::mutex> lock(data_mutex);
    for (const auto& [name, sensor] : _sensorData) {
        if (sensor->oldValue.load() != sensor->value.load()) {
            sensor->updated.store(true);
        }
        // Note: We don't set updated to false here because readSensor()
        // handles the updated flag based on new CAN data availability
    }
}
