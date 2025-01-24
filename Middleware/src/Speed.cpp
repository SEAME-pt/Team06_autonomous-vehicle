#include "Speed.hpp"

Speed::Speed(const std::string& name) {
    sensorData.name = name;
    sensorData.timestamp = std::time(nullptr);
    sensorData.critical = true;
    sensorData.data["speed"] = 0;
}

Speed::~Speed() {
}

const std::string& Speed::getName() const {
    return sensorData.name;
}

bool Speed::getCritical() const {
    return sensorData.critical;
}

const SensorData& Speed::getSensorData() const {
    return sensorData;
}

void Speed::updateSensorData() {
    sensorData.updated = false;
    old = sensorData.data["speed"];
    readSpeed();
    if (old != speed) {
        sensorData.data["speed"] = static_cast<unsigned int>(speed);
        sensorData.data["rpm"] = static_cast<unsigned int>(rpm);
        sensorData.timestamp = std::time(nullptr);
        sensorData.updated = true;
    }

}

void Speed::readSpeed() {
    if (can.Receive(buffer, length)) {
        if (can.getId() == canId) {
            speed = (buffer[0] | (buffer[1] << 8));
            rpm = (buffer[2] | (buffer[3] << 8));
        } else {
            std::cerr << "Invalid CAN ID: " << std::hex << can.getId() << std::endl;
        }
    }
}

std::mutex& Speed::getMutex() {
    return mtx;
}

bool Speed::getUpdated() const {
    return sensorData.updated;
}
