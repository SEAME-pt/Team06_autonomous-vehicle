#include "TrafficSignHandler.hpp"
#include "IPublisher.hpp"
#include "ZmqSubscriber.hpp"
#include <chrono>
#include <iostream>
#include <thread>
#include <iomanip>
#include <sstream>
#include <memory>
#include <stdexcept>

// TrafficSignData implementation
std::string TrafficSignData::toString() const {
  return "traffic_sign:" + getSignTypeString() + ";";
}

std::string TrafficSignData::getSignTypeString() const {
  switch (sign_type) {
    case TrafficSignType::NONE:
      return "NONE";
    case TrafficSignType::STOP:
      return "STOP";
    case TrafficSignType::SPEED_50:
      return "SPEED_50";
    case TrafficSignType::SPEED_80:
      return "SPEED_80";
    case TrafficSignType::CROSSWALK:
      return "CROSSWALK";
    default:
      return "NONE";
  }
}

TrafficSignType TrafficSignData::stringToSignType(const std::string& sign_str) {
  if (sign_str == "STOP") {
    return TrafficSignType::STOP;
  } else if (sign_str == "SPEED_50") {
    return TrafficSignType::SPEED_50;
  } else if (sign_str == "SPEED_80") {
    return TrafficSignType::SPEED_80;
  } else if (sign_str == "CROSSWALK") {
    return TrafficSignType::CROSSWALK;
  } else {
    return TrafficSignType::NONE;
  }
}

TrafficSignData TrafficSignData::fromString(const std::string& data) {
  TrafficSignData result;
  result.sign_type = TrafficSignType::NONE; // Default to no sign

  // Parse format: "traffic_sign:SIGN_TYPE" or "traffic_sign:SIGN_TYPE;"
  if (data.find("traffic_sign:") == 0) {
    // Find the sign type after "traffic_sign:"
    size_t colon_pos = data.find(':');
    if (colon_pos != std::string::npos && colon_pos + 1 < data.length()) {
      // Extract the sign type after the colon
      std::string sign_str = data.substr(colon_pos + 1);

      // Remove trailing semicolon if present
      if (!sign_str.empty() && sign_str.back() == ';') {
        sign_str.pop_back();
      }

      result.sign_type = stringToSignType(sign_str);
    }
  }
  // Direct format support (AI model sends directly)
  else if (data == "STOP" || data == "STOP;") {
    result.sign_type = TrafficSignType::STOP;
  } else if (data == "SPEED_50" || data == "SPEED_50;") {
    result.sign_type = TrafficSignType::SPEED_50;
  } else if (data == "SPEED_80" || data == "SPEED_80;") {
    result.sign_type = TrafficSignType::SPEED_80;
  } else if (data == "CROSSWALK" || data == "CROSSWALK;") {
    result.sign_type = TrafficSignType::CROSSWALK;
  } else if (data == "NONE" || data == "NONE;") {
    result.sign_type = TrafficSignType::NONE;
  }

  return result;
}

// TrafficSignHandler implementation
TrafficSignHandler::TrafficSignHandler(const std::string &traffic_sign_subscriber_address,
                                     zmq::context_t &zmq_context,
                                     std::shared_ptr<IPublisher> nc_publisher_ptr,
                                     bool test_mode)
    : stop_flag(false), has_new_data(false), _test_mode(test_mode) {

  // Initialize subscriber for Traffic Sign Detection System
  traffic_sign_subscriber = std::make_unique<ZmqSubscriber>(traffic_sign_subscriber_address, zmq_context, test_mode);

  // Initialize publisher - must be provided when not in test mode
  if (nc_publisher_ptr) {
    nc_publisher = nc_publisher_ptr;
  } else if (test_mode) {
    // In test mode, we can create a dummy publisher or use nullptr
    nc_publisher = nullptr;
  } else {
    throw std::runtime_error("TrafficSignHandler: Publisher must be provided in production mode");
  }

  // Initialize with default "no sign" state
  latest_data.sign_type = TrafficSignType::NONE;
}

