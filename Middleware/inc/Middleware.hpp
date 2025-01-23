#ifndef MIDDLEWARE_HPP
#define MIDDLEWARE_HPP

#include "ISensor.hpp"
#include <unordered_map>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <zmq.hpp>

class Middleware {
public:
    Middleware(const std::string& zmq_c_address, const std::string& zmq_nc_address);
    ~Middleware();

    void addSensor(ISensor* sensor);
    void start();
    void stop();

private:
    std::unordered_map<std::string, ISensor*> critical_sensors;
    std::unordered_map<std::string, ISensor*> non_critical_sensors;
    std::thread non_critical_thread;
    std::thread critical_thread;
    std::atomic<bool> stop_flag;

    zmq::context_t zmq_context;
    zmq::socket_t zmq_c_publisher;
    zmq::socket_t zmq_nc_publisher;
    std::string zmq_c_address;
    std::string zmq_nc_address;

    void updateNonCritical();
    void updateCritical();
    void publishSensorData(const SensorData& data);

    const unsigned int critical_update_interval_ms = 100;
    const unsigned int non_critical_update_interval_ms = 1000;
};

#endif
