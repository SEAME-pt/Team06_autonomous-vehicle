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
        auto info = batterySensor.get_battery_info();
        std::cout << "Valor ADC: " << info["raw_adc"] << "\n";
        std::cout << "Tensão ADC: " << info["adc_voltage"] << "V\n";
        std::cout << "Tensão Total: " << info["voltage"] << "V\n";
        std::cout << "Tensão por célula: " << info["cell_voltages"] << "V\n";
        std::cout << "Bateria: " << info["percentage"] << "%\n";
        std::cout << "Status: " << battery.getStatus(info["voltage"]) << "\n";
        std::cout << "--------------------\n";
    }
    middleware.stop();

    return 0;
}
