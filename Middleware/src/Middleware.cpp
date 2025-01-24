#include "Middleware.hpp"
#include <iostream>

Middleware::Middleware(const std::string& zmq_c_address, const std::string& zmq_nc_address)
    : stop_flag(false),
      zmq_context(1),
      zmq_c_publisher(zmq_context, zmq::socket_type::pub),
      zmq_nc_publisher(zmq_context, zmq::socket_type::pub),
      zmq_c_address(zmq_c_address),
      zmq_nc_address(zmq_nc_address) {
    zmq_nc_publisher.bind(zmq_nc_address);
    zmq_c_publisher.bind(zmq_c_address);
}

Middleware::~Middleware() {
    stop();
}

void Middleware::addSensor(ISensor* sensor) {
    if (sensor->getCritical()) {
        critical_sensors[sensor->getName()] = sensor;
    } else {
        non_critical_sensors[sensor->getName()] = sensor;
    }
    publishSensorData(sensor->getSensorData());
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

void Middleware::start() {
    stop_flag = false;
    non_critical_thread = std::thread(&Middleware::updateNonCritical, this);
    critical_thread = std::thread(&Middleware::updateCritical, this);
}

void Middleware::stop() {
    if (!stop_flag.exchange(true)) {
        if (non_critical_thread.joinable()) {
            non_critical_thread.join();
        }
        if (critical_thread.joinable()) {
            critical_thread.join();
        }
    }
}

void Middleware::publishSensorData(const SensorData& sensorData) {  // only sending main value because Qt isnt parsing yet
    std::string dataStr = std::to_string(sensorData.data.at(sensorData.name));

    zmq::message_t message(dataStr.size());
    memcpy(message.data(), dataStr.c_str(), dataStr.size());
    std::cerr << sensorData.name << ": " << dataStr << std::endl; //debugging
    if (sensorData.critical) {
        zmq_c_publisher.send(message, zmq::send_flags::none);
    } else {
        zmq_nc_publisher.send(message, zmq::send_flags::none);
    }
}

// void Middleware::publishSensorData(const SensorData& sensorData) {
//     std::string dataStr;
//     for (std::unordered_map<std::string, unsigned int>::iterator it = sensorData.data.begin(); it != sensorData.data.end(); ++it) {
//         dataStr += it->first + ":" + std::to_string(it->second) + ";";
//     }
//     zmq::message_t message(dataStr.size());
//     memcpy(message.data(), dataStr.c_str(), dataStr.size());
//     std::cerr << dataStr << std::endl; //debugging
//     if (sensorData.critical) {
//         zmq_c_publisher.send(message, zmq::send_flags::none);
//     } else {
//         zmq_nc_publisher.send(message, zmq::send_flags::none);
//     }
// }


void Middleware::updateNonCritical() {
    while (!stop_flag) {
        {
            for (std::unordered_map<std::string, ISensor*>::iterator it = non_critical_sensors.begin(); it != non_critical_sensors.end(); ++it) {
                try {
                    it->second->updateSensorData();
                    if (it->second->getUpdated()) {
                        publishSensorData(it->second->getSensorData());
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error updating non-critical sensor [" << it->second->getName() << "]: " << e.what() << std::endl;
                } catch (...) {
                    std::cerr << "Unknown error occurred while updating non-critical sensor [" << it->second->getName() << "]!" << std::endl;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(non_critical_update_interval_ms));
    }
}

void Middleware::updateCritical() { // will update and checkUpdate&publish on different threads
    while (!stop_flag) {
        {
            for (std::unordered_map<std::string, ISensor*>::iterator it = critical_sensors.begin(); it != critical_sensors.end(); ++it) {
                try {
                    it->second->updateSensorData();;
                    if (it->second->getUpdated()) {
                        publishSensorData(it->second->getSensorData());
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error updating critical sensor [" << it->second->getName() << "]: " << e.what() << std::endl;
                } catch (...) {
                    std::cerr << "Unknown error occurred while updating critical sensor [" << it->second->getName() << "]!" << std::endl;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(critical_update_interval_ms));
    }
}
