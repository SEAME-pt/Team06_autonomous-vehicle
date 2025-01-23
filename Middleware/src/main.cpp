#include "Middleware.hpp"
#include "Battery.hpp"
#include "Speed.hpp"
#include <thread>
#include <chrono>
#include <csignal>
#include <iostream>

std::atomic<bool> stop_flag(false);

void signalHandler(int signal) {
    stop_flag = true;
}

int main() {
    try {
        std::signal(SIGINT, signalHandler);
        std::string zmq_c_address = "tcp://127.0.0.1:5555"; // critical addr
        std::string zmq_nc_address = "tcp://127.0.0.1:5556"; // non-critical addr

        Middleware middleware(zmq_c_address, zmq_nc_address);
        Battery battery("battery");
        Speed speed("speed");

        middleware.addSensor(&battery);
        middleware.addSensor(&speed);
        middleware.start();

        while (!stop_flag) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        middleware.stop();
    } catch (const std::exception& e) {
        std::cerr << "Unhandled exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Unknown exception occurred!" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
