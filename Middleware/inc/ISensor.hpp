#ifndef ISENSOR_HPP
#define ISENSOR_HPP

#include <string>
#include <ctime>
#include <iostream>
#include <unordered_map>
#include <mutex>

struct SensorData {
    std::string name;
    std::unordered_map<std::string, unsigned int> data;
    std::time_t timestamp;
    bool critical;
    bool updated;
};

class ISensor {
public:
    virtual ~ISensor() = default;
    virtual const SensorData& getSensorData() const = 0;
    virtual const std::string& getName() const = 0;
    virtual bool getCritical() const = 0;
    virtual bool getUpdated() const = 0;
    virtual std::mutex& getMutex() = 0;
    virtual void updateSensorData() = 0;
};

#endif
