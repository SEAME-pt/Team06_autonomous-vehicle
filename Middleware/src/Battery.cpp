#include "Battery.hpp"

Battery::Battery(const std::string& name) {
    sensorData.value = 0;
    sensorData.timestamp = std::time(nullptr);
    sensorData.name = name;
    sensorData.critical = false;
    sensorData.updated = true;
}

Battery::~Battery() {
}

void Battery::updateSensorData() {
    unsigned int tmp = sensorData.value;
    sensorData.value = calcPercentage();
    sensorData.timestamp = std::time(nullptr);
    if (tmp != sensorData.value) {
        sensorData.updated = true;
    } else {
        sensorData.updated = false;
    }
}

unsigned int Battery::calcPercentage() {
    percentage = batteryReader.getPercentage();
    charging = batteryReader.isCharging();
    if (!sensorData.value) {
        return percentage;
    } else if ( percentage > 100 ) {
        percentage = 100;
    } else if ( percentage < 1 ) {
        percentage = 1;
    } else {
        if ( charging ) {
            percentage = (percentage > sensorData.value ? percentage : sensorData.value);
        } else {
            percentage = (percentage < sensorData.value ? percentage : sensorData.value);
        }
    }
	return percentage;
}

std::string Battery::getName() const {
    return sensorData.name;
}

std::mutex& Battery::getMutex() {
    return mtx;
}

bool Battery::getCritical() const {
    return sensorData.critical;
}

bool Battery::getCharging() {
    return charging;
}

SensorData Battery::getSensorData() {
    return sensorData;
}
