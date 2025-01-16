#ifndef ISENSOR_HPP
#define ISENSOR_HPP

#include <string>
#include <ctime>
#include <iostream>
#include <mutex>

struct SensorData {
    std::string name;
    float value;
    std::time_t timestamp;
    bool critical;
    bool updated;
};

class ISensor {
public:
    virtual ~ISensor() = default;
    virtual SensorData getSensorData() = 0;
    virtual std::string getName() const = 0;
    virtual bool getCritical() const = 0;
    virtual std::mutex& getMutex() = 0;
    virtual void updateSensorData() = 0;
};

#endif
