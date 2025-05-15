#include "Controller.hpp"
#include "ControlTransmitter.hpp"
#include "ZmqPublisher.hpp"
#include <iostream>
#include <zmq.hpp>
#include <string>
#include <thread>
#include <chrono>

int main() {
    std::cout << "Controller application starting..." << std::endl;
    std::string zmq_address = "tcp://127.0.0.1:5557";
    zmq::context_t zmq_context(1);

    // Verify ZMQ connectivity with test message
    try {
        std::cout << "Testing ZMQ connectivity to: " << zmq_address << std::endl;
        ZmqPublisher test_publisher(zmq_address, zmq_context);

        // Send a test message
        std::cout << "Sending test message..." << std::endl;
        test_publisher.send("test:message;");

        // Allow time for message to be processed
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Test message sent" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "ZMQ test failed: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    // Create transmitter with controller reference
    std::cout << "Creating control transmitter..." << std::endl;
    ControlTransmitter controlTransmitter(zmq_address, zmq_context);

    // Initialize the controller
    std::cout << "Initializing controller..." << std::endl;
    if (!controlTransmitter.initController()) {
        std::cerr << "Failed to initialize controller!" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Starting transmitter loop..." << std::endl;
    // This is a blocking call that should keep the program running
    controlTransmitter.startTransmitting();

    std::cout << "Transmitter loop exited, shutting down" << std::endl;
    return EXIT_SUCCESS;
}
