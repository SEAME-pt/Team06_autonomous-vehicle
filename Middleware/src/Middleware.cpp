#include "Middleware.hpp"
#include <iostream>
#include <chrono>

Middleware::Middleware() : running(false), speed(0.0), batteryLevel(0.0) {}

Middleware::~Middleware() {
    stop();
}

void Middleware::addSensor(ISensor* sensor) {
    sensors[sensor->getName()] = sensor;
}

void Middleware::start() {
    running = true;
    workerThread = std::thread(&Middleware::run, this);
}

void Middleware::stop() {
    running = false;
    if (workerThread.joinable()) {
        workerThread.join();
    }
}

double Middleware::getSpeed() const {
    return speed.load();
}

double Middleware::getBatteryLevel() const {
    return batteryLevel.load();
}

void Middleware::run() {
    while (running) {
        if (sensors.find("Speed") != sensors.end()) {
            float speedValue = sensors["Speed"]->getValue();
            speed.store(speedValue);
        }
        if (sensors.find("Battery") != sensors.end()) {
            float batteryValue = sensors["Battery"]->getValue();
            batteryLevel.store(batteryValue);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
