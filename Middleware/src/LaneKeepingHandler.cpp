#include "LaneKeepingHandler.hpp"
#include "IPublisher.hpp"
#include "ZmqSubscriber.hpp"
#include <chrono>
#include <iostream>
#include <thread>
#include <iomanip>
#include <sstream>
#include <memory>

// LaneKeepingData implementation
std::string LaneKeepingData::toString() const {
  return "lane:" + std::to_string(lane_status) + ";";
}

LaneKeepingData LaneKeepingData::fromString(const std::string& data) {
  LaneKeepingData result;
  result.lane_status = 0; // Default to no deviation

  // Parse format: "lane:X" or "lane:X;"
  if (data.find("lane:") == 0) {
    // Find the lane status value after "lane:"
    size_t colon_pos = data.find(':');
    if (colon_pos != std::string::npos && colon_pos + 1 < data.length()) {
      // Extract the number after the colon
      std::string status_str = data.substr(colon_pos + 1);

      // Remove trailing semicolon if present
      if (!status_str.empty() && status_str.back() == ';') {
        status_str.pop_back();
      }

      try {
        result.lane_status = std::stoi(status_str);
      } catch (const std::exception& e) {
        std::cerr << "Error parsing lane status from: " << data << std::endl;
        result.lane_status = 0; // Default to no deviation on parse error
      }
    }
  }
  return result;
}

// LaneKeepingHandler implementation
LaneKeepingHandler::LaneKeepingHandler(const std::string &lkas_subscriber_address,
                                     zmq::context_t &zmq_context,
                                     std::shared_ptr<IPublisher> nc_publisher_ptr,
                                     bool test_mode,
                                     const std::string &nc_address)
    : stop_flag(false), has_new_data(false), _test_mode(test_mode) {

  // Initialize subscriber for Lane Keeping Assistance Software
  lkas_subscriber = std::make_unique<ZmqSubscriber>(lkas_subscriber_address, zmq_context, test_mode);

  // Initialize publisher - create one if not provided (like SensorHandler pattern)
  nc_publisher = nc_publisher_ptr ? nc_publisher_ptr
                                  : std::make_shared<ZmqPublisher>(nc_address, zmq_context);

  // Initialize with default "no deviation" state
  latest_data.lane_status = 0;
}

LaneKeepingHandler::~LaneKeepingHandler() {
  stop();
}

void LaneKeepingHandler::start() {
  std::cout << "Starting Lane Keeping Handler..." << std::endl;

  stop_flag = false;
  processing_thread = std::thread(&LaneKeepingHandler::receiveAndProcessLaneData, this);

  std::cout << "Lane Keeping Handler started successfully." << std::endl;
}

void LaneKeepingHandler::stop() {
  if (stop_flag.load()) {
    return; // Already stopped
  }

  std::cout << "Stopping Lane Keeping Handler..." << std::endl;

  stop_flag = true;

  // Wake up processing thread
  data_cv.notify_all();

  if (processing_thread.joinable()) {
    processing_thread.join();
  }

  std::cout << "Lane Keeping Handler stopped successfully." << std::endl;
}

void LaneKeepingHandler::setTestLaneKeepingData(const LaneKeepingData& data) {
  if (!_test_mode) {
    return;
  }

  std::lock_guard<std::mutex> lock(data_mutex);
  latest_data = data;
  has_new_data = true;
  data_cv.notify_one();
}

void LaneKeepingHandler::receiveAndProcessLaneData() {
  std::cout << "Lane Keeping Handler processing thread started." << std::endl;

  while (!stop_flag.load()) {
    try {
      // Try to receive data from Lane Keeping Assistance Software
      std::string received_data = lkas_subscriber->receive(processing_interval_ms);

      if (!received_data.empty()) {
        // Parse the received data
        LaneKeepingData lane_data = LaneKeepingData::fromString(received_data);

        {
          std::lock_guard<std::mutex> lock(data_mutex);
          latest_data = lane_data;
          has_new_data = true;
        }

        // Process and publish the data (pass both original string and parsed data)
        processLaneKeepingData(received_data, lane_data);
      }

      // Check for test data in test mode
      if (_test_mode) {
        std::unique_lock<std::mutex> lock(data_mutex);
        if (has_new_data) {
          LaneKeepingData data_to_process = latest_data;
          has_new_data = false;
          lock.unlock();

          // For test mode, generate the string format from the test data
          std::string test_data_string = data_to_process.toString();
          processLaneKeepingData(test_data_string, data_to_process);
        }
      }

      // Small sleep to prevent busy waiting
      std::this_thread::sleep_for(std::chrono::milliseconds(10));

    } catch (const std::exception& e) {
      std::cerr << "Error in lane keeping processing thread: " << e.what() << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  std::cout << "Lane Keeping Handler processing thread finished." << std::endl;
}

void LaneKeepingHandler::processLaneKeepingData(const std::string& original_data, const LaneKeepingData& parsed_data) {
  // Log the received data
  std::cout << "Processing lane keeping data: received='" << original_data << "' parsed_status=" << parsed_data.lane_status << std::endl;

  // Forward both the original data and parsed data to the publisher
  publishLaneData(original_data, parsed_data);
}

void LaneKeepingHandler::publishLaneData(const std::string& original_data, const LaneKeepingData& parsed_data) {
  try {
    // Ensure the published data has a semicolon at the end
    std::string data_to_publish = original_data;
    if (!data_to_publish.empty() && data_to_publish.back() != ';') {
      data_to_publish += ";";
    }

    // Keep parsed_data for internal tracking and future functionality

    // Publish to non-critical channel (like SensorHandler pattern)
    if (nc_publisher) {
      std::cout << "Publishing lane data: " << data_to_publish << " (internal status=" << parsed_data.lane_status << ")" << std::endl;
      nc_publisher->send(data_to_publish);
    } else {
      std::cerr << "Error: No non-critical publisher available" << std::endl;
    }

  } catch (const std::exception& e) {
    std::cerr << "Error publishing lane data: " << e.what() << std::endl;
  }
}
