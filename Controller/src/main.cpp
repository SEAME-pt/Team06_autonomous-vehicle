#include "Controller.hpp"
#include "ControlTransmitter.hpp"
#include "ZmqPublisher.hpp"
#include <iostream>
#include <zmq.hpp>
#include <string>

int main() {
    std::string zmq_address = "tcp://127.0.0.1:5557";
    zmq::context_t zmq_context(1);

    // Create transmitter with controller reference
    ControlTransmitter controlTransmitter(zmq_address, zmq_context);

    // Initialize the controller inside the transmitter
    if (!controlTransmitter.initController()) {
        std::cerr << "Failed to initialize controller!" << std::endl;
        return EXIT_FAILURE;
    }

    // This is a blocking call that should keep the program running
    controlTransmitter.startTransmitting();

    return EXIT_SUCCESS;
}
