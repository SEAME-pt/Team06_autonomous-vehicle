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
        // Extract pulse data from CAN message
        // buffer[0-1]: Pulse count in this interval (16-bit, little endian)
        uint16_t pulse_delta = latest_data[0] | (latest_data[1] << 8);

        // buffer[2-5]: Total pulse count since startup (32-bit, little endian)
        uint32_t new_total_pulses = latest_data[2] | (latest_data[3] << 8) |
                                   (latest_data[4] << 16) | (latest_data[5] << 24);

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
        // Calculate revolutions from pulse count
        double revolutions = static_cast<double>(last_pulse_delta) / pulsesPerRevolution;

        // Calculate distance traveled in meters
        double distance_m = revolutions * wheelCircumference_m;

        // Calculate speed in m/s, then convert to km/h * 10 for precision
        double speed_mps = distance_m / time_diff_seconds;
        double speed_kmh = speed_mps * 3.6;
        uint32_t speed_value = static_cast<uint32_t>(speed_kmh * 10); // km/h * 10

        // Update speed sensor data
        auto old_speed = _sensorData["speed"]->value.load();
        _sensorData["speed"]->oldValue.store(old_speed);
        _sensorData["speed"]->value.store(speed_value);
        _sensorData["speed"]->timestamp = current_time;
        _sensorData["speed"]->updated.store(true);

        std::cout << "Speed calculated: " << (speed_value / 10.0) << " km/h" << std::endl;
    } else {
        // No movement or invalid time
        auto old_speed = _sensorData["speed"]->value.load();
        _sensorData["speed"]->oldValue.store(old_speed);
        _sensorData["speed"]->value.store(0);
        _sensorData["speed"]->timestamp = current_time;
        _sensorData["speed"]->updated.store(true);
    }

    last_measurement_time = current_time;
}

void Speed::calculateOdo() {
    // Calculate total distance from total pulses
    double total_revolutions = static_cast<double>(total_pulses) / pulsesPerRevolution;
    double total_distance_m = total_revolutions * wheelCircumference_m;
    double total_distance_km = total_distance_m / 1000.0;
    uint32_t odo_value = static_cast<uint32_t>(total_distance_km * 1000); // km * 1000 for precision

    // Update odometer sensor data
    auto old_odo = _sensorData["odo"]->value.load();
    _sensorData["odo"]->oldValue.store(old_odo);
    _sensorData["odo"]->value.store(odo_value);
    _sensorData["odo"]->timestamp = latest_timestamp;

    // Mark as updated if value changed or this is the first calculation
    if (old_odo != odo_value || old_odo == 0) {
        _sensorData["odo"]->updated.store(true);
        std::cout << "Odometer updated: " << (odo_value / 1000.0) << " km" << std::endl;
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
