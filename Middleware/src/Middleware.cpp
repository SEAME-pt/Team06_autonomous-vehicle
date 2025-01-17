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

void Middleware::addSensor(bool critical, ISensor* sensor) {
    sensors[sensor->getName()] = sensor;
}

void Middleware::start() {
    stop_flag = false;
    non_critical_thread = std::thread(&Middleware::updateNonCritical, this);
    read_critical_thread = std::thread(&Middleware::readCritical, this);
    critical_thread = std::thread(&Middleware::updateCritical, this);
}

void Middleware::stop() {
    if (!stop_flag.exchange(true)) {
        if (non_critical_thread.joinable()) {
            non_critical_thread.join();
        }
        if (read_critical_thread.joinable()) {
            read_critical_thread.join();
        }
        if (critical_thread.joinable()) {
            critical_thread.join();
        }
    }
}

void Middleware::publishSensorData(const SensorData& data) {
    int value = static_cast<int>(data.value);
    std::string value_str = std::to_string(value);

    zmq::message_t message(value_str.size());
    memcpy(message.data(), value_str.c_str(), value_str.size());
    std::cerr << data.name << ": " << value_str << std::endl; //debugging
    if (data.critical) {
        zmq_c_publisher.send(message, zmq::send_flags::none);
    } else {
        zmq_nc_publisher.send(message, zmq::send_flags::none);
    }
}


void Middleware::updateNonCritical() {
    while (!stop_flag) {
        {
            for (std::unordered_map<std::string, ISensor*>::iterator it = sensors.begin(); it != sensors.end(); ++it) {
                try {
                    if (!it->second->getCritical()) {
                        it->second->updateSensorData();
                        SensorData data = it->second->getSensorData();
                        if (data.updated) {
                            std::lock_guard<std::mutex> lock(it->second->getMutex());
                            publishSensorData(data);
                        }
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error updating non critical sensor " << it->first << ": " << e.what() << std::endl;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(non_critical_update_interval_ms));
    }
}

void Middleware::readCritical(){
    while (!stop_flag) {
        {
            for (std::unordered_map<std::string, ISensor*>::iterator it = sensors.begin(); it != sensors.end(); ++it) {
                try {
                    if (it->second->getCritical()) {
                        it->second->updateSensorData();
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error reading critical sensor " << it->first << ": " << e.what() << std::endl;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(read_critical_interval_ms));
    }
}

void Middleware::updateCritical() {
    while (!stop_flag) {
        {
            for (std::unordered_map<std::string, ISensor*>::iterator it = sensors.begin(); it != sensors.end(); ++it) {
                try {
                    if (it->second->getCritical()) {
                        SensorData data = it->second->getSensorData();
                        if (data.updated) {
                            std::lock_guard<std::mutex> lock(it->second->getMutex());
                            publishSensorData(data);
                        }
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error updating critical sensor " << it->first << ": " << e.what() << std::endl;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(critical_update_interval_ms));
    }
}
