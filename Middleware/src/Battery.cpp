#include "Battery.hpp"

Battery::Battery(const std::string& name) {
    sensorData.name = name;
    sensorData.timestamp = std::time(nullptr);
    sensorData.critical = false;
    sensorData.updated = true;
    sensorData.data["battery"] = 0;
}

Battery::~Battery() {
}

void Battery::updateSensorData() {
    sensorData.updated = false;
    old = sensorData.data["battery"];
    readBattery();
    if (old != battery) {
        sensorData.data["battery"] = battery;
        sensorData.data["charging"] = (charging ? 1 : 0);
        sensorData.timestamp = std::time(nullptr);
        sensorData.updated = true;
    }
}

void Battery::readBattery() {
    battery = batteryReader.getPercentage();
    charging = batteryReader.isCharging();
    if (!old) {
        return ;
    } else if (charging) {
        battery = (battery > old ? battery : old);
    } else {
        battery = (battery < old ? battery : old);
    }
}

const std::string& Battery::getName() const {
    return sensorData.name;
}

std::mutex& Battery::getMutex() {
    return mtx;
}

bool Battery::getCritical() const {
    return sensorData.critical;
}

const SensorData& Battery::getSensorData() const {
    return sensorData;
}

bool Battery::getUpdated() const {
    return sensorData.updated;
}
