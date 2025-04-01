#include "Battery.hpp"

Battery::Battery() {
    _name = "battery";
    _sensorData["battery"] = std::make_shared<SensorData>();
    _sensorData["battery"]->critical = false;
    _sensorData["battery"]->value = 0;
    _sensorData["battery"]->timestamp = std::chrono::high_resolution_clock::now();
    _sensorData["battery"]->name = "battery";
    _sensorData["charging"] = std::make_shared<SensorData>();
    _sensorData["charging"]->critical = false;
    _sensorData["charging"]->timestamp = std::chrono::high_resolution_clock::now();
    _sensorData["charging"]->name = "charging";
    _sensorData["power"] = std::make_shared<SensorData>();
    _sensorData["power"]->critical = true;
    _sensorData["power"]->value = 0;
    _sensorData["power"]->timestamp = std::chrono::high_resolution_clock::now();
    _sensorData["power"]->name = "power";
}

Battery::~Battery() {
}

const std::string& Battery::getName() const {
    return _name;
}

void Battery::updateSensorData() {
    readSensor();
    checkUpdated();
}

void Battery::checkUpdated() {
    for (std::unordered_map<std::string, std::shared_ptr<SensorData>>::const_iterator it = _sensorData.begin(); it != _sensorData.end(); ++it) {
        if (it->second->oldValue != it->second->value) {
            it->second->updated = true;
        } else {
            it->second->updated = false;
        }
    }
}

void Battery::readSensor() {
    unsigned int oldBattery = _sensorData["battery"]->value;
    unsigned int battery = batteryReader.getPercentage();
    unsigned int charging = batteryReader.isCharging();

    _sensorData["battery"]->oldValue = oldBattery;
    _sensorData["battery"]->value = battery;
    _sensorData["charging"]->value = charging;
    _sensorData["power"]->value = 20;
    // battery charging cheat. only goes up while charging, only goes down while not charging
    if (!oldBattery) {
        return ;
    } else if (charging) {
        _sensorData["battery"]->value = (battery > oldBattery ? battery : oldBattery);
    } else {
        _sensorData["battery"]->value = (battery < oldBattery ? battery : oldBattery);
    }
    _sensorData["battery"]->timestamp = std::chrono::high_resolution_clock::now();
    _sensorData["charging"]->timestamp = std::chrono::high_resolution_clock::now();
    _sensorData["power"]->timestamp = std::chrono::high_resolution_clock::now();
}

std::unordered_map<std::string, std::shared_ptr<SensorData>> Battery::getSensorData() const {
    return _sensorData;
}
