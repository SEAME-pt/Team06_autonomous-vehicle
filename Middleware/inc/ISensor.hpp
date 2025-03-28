#ifndef ISENSOR_HPP
#define ISENSOR_HPP

#include <string>
#include <iostream>
#include <chrono>
#include <unordered_map>
#include <memory>

struct SensorData {
    std::string name;
    unsigned int value;
    unsigned int oldValue;
    std::chrono::time_point<std::chrono::high_resolution_clock> timestamp;
    bool critical;
    bool updated;
};

class ISensor {
public:
    virtual ~ISensor() = default;
    virtual std::unordered_map<std::string, std::shared_ptr<SensorData>> getSensorData() const = 0;
    virtual const std::string& getName() const = 0;
    virtual void updateSensorData() = 0;

private:
    virtual void readSensor() = 0;
    virtual void checkUpdated() = 0;
};

#endif
