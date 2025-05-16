#include "Speed.hpp"

Speed::Speed(std::shared_ptr<ICanReader> canReader) : can(canReader ? canReader : std::make_shared<CanReader>()) {
    _name = "speed";
    _sensorData["speed"] = std::make_shared<SensorData>("speed", true);
    _sensorData["speed"]->value.store(0);

    _sensorData["odo"] = std::make_shared<SensorData>("odo", false);
    _sensorData["odo"]->value.store(0);
}

Speed::~Speed() {
}

const std::string& Speed::getName() const {
    return _name;
}

std::unordered_map<std::string, std::shared_ptr<SensorData>> Speed::getSensorData() const {
    return _sensorData;
}

void Speed::updateSensorData() {
    readSensor();

    // We want to preserve any "updated" flags set in readSensor or calculateOdo
    // Store the current update flags before calling checkUpdated
    bool speedUpdated = _sensorData["speed"]->updated.load();
    bool odoUpdated = _sensorData["odo"]->updated.load();

    // Call checkUpdated to handle other sensor data
    checkUpdated();

    // Restore the update flags if they were set to true
    if (speedUpdated) {
        _sensorData["speed"]->updated.store(true);
    }
    if (odoUpdated) {
        _sensorData["odo"]->updated.store(true);
    }
}

void Speed::checkUpdated() {
    for (const auto& [name, sensor] : _sensorData) {
        if (sensor->oldValue.load() != sensor->value.load()) {
            sensor->updated.store(true);
        } else {
            sensor->updated.store(false);
        }
    }
}

void Speed::readSensor() {
    if (can->Receive(buffer, length)) {
        if (can->getId() == canId) {
            auto speed_value = _sensorData["speed"]->value.load();
            _sensorData["speed"]->oldValue.store(speed_value);
            _sensorData["speed"]->value.store(buffer[0] | (buffer[1] << 8));
            _sensorData["speed"]->timestamp = std::chrono::steady_clock::now();

            // Always mark speed as updated when we receive valid CAN data
            // This way even if the value hasn't changed, we still consider it updated
            _sensorData["speed"]->updated.store(true);

            calculateOdo();
            // sensorData.data["rpm"] = (buffer[2] | (buffer[3] << 8));
        } else {
            std::cerr << "Invalid CAN ID: " << std::hex << can->getId() << std::endl;
        }
    }
}

void Speed::calculateOdo() {
    auto odo_value = _sensorData["odo"]->value.load();
    _sensorData["odo"]->oldValue.store(odo_value);
    auto oldTimestamp = _sensorData["odo"]->timestamp;
    _sensorData["odo"]->timestamp = std::chrono::steady_clock::now();

    // Calculate time difference in seconds
    auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(_sensorData["odo"]->timestamp - oldTimestamp);
    double time_diff_seconds = duration.count();

    // Calculate distance: speed (km/h) * time (s) * (5/18) for m/s conversion
    auto speed = _sensorData["speed"]->value.load();
    auto new_odo = static_cast<unsigned int>(speed * (5.0 / 18.0) * time_diff_seconds);
    _sensorData["odo"]->value.store(odo_value + new_odo);

    // Always mark odo as updated when we recalculate it with new data
    if (new_odo > 0) {
        _sensorData["odo"]->updated.store(true);
    }
}
