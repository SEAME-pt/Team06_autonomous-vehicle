#ifndef MIDDLEWARE_HPP
#define MIDDLEWARE_HPP

#include "ISensor.hpp"
#include <unordered_map>
#include <thread>
#include <atomic>
#include <string>

class Middleware {
public:
    Middleware();
    ~Middleware();

    void addSensor(ISensor* sensor);

    void start();
    void stop();

    float getSpeed() const;
    float getBatteryLevel() const;

private:
    std::unordered_map<std::string, ISensor*> sensors;

    std::atomic<bool> running;
    std::thread workerThread;

    std::atomic<float> speed;
    std::atomic<float> batteryLevel;

    void run();
};

#endif
