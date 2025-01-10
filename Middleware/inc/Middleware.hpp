#ifndef MIDDLEWARE_HPP
#define MIDDLEWARE_HPP

#include <unordered_map>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include "ISensor.hpp"

class Middleware {
public:
    Middleware(int update_interval_ms, const std::string& zmq_address);
    ~Middleware();

    void addSensor(const std::string& name, ISensor* sensor);
    void start();
    void stop();

private:
    std::unordered_map<std::string, ISensor*> sensors; // Stores sensors
    std::thread updater_thread;                        // Thread for updating sensors
    std::mutex sensor_mutex;                           // Protects the sensor map
    std::condition_variable cv;                        // Synchronizes the update loop
    std::atomic<bool> stop_flag;                       // Controls the stop condition
    int update_interval_ms;                            // Update interval in milliseconds

    zmq::context_t zmq_context;                        // ZeroMQ context
    zmq::socket_t zmq_publisher;                       // ZeroMQ publisher socket
    std::string zmq_address;                           // Address to publish data

    void updateLoop();                                 // Periodically updates sensors
    void publishSensorData(const SensorData& data);    // Publishes SensorData over ZeroMQ
};

#endif // MIDDLEWARE_HPP
