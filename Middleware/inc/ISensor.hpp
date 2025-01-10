#ifndef ISENSOR_HPP
#define ISENSOR_HPP

#include <string>
#include <ctime>
#include <atomic>
#include <mutex>
#include <iostream>

struct SensorData {
    std::string name;
    std::string unit;
    float value;
    std::string type;
    std::time_t timestamp;
};

class ISensor {
public:
    virtual ~ISensor() = default;
    virtual SensorData getSensorData() = 0;
    virtual std::string getName() const = 0;
    virtual void updateSensorData() = 0;
};

#endif
