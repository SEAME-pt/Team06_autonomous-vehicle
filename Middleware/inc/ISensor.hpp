#ifndef ISENSOR_HPP
#define ISENSOR_HPP

#include <string>
#include <iostream>
#include <chrono>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>

struct SensorData {
    std::string name;
    std::atomic<unsigned int> value;
    std::atomic<unsigned int> oldValue;
    std::chrono::time_point<std::chrono::steady_clock> timestamp;
    bool critical;
    std::atomic<bool> updated;

    SensorData(const std::string& n, bool c)
        : name(n), value(0), oldValue(0),
          timestamp(std::chrono::steady_clock::now()),
          critical(c), updated(false) {}
};

class ISensor {
public:
    virtual ~ISensor() = default;

    // Thread-safe methods
    virtual std::unordered_map<std::string, std::shared_ptr<SensorData>> getSensorData() const = 0;
    virtual const std::string& getName() const = 0;
    virtual void updateSensorData() = 0;

protected:
    mutable std::mutex sensor_mutex_;  // For derived classes to use

private:
    virtual void readSensor() = 0;
    virtual void checkUpdated() = 0;
};

#endif
