    #include "SensorHandler.hpp"
    #include <iostream>

    SensorHandler::SensorHandler(const std::string& zmq_c_address, const std::string& zmq_nc_address, zmq::context_t& zmq_context)
        : stop_flag(false),
        zmq_c_publisher(zmq_c_address, zmq_context),
        zmq_nc_publisher(zmq_nc_address, zmq_context) {
        addSensors();
        zmq_c_publisher.send("init;");
        zmq_nc_publisher.send("init;");
    }

    SensorHandler::~SensorHandler() {
        stop();
    }

    void SensorHandler::addSensors() {
        _sensors["battery"] = std::make_shared<Battery>();
        _sensors["speed"] = std::make_shared<Speed>();
        sortSensorData();
    }

    void SensorHandler::sortSensorData() {
        for (std::unordered_map<std::string, std::shared_ptr<ISensor>>::iterator it = _sensors.begin(); it != _sensors.end(); ++it) {
            for (std::unordered_map<std::string, std::shared_ptr<SensorData>>::iterator it2 = it->second->getSensorData().begin(); it2 != it->second->getSensorData().end(); ++it2) {
                if (it2->second->critical) {
                    _criticalData[it2->first] = it2->second;
                } else {
                    _nonCriticalData[it2->first] = it2->second;
                }
            }
        }
    }

    void SensorHandler::start() {
        stop_flag = false;
        non_critical_thread = std::thread(&SensorHandler::publishNonCritical, this);
        critical_thread = std::thread(&SensorHandler::publishCritical, this);
        readSensors();
    }


    void SensorHandler::stop() {
        if (!stop_flag.exchange(true)) {
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
                for (std::unordered_map<std::string, std::shared_ptr<ISensor>>::iterator it = _sensors.begin(); it != _sensors.end(); ++it) {
                    try {
                        it->second->updateSensorData();
                    } catch (const std::exception& e) {
                        std::cerr << "Error updating sensor [" << it->second->getName() << "]: " << e.what() << std::endl;
                    } catch (...) {
                        std::cerr << "Unknown error occurred while updating sensor [" << it->second->getName() << "]!" << std::endl;
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    void SensorHandler::publishNonCritical() {
        while (!stop_flag) {
            {
                for (std::unordered_map<std::string, std::shared_ptr<SensorData>>::iterator it = _nonCriticalData.begin(); it != _nonCriticalData.end(); ++it) {
                    if (it->second->updated) {
                        publishSensorData(it->second);
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(non_critical_update_interval_ms));
        }
    }

    void SensorHandler::publishCritical() {
        while (!stop_flag) {
            {
                for (std::unordered_map<std::string, std::shared_ptr<SensorData>>::iterator it = _criticalData.begin(); it != _criticalData.end(); ++it) {
                    if (it->second->updated) {
                        publishSensorData(it->second);
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(critical_update_interval_ms));
        }
    }

    void SensorHandler::publishSensorData(std::shared_ptr<SensorData> sensorData) {
        // std::string dataStr;
        // dataStr = sensorData->name + ":" + std::to_string(sensorData->value) + ";";
        // if (sensorData->critical) {
        //     zmq_c_publisher.send(dataStr);
        // } else {
        //     zmq_nc_publisher.send(dataStr);
        // }
    }
