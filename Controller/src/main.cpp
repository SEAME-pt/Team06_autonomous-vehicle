#include "Controller.hpp"
#include "ControlTransmitter.hpp"
#include "ZmqPublisher.hpp"
#include <iostream>
#include <zmq.hpp>
#include <string>

int main() {
    std::string zmq_address = "tcp://127.0.0.1:5557";
    zmq::context_t zmq_context(1);

    Controller controller;

    controller.openDevice();
    if (!controller.isConnected()) {
        std::cerr << "Failed to initialize controller!" << std::endl;
        return EXIT_FAILURE;
    }

    ControlTransmitter controlTransmitter(zmq_address, zmq_context);
    controlTransmitter.startTransmitting();

    return EXIT_SUCCESS;
}
