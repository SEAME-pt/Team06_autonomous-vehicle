#include "Middleware.hpp"
#include "Battery.hpp"
#include <thread>
#include <chrono>

int main() {
    int update_interval_ms = 100; // 0.1 second
    std::string zmq_address = "tcp://*:5555";

    // Create Middleware
    Middleware middleware(update_interval_ms, zmq_address);

    // Create and add sensors
    Battery battery("Main Battery");
    middleware.addSensor("battery", &battery);

    // Start Middleware
    middleware.start();

    // Allow it to run for 10 seconds
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // Stop Middleware
    middleware.stop();

    return 0;
}
