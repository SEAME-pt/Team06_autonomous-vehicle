#include "TrafficSignHandler.hpp"
#include "ZmqPublisher.hpp"
#include "ZmqSubscriber.hpp"
#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

// Define the map of publishable traffic signs
const std::unordered_map<std::string, std::string>
    TrafficSignHandler::publishable_signs = {{"SPEED_50", "50"},
                                             {"SPEED_80", "80"},
                                             {"STOP", "stop"},
                                             {"CROSSWALK", "crosswalk"},
                                             {"YIELD", "yield"}};

// TrafficSignHandler implementation
TrafficSignHandler::TrafficSignHandler(
    const std::string &traffic_sign_subscriber_address,
    zmq::context_t &zmq_context, std::shared_ptr<IPublisher> nc_publisher_ptr,
    bool test_mode)
    : stop_flag(false), _test_mode(test_mode) {

  // Initialize subscriber for Traffic Sign Detection System
  traffic_sign_subscriber = std::make_unique<ZmqSubscriber>(
      traffic_sign_subscriber_address, zmq_context, test_mode);

  // Initialize publisher - must be provided when not in test mode
  if (nc_publisher_ptr) {
    nc_publisher = nc_publisher_ptr;
  } else if (test_mode) {
    // In test mode, we can create a dummy publisher or use nullptr
    nc_publisher = nullptr;
  } else {
    throw std::runtime_error(
        "TrafficSignHandler: Publisher must be provided in production mode");
  }
}

TrafficSignHandler::~TrafficSignHandler() { stop(); }

void TrafficSignHandler::start() {
  std::cout << "Starting Traffic Sign Handler..."
            << std::endl; // LCOV_EXCL_LINE - Debug logging

  stop_flag = false;
  processing_thread =
      std::thread(&TrafficSignHandler::receiveAndProcessTrafficSignData, this);

  std::cout << "Traffic Sign Handler started successfully."
            << std::endl; // LCOV_EXCL_LINE - Debug logging
}

void TrafficSignHandler::stop() {
  if (stop_flag.load()) {
    return; // Already stopped
  }

  std::cout << "Stopping Traffic Sign Handler..."
            << std::endl; // LCOV_EXCL_LINE - Debug logging

  stop_flag = true;

  if (processing_thread.joinable()) {
    processing_thread.join();
  }

  std::cout << "Traffic Sign Handler stopped successfully."
            << std::endl; // LCOV_EXCL_LINE - Debug logging
}

void TrafficSignHandler::setTestTrafficSignData(const std::string &data) {
  if (!_test_mode) {
    return;
  }

  std::cout << "TEST MODE: Processing test data: " << data
            << std::endl; // LCOV_EXCL_LINE - Test mode logging
  processTrafficSignMessage(data);
}

void TrafficSignHandler::receiveAndProcessTrafficSignData() {
  std::cout << "Traffic Sign Handler processing thread started."
            << std::endl; // LCOV_EXCL_LINE - Debug logging

  while (!stop_flag.load()) {
    try {
      // Try to receive data from Traffic Sign Detection System
      std::string received_data =
          traffic_sign_subscriber->receive(processing_interval_ms);

      if (!received_data.empty()) {
        std::cout << "Received traffic sign data: " << received_data
                  << std::endl; // LCOV_EXCL_LINE - Debug logging
        processTrafficSignMessage(received_data);
      }

      // Small sleep to prevent busy waiting
      std::this_thread::sleep_for(std::chrono::milliseconds(10));

    } catch (const std::exception &e) {
      std::cerr << "Error in traffic sign processing thread: " << e.what()
                << std::endl; // LCOV_EXCL_LINE - Thread error handling
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  std::cout << "Traffic Sign Handler processing thread finished."
            << std::endl; // LCOV_EXCL_LINE - Debug logging
}

void TrafficSignHandler::processTrafficSignMessage(const std::string &data) {
  try {
    std::string sign_name = data;

    // Handle structured format: "traffic_sign:SIGN_TYPE" or
    // "traffic_sign:SIGN_TYPE;"
    if (data.find("traffic_sign:") == 0) {
      size_t colon_pos = data.find(':');
      if (colon_pos != std::string::npos && colon_pos + 1 < data.length()) {
        sign_name = data.substr(colon_pos + 1);
      }
    }

    // Remove trailing semicolon if present
    if (!sign_name.empty() && sign_name.back() == ';') {
      sign_name.pop_back();
    }

    // Check if this sign is in our publishable signs map
    auto it = publishable_signs.find(sign_name);
    if (it != publishable_signs.end()) {
      // Found a publishable sign, publish it
      std::string data_to_publish = "sign:" + it->second;

      if (nc_publisher) {
        std::cout << "Publishing to cluster: " << data_to_publish
                  << " (from sign: " << sign_name << ")"
                  << std::endl; // LCOV_EXCL_LINE - Debug logging
        nc_publisher->send(data_to_publish);
      } else {
        if (_test_mode) {
          std::cout << "TEST MODE: Would publish to cluster: "
                    << data_to_publish << " (from sign: " << sign_name << ")"
                    << std::endl; // LCOV_EXCL_LINE - Test mode logging
        } else {
          std::cerr << "Error: No non-critical publisher available"
                    << std::endl; // LCOV_EXCL_LINE - Error handling
        }
      }
    } else {
      std::cout << "Received sign '" << sign_name
                << "' - not in publishable signs, ignoring"
                << std::endl; // LCOV_EXCL_LINE - Debug logging
    }

  } catch (const std::exception &e) {
    std::cerr << "Error processing traffic sign message: " << e.what()
              << std::endl; // LCOV_EXCL_LINE - Error handling
  }
}
