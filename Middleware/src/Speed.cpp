#include "Speed.hpp"

Speed::Speed() {
    _name = "speed";
    _sensorData["speed"] = std::make_shared<SensorData>();
    _sensorData["speed"]->critical = true;
    _sensorData["speed"]->value = 0;
    _sensorData["speed"]->timestamp = std::chrono::high_resolution_clock::now();
    _sensorData["odo"] = std::make_shared<SensorData>();
    _sensorData["odo"]->critical = false;
    _sensorData["odo"]->value = 0;
    _sensorData["odo"]->timestamp = std::chrono::high_resolution_clock::now();
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
    for (std::unordered_map<std::string, std::shared_ptr<SensorData>>::const_iterator it = _sensorData.begin(); it != _sensorData.end(); ++it) {
        if (it->second->oldValue != it->second->value) {
            it->second->updated = true;
        } else {
            it->second->updated = false;
        }
    }
}

void Speed::readSensor() {
    if (can.Receive(buffer, length)) {
        if (can.getId() == canId) {
            _sensorData["speed"]->oldValue = _sensorData["speed"]->value;
            _sensorData["speed"]->value = (buffer[0] | (buffer[1] << 8));
            _sensorData["speed"]->timestamp = std::chrono::high_resolution_clock::now();
            calculateOdo();
            // sensorData.data["rpm"] = (buffer[2] | (buffer[3] << 8));
        } else {
            std::cerr << "Invalid CAN ID: " << std::hex << can.getId() << std::endl;
        }
    }
}

void Speed::calculateOdo() {
    _sensorData["odo"]->oldValue = _sensorData["odo"]->value;
    std::chrono::time_point<std::chrono::high_resolution_clock> oldTimestamp = _sensorData["odo"]->timestamp;
    _sensorData["odo"]->timestamp = std::chrono::high_resolution_clock::now();
    _sensorData["odo"]->value += _sensorData["speed"]->value * (5.0 / 18.0) * (_sensorData["odo"]->timestamp - oldTimestamp).count();
}
