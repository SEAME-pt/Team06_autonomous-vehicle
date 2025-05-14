    #include "SensorHandler.hpp"
    #include <iostream>

    SensorHandler::SensorHandler(const std::string& zmq_c_address, const std::string& zmq_nc_address, zmq::context_t& zmq_context)
        : stop_flag(false),
        zmq_c_publisher(zmq_c_address, zmq_context),
        zmq_nc_publisher(zmq_nc_address, zmq_context) {
        addSensors();
        zmq_c_publisher.send("init;");
        zmq_nc_publisher.send("init;");

        // Log initial sensor setup
        std::lock_guard<std::mutex> lock(sensors_mutex);
        for (const auto& [name, sensor] : _sensors) {
            std::cout << "Sensor: " << sensor->getName() << std::endl;
            for (const auto& [data_name, data] : sensor->getSensorData()) {
                std::cout << "SensorData: " << data_name << std::endl;
            }
        }
    }

    SensorHandler::~SensorHandler() {
        stop();
    }

    void SensorHandler::addSensors() {
        std::lock_guard<std::mutex> lock(sensors_mutex);
        _sensors["battery"] = std::make_shared<Battery>();
        _sensors["speed"] = std::make_shared<Speed>();
        sortSensorData();
    }

    void SensorHandler::sortSensorData() {
        // Note: This method is called from addSensors() which already holds the sensors_mutex
        std::lock_guard<std::mutex> critical_lock(critical_mutex);
        std::lock_guard<std::mutex> non_critical_lock(non_critical_mutex);

        _criticalData.clear();
        _nonCriticalData.clear();

        for (const auto& [name, sensor] : _sensors) {
            for (const auto& [data_name, data] : sensor->getSensorData()) {
                if (data->critical) {
                    _criticalData[data_name] = data;
                } else {
                    _nonCriticalData[data_name] = data;
                }
            }
        }
    }

    void SensorHandler::start() {
        if (!stop_flag.exchange(false)) {
            sensor_read_thread = std::thread(&SensorHandler::readSensors, this);
            non_critical_thread = std::thread(&SensorHandler::publishNonCritical, this);
            critical_thread = std::thread(&SensorHandler::publishCritical, this);
        }
    }

    void SensorHandler::stop() {
        if (!stop_flag.exchange(true)) {
            data_cv.notify_all();

            if (sensor_read_thread.joinable()) {
                sensor_read_thread.join();
            }
            if (non_critical_thread.joinable()) {
                non_critical_thread.join();
            }
            if (critical_thread.joinable()) {
                critical_thread.join();
            }
        }
    }

    void SensorHandler::readSensors() {
        while (!stop_flag) {
            {
                std::lock_guard<std::mutex> lock(sensors_mutex);
                for (auto& [name, sensor] : _sensors) {
                    try {
                        sensor->updateSensorData();
                    } catch (const std::exception& e) {
                        std::cerr << "Error updating sensor [" << sensor->getName() << "]: " << e.what() << std::endl;
                    } catch (...) {
                        std::cerr << "Unknown error occurred while updating sensor [" << sensor->getName() << "]!" << std::endl;
                    }
                }
            }
            data_cv.notify_all();
            std::this_thread::sleep_for(std::chrono::milliseconds(sensor_read_interval_ms));
        }
    }

    void SensorHandler::publishNonCritical() {
        while (!stop_flag) {
            std::unique_lock<std::mutex> lock(non_critical_mutex);
            data_cv.wait_for(lock, std::chrono::milliseconds(non_critical_update_interval_ms));

            if (!stop_flag) {
                for (const auto& [name, data] : _nonCriticalData) {
                    if (data && data->updated) {
                        publishSensorData(data);
                    }
                }
            }
        }
    }

    void SensorHandler::publishCritical() {
        while (!stop_flag) {
            std::unique_lock<std::mutex> lock(critical_mutex);
            data_cv.wait_for(lock, std::chrono::milliseconds(critical_update_interval_ms));

            if (!stop_flag) {
                for (const auto& [name, data] : _criticalData) {
                    if (data && data->updated) {
                        publishSensorData(data);
                    }
                }
            }
        }
    }

    void SensorHandler::publishSensorData(const std::shared_ptr<SensorData>& sensorData) {
        if (!sensorData) return;

        std::string dataStr = sensorData->name + ":" + std::to_string(sensorData->value) + ";";

        try {
            if (sensorData->critical) {
                zmq_c_publisher.send(dataStr);
            } else {
                zmq_nc_publisher.send(dataStr);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error publishing sensor data: " << e.what() << std::endl;
        }
    }
