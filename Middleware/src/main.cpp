#include "SensorHandler.hpp"
#include "ControlAssembly.hpp"
#include <thread>
#include <chrono>
#include <csignal>
#include <iostream>
#include <zmq.hpp>

std::atomic<bool> stop_flag(false);

void signalHandler(int signal) {
    stop_flag = true;
}

int main() {
    try {
        std::signal(SIGINT, signalHandler);
        std::string zmq_c_address = "tcp://127.0.0.1:5555"; // critical addr
        std::string zmq_nc_address = "tcp://127.0.0.1:5556"; // non-critical addr
        std::string zmq_control_address = "tcp://127.0.0.1:5557"; // control addr
        zmq::context_t zmq_context(1);

        SensorHandler sensorHandler(zmq_c_address, zmq_nc_address, zmq_context);
        // ControlAssembly controlAssembly(zmq_control_address, zmq_context);
        sensorHandler.start();
        // controlAssembly.start();
        while (!stop_flag) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        sensorHandler.stop();
        // controlAssembly.stop();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Unknown error!" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
