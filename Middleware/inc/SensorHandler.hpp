#ifndef SENSORHANDLER_HPP
#define SENSORHANDLER_HPP

#include "ISensor.hpp"
#include "Battery.hpp"
#include "Speed.hpp"
#include "ZmqPublisher.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <memory>

class SensorHandler {
public:
    SensorHandler(const std::string& zmq_c_address, const std::string& zmq_nc_address, zmq::context_t& zmq_context);
    ~SensorHandler();

    void start();
    void stop();

private:
    std::unordered_map<std::string, std::shared_ptr<ISensor>> _sensors;
    std::unordered_map<std::string, std::shared_ptr<SensorData>> _criticalData;
    std::unordered_map<std::string, std::shared_ptr<SensorData>> _nonCriticalData;
    std::thread non_critical_thread;
    std::thread critical_thread;
    std::atomic<bool> stop_flag;

    ZmqPublisher zmq_c_publisher;
    ZmqPublisher zmq_nc_publisher;

    void addSensors();
    void sortSensorData();
    void readSensors();
    void publishNonCritical();
    void publishCritical();
    void publishSensorData(std::shared_ptr<SensorData> sensorData);


    const unsigned int critical_update_interval_ms = 100;
    const unsigned int non_critical_update_interval_ms = 1000;
};

#endif
