#include "Speed.hpp"



Speed::Speed() {
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
    checkUpdated();
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
    if (can.Receive(buffer, length)) {
        if (can.getId() == canId) {
            auto speed_value = _sensorData["speed"]->value.load();
            _sensorData["speed"]->oldValue.store(speed_value);
            _sensorData["speed"]->value.store(buffer[0] | (buffer[1] << 8));
            _sensorData["speed"]->timestamp = std::chrono::steady_clock::now();
            calculateOdo();
            // sensorData.data["rpm"] = (buffer[2] | (buffer[3] << 8));
        } else {
            std::cerr << "Invalid CAN ID: " << std::hex << can.getId() << std::endl;
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
}
