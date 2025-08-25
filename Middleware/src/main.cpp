#include "ControlAssembly.hpp"
#include "LaneKeepingHandler.hpp"
#include "SensorHandler.hpp"
#include "TrafficSignHandler.hpp"
#include "IPublisher.hpp"
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <zmq.hpp>

namespace {
std::atomic<bool> stop_flag(false);
std::atomic<int> signal_count(0);
std::unique_ptr<SensorHandler> sensor_handler;
std::unique_ptr<ControlAssembly> control_assembly;
std::unique_ptr<LaneKeepingHandler> lane_keeping_handler;
std::unique_ptr<TrafficSignHandler> traffic_sign_handler;

void signalHandler(int signal) {
  int count = signal_count.fetch_add(1) + 1;

  if (count == 1) {
    std::cout << "\nReceived signal " << signal << ", initiating graceful shutdown..."
              << std::endl;
    std::cout << "Press Ctrl+C again to force exit." << std::endl;
    stop_flag = true;
  } else if (count >= 2) {
    std::cout << "\nReceived signal " << signal << " again, forcing exit..."
              << std::endl;
    std::cout << "Terminating immediately..." << std::endl;
    std::exit(EXIT_FAILURE);
  }
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
  const std::string zmq_c_address = "tcp://100.93.45.188:5555"; // critical addr
  const std::string zmq_nc_address =
      "tcp://100.93.45.188:5556"; // non-critical addr
    const std::string zmq_control_address =
        "tcp://127.0.0.1:5557"; // control addr
    const std::string zmq_lkas_address =
        "tcp://127.0.0.1:5558"; // lane keeping assistance system addr
    const std::string zmq_traffic_sign_address =
        "tcp://127.0.0.1:5559"; // traffic sign detection system addr

    // Initialize ZMQ context
    zmq::context_t zmq_context(1);

    // Initialize components
    // Create shared ZMQ publishers to avoid binding conflicts
    std::cout << "Creating shared ZMQ publishers..." << std::endl;
    auto c_publisher = std::make_shared<ZmqPublisher>(zmq_c_address, zmq_context);
    auto nc_publisher = std::make_shared<ZmqPublisher>(zmq_nc_address, zmq_context);

    std::cout << "Initializing sensor handler..." << std::endl;
    sensor_handler = std::make_unique<SensorHandler>(
        zmq_c_address, zmq_nc_address, zmq_context, c_publisher, nc_publisher,
        true); // Use real sensors in production

    std::cout << "Initializing control assembly..." << std::endl;
    control_assembly =
        std::make_unique<ControlAssembly>(zmq_control_address, zmq_context, nullptr, nullptr, nc_publisher);

    std::cout << "Initializing lane keeping handler..." << std::endl;
    // Share the non-critical publisher with sensor handler
    lane_keeping_handler = std::make_unique<LaneKeepingHandler>(
        zmq_lkas_address, zmq_context, nc_publisher, false); // Production mode

    std::cout << "Initializing traffic sign handler..." << std::endl;
    // Share the non-critical publisher with other handlers
    traffic_sign_handler = std::make_unique<TrafficSignHandler>(
        zmq_traffic_sign_address, zmq_context, nc_publisher, false); // Production mode

    // Start components
    std::cout << "Starting sensor handler..." << std::endl;
    sensor_handler->start();

    std::cout << "Starting control assembly..." << std::endl;
    control_assembly->start();

    std::cout << "Starting lane keeping handler..." << std::endl;
    lane_keeping_handler->start();

    std::cout << "Starting traffic sign handler..." << std::endl;
    traffic_sign_handler->start();

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

    std::cout << "Stopping traffic sign handler..." << std::endl;
    traffic_sign_handler->stop();

    // Release component resources - this will close ZMQ sockets
    std::cout << "Releasing components..." << std::endl;
    sensor_handler.reset();
    control_assembly.reset();
    lane_keeping_handler.reset();
    traffic_sign_handler.reset();

    // Release publishers to ensure their sockets are closed
    c_publisher.reset();
    nc_publisher.reset();

    // Give a brief moment for all sockets to close cleanly
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Terminate ZMQ context to ensure clean shutdown
    std::cout << "Terminating ZMQ context..." << std::endl;
    try {
      zmq_context.close();
    } catch (const zmq::error_t &e) {
      std::cerr << "Warning: ZMQ context close error: " << e.what() << std::endl;
      // Force termination if close fails
      zmq_context.shutdown();
    }

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
