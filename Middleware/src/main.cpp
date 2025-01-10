#include "Middleware.hpp"
#include "Battery.hpp"
#include <thread>
#include <chrono>

int main() {
    int update_interval_ms = 100; // 0.1 second
    // std::string zmq_address = "tcp://*:5555";
    Middleware middleware(update_interval_ms/*, zmq_address*/);
    Battery battery("battery");
    middleware.addSensor("battery", &battery);
    middleware.start();
    std::this_thread::sleep_for(std::chrono::seconds(10));
    middleware.stop();

    return 0;
}
