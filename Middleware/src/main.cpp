#include "Middleware.hpp"
#include "Battery.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    Middleware middleware;

    // CanSensor canSensor("Speed");
    Battery batterySensor("Battery");

    // middleware.addSensor(&canSensor);
    middleware.addSensor(&batterySensor);

    middleware.start();

    std::cout << "Middleware started... running indefinitely while the Jetson Racer is on.\n";

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    middleware.stop();

    return 0;
}
