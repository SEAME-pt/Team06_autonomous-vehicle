#include "Middleware.hpp"
#include "Battery.hpp"
#include "CAN.hpp"
#include <thread>
#include <chrono>
#include <csignal>

std::atomic<bool> stop_flag(false);

void signalHandler(int signal) {
    stop_flag = true;
}

int main() {
    std::signal(SIGINT, signalHandler);
    std::string zmq_c_address = "tcp://127.0.0.1:5555";
    std::string zmq_nc_address = "tcp://127.0.0.1:5556";
    Middleware middleware(zmq_c_address, zmq_nc_address);
    Battery battery("battery");
    CAN can("speed");
    middleware.addSensor(false, &battery);
    middleware.addSensor(true, &can);
    middleware.start();
    while (stop_flag == false) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    middleware.stop();
    return 0;
}
