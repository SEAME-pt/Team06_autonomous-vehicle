#ifndef SENSORHANDLER_HPP
#define SENSORHANDLER_HPP

#include "ISensor.hpp"
#include "Battery.hpp"
#include "Speed.hpp"
#include "ZmqPublisher.hpp"
#include "SensorLogger.hpp"
#include <atomic>
#include <thread>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <condition_variable>


class SensorHandler {
public:
    explicit SensorHandler(const std::string& zmq_c_address,
                          const std::string& zmq_nc_address,
                          zmq::context_t& zmq_context);
    ~SensorHandler();

    // Delete copy and move operations
    SensorHandler(const SensorHandler&) = delete;
    SensorHandler& operator=(const SensorHandler&) = delete;
    SensorHandler(SensorHandler&&) = delete;
    SensorHandler& operator=(SensorHandler&&) = delete;

    void start();
    void stop();

private:
    void addSensors();
    void sortSensorData();
    void readSensors();
    void publishCritical();
    void publishNonCritical();
    void publishSensorData(const std::shared_ptr<SensorData>& sensorData);

    std::atomic<bool> stop_flag;
    std::thread critical_thread;
    std::thread non_critical_thread;
    std::thread sensor_read_thread;

    mutable std::mutex sensors_mutex;
    mutable std::mutex critical_mutex;
    mutable std::mutex non_critical_mutex;
    std::condition_variable data_cv;

    std::unordered_map<std::string, std::shared_ptr<ISensor>> _sensors;
    std::unordered_map<std::string, std::shared_ptr<SensorData>> _criticalData;
    std::unordered_map<std::string, std::shared_ptr<SensorData>> _nonCriticalData;

    ZmqPublisher zmq_c_publisher;
    ZmqPublisher zmq_nc_publisher;
    SensorLogger _logger;

    static constexpr int critical_update_interval_ms = 50;
    static constexpr int non_critical_update_interval_ms = 200;
    static constexpr int sensor_read_interval_ms = 100;
};

#endif
