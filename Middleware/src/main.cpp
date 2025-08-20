#include "ControlAssembly.hpp"
#include "LaneKeepingHandler.hpp"
#include "SensorHandler.hpp"
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <zmq.hpp>

namespace {
std::atomic<bool> stop_flag(false);
std::unique_ptr<SensorHandler> sensor_handler;
std::unique_ptr<ControlAssembly> control_assembly;
std::unique_ptr<LaneKeepingHandler> lane_keeping_handler;

void signalHandler(int signal) {
  std::cout << "\nReceived signal " << signal << ", initiating shutdown..."
            << std::endl;
  stop_flag = true;
}

void setupSignalHandlers() {
  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);
}
} // namespace

int main() {
  try {
    setupSignalHandlers();

    // Configuration
    const std::string zmq_c_address = "tcp://127.0.0.1:5555"; // critical addr
    const std::string zmq_nc_address =
        "tcp://127.0.0.1:5556"; // non-critical addr
    const std::string zmq_control_address =
        "tcp://127.0.0.1:5557"; // control addr
    const std::string zmq_lkas_address =
        "tcp://127.0.0.1:5558"; // lane keeping assistance system addr

    // Initialize ZMQ context
    zmq::context_t zmq_context(1);

    // Initialize components
    std::cout << "Initializing sensor handler..." << std::endl;
    sensor_handler = std::make_unique<SensorHandler>(
        zmq_c_address, zmq_nc_address, zmq_context, nullptr, nullptr,
        true); // Use real sensors in production

    std::cout << "Initializing control assembly..." << std::endl;
    control_assembly =
        std::make_unique<ControlAssembly>(zmq_control_address, zmq_context);

    std::cout << "Initializing lane keeping handler..." << std::endl;
    // Lane keeping handler will create its own publisher internally
    lane_keeping_handler = std::make_unique<LaneKeepingHandler>(
        zmq_lkas_address, zmq_context, nullptr, false, zmq_nc_address); // Production mode

    // Start components
    std::cout << "Starting sensor handler..." << std::endl;
    sensor_handler->start();

    std::cout << "Starting control assembly..." << std::endl;
    control_assembly->start();

    std::cout << "Starting lane keeping handler..." << std::endl;
    lane_keeping_handler->start();

    // Main loop
    std::cout << "System running. Press Ctrl+C to stop." << std::endl;
    while (!stop_flag) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Cleanup
    std::cout << "Stopping sensor handler..." << std::endl;
    sensor_handler->stop();

    std::cout << "Stopping control assembly..." << std::endl;
    control_assembly->stop();

    std::cout << "Stopping lane keeping handler..." << std::endl;
    lane_keeping_handler->stop();

    // Release component resources
    std::cout << "Releasing components..." << std::endl;
    sensor_handler.reset();
    control_assembly.reset();
    lane_keeping_handler.reset();

    // Terminate ZMQ context to ensure clean shutdown
    std::cout << "Terminating ZMQ context..." << std::endl;
    zmq_context.close();

    std::cout << "Shutdown complete." << std::endl;
    return EXIT_SUCCESS;

  } catch (const zmq::error_t &e) {
    std::cerr << "ZMQ Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "Unknown error!" << std::endl;
    return EXIT_FAILURE;
  }
}
