#include "Middleware.hpp"
#include <iostream>

Middleware::Middleware(int update_interval_ms, const std::string& zmq_address)
    : update_interval_ms(update_interval_ms),
      stop_flag(false),
      zmq_context(1),
      zmq_publisher(zmq_context, zmq::socket_type::pub),
      zmq_address(zmq_address) {
    zmq_publisher.bind(zmq_address); // E.g., "tcp://*:5555"
}

Middleware::~Middleware() {
    stop();
}

void Middleware::addSensor(const std::string& name, ISensor* sensor) {
    std::lock_guard<std::mutex> lock(sensor_mutex);
    sensors[name] = sensor;
}

void Middleware::start() {
    stop_flag = false;
    updater_thread = std::thread(&Middleware::updateLoop, this);
}

void Middleware::stop() {
    if (!stop_flag.exchange(true)) {
        cv.notify_all();
        if (updater_thread.joinable()) {
            updater_thread.join();
        }
    }
}

void Middleware::publishSensorData(const SensorData& data) {
    nlohmann::json json_data = {
        {"name", data.name},
        {"unit", data.unit},
        {"value", data.value},
        {"type", data.type},
        {"timestamp", data.timestamp}
    };

    std::string json_str = json_data.dump();
    zmq::message_t message(json_str.begin(), json_str.end());
    zmq_publisher.send(message, zmq::send_flags::none);
}

void Middleware::updateLoop() {
    while (!stop_flag) {
        {
            for (auto& [name, sensor] : sensors) {
                try {
                    std::lock_guard<std::mutex> lock(sensor_mutex);
                    sensor->updateSensorData();
                    SensorData data = sensor->getSensorData();
                    publishSensorData(data);
                } catch (const std::exception& e) {
                    std::cerr << "Error updating sensor " << name << ": " << e.what() << std::endl;
                }
            }
        }
        std::unique_lock<std::mutex> lock(sensor_mutex);
    }
}