TrafficSignHandler::~TrafficSignHandler() {
  stop();
}

void TrafficSignHandler::start() {
  std::cout << "Starting Traffic Sign Handler..." << std::endl;

  stop_flag = false;
  processing_thread = std::thread(&TrafficSignHandler::receiveAndProcessTrafficSignData, this);

  std::cout << "Traffic Sign Handler started successfully." << std::endl;
}

void TrafficSignHandler::stop() {
  if (stop_flag.load()) {
    return; // Already stopped
  }

  std::cout << "Stopping Traffic Sign Handler..." << std::endl;

  stop_flag = true;

  // Wake up processing thread
  data_cv.notify_all();

  if (processing_thread.joinable()) {
    processing_thread.join();
  }

  std::cout << "Traffic Sign Handler stopped successfully." << std::endl;
}

void TrafficSignHandler::setTestTrafficSignData(const TrafficSignData& data) {
  if (!_test_mode) {
    return;
  }

  std::lock_guard<std::mutex> lock(data_mutex);
  latest_data = data;
  has_new_data = true;
  data_cv.notify_one();
}

void TrafficSignHandler::receiveAndProcessTrafficSignData() {
  std::cout << "Traffic Sign Handler processing thread started." << std::endl;

  while (!stop_flag.load()) {
    try {
      // Try to receive data from Traffic Sign Detection System
      std::string received_data = traffic_sign_subscriber->receive(processing_interval_ms);

      if (!received_data.empty()) {
        // Parse the received data
        TrafficSignData sign_data = TrafficSignData::fromString(received_data);

        {
          std::lock_guard<std::mutex> lock(data_mutex);
          latest_data = sign_data;
          has_new_data = true;
        }

        // Process and publish the data (pass both original string and parsed data)
        processTrafficSignData(received_data, sign_data);
      }

      // Check for test data in test mode
      if (_test_mode) {
        std::unique_lock<std::mutex> lock(data_mutex);
        if (has_new_data) {
          TrafficSignData data_to_process = latest_data;
          has_new_data = false;
          lock.unlock();

          // For test mode, generate the string format from the test data
          std::string test_data_string = data_to_process.toString();
          processTrafficSignData(test_data_string, data_to_process);
        }
      }

      // Small sleep to prevent busy waiting
      std::this_thread::sleep_for(std::chrono::milliseconds(10));

    } catch (const std::exception& e) {
      std::cerr << "Error in traffic sign processing thread: " << e.what() << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  std::cout << "Traffic Sign Handler processing thread finished." << std::endl;
}

void TrafficSignHandler::processTrafficSignData(const std::string& original_data, const TrafficSignData& parsed_data) {
  // Log the received data
  std::cout << "Processing traffic sign data: received='" << original_data
            << "' parsed_type=" << parsed_data.getSignTypeString() << std::endl;

  // Forward both the original data and parsed data to the publisher
  publishTrafficSignData(original_data, parsed_data);
}

void TrafficSignHandler::publishTrafficSignData(const std::string& original_data, const TrafficSignData& parsed_data) {
  try {
    // Create standardized format for publishing
    std::string data_to_publish = parsed_data.toString();

    // Publish to non-critical channel (like SensorHandler pattern)
    if (nc_publisher) {
      std::cout << "Publishing traffic sign data: " << data_to_publish
                << " (internal type=" << parsed_data.getSignTypeString() << ")" << std::endl;
      nc_publisher->send(data_to_publish);
    } else {
      if (_test_mode) {
        std::cout << "TEST MODE: Would publish traffic sign data: " << data_to_publish
                  << " (internal type=" << parsed_data.getSignTypeString() << ")" << std::endl;
      } else {
        std::cerr << "Error: No non-critical publisher available" << std::endl;
      }
    }

  } catch (const std::exception& e) {
    std::cerr << "Error publishing traffic sign data: " << e.what() << std::endl;
  }
}
