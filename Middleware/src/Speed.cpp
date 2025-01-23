#include "Speed.hpp"

Speed::Speed(const std::string& name) {
    sensorData.value = 0;
    sensorData.timestamp = std::time(nullptr);
    sensorData.name = name;
    sensorData.critical = true;
    sensorData.updated = true;
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
    if (can.Receive(buffer, length)) {
        if (can.getId() == canId) {
            speed = (buffer[0] | (buffer[1] << 8));
            rpm = (buffer[2] | (buffer[3] << 8));
            unsigned int tmp = sensorData.value;
            sensorData.value = static_cast<unsigned int>(speed);
            sensorData.timestamp = std::time(nullptr);
            if (tmp != sensorData.value) {
                sensorData.updated = true;
            }
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

const unsigned int& Speed::getValue() const {
    return sensorData.value;
}
