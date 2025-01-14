#ifndef MIDDLEWARE_HPP
#define MIDDLEWARE_HPP

#include <unordered_map>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
// #include <zmq.hpp>
// #include <nlohmann/json.hpp>
#include "ISensor.hpp"

class Middleware {
public:
    Middleware(int update_interval_ms/*, const std::string& zmq_address*/);
    ~Middleware();

    void addSensor(const std::string& name, ISensor* sensor);
    void start();
    void stop();

private:
    std::unordered_map<std::string, ISensor*> sensors;
    std::thread updater_thread;
    std::mutex sensor_mutex;
    std::condition_variable cv;
    std::atomic<bool> stop_flag;
    int update_interval_ms;

    // zmq::context_t zmq_context;
    // zmq::socket_t zmq_publisher;
    // std::string zmq_address;

    void updateLoop();
    // void publishSensorData(const SensorData& data);
};

#endif
