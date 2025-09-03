#include "ControlLogger.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>

ControlLogger::ControlLogger(const std::string &log_file_path) {
  std::cout << "Initializing control logger with file: " << log_file_path
            << std::endl; // LCOV_EXCL_LINE - Debug logging

  // Try to create/open the file first in read mode to check if it exists
  std::ifstream check_file(log_file_path.c_str());
  if (!check_file) {
    // File doesn't exist, create it
    std::ofstream create_file(log_file_path.c_str());
    create_file.close();
    std::cout << "Created new control log file"
              << std::endl; // LCOV_EXCL_LINE - Debug logging
  } else {
    check_file.close();
  }

  // Open with both append and out mode to ensure proper file creation and
  // appending
  log_file.open(log_file_path.c_str(), std::ios::app | std::ios::out);
  if (!log_file.is_open()) {
    std::cerr << "Failed to open control log file: " << log_file_path
              << std::endl; // LCOV_EXCL_LINE - Error handling
    throw std::runtime_error("Failed to open control log file: " +
                             log_file_path);
  }

  std::cout << "Control log file opened successfully"
            << std::endl; // LCOV_EXCL_LINE - Debug logging

  // Write session start marker and immediately flush
  std::lock_guard<std::mutex> lock(log_mutex);
  log_file << "\n=== Control Logging Session Started at " << getTimestamp()
           << " ===\n"
           << std::flush;

  // Verify file is still good after writing
  if (!log_file.good()) {
    std::cerr << "Failed to write to control log file after opening"
              << std::endl; // LCOV_EXCL_LINE - Error handling
    throw std::runtime_error(
        "Failed to write to control log file after opening");
  }

  std::cout << "Control logger initialization complete"
            << std::endl; // LCOV_EXCL_LINE - Debug logging
}

ControlLogger::~ControlLogger() {
  if (log_file.is_open()) {
    try {
      std::lock_guard<std::mutex> lock(log_mutex);
      log_file << "\n=== Control Logging Session Ended at " << getTimestamp()
               << " ===\n"
               << std::flush;
      log_file.close();
      std::cout << "Control logger shutdown complete"
                << std::endl; // LCOV_EXCL_LINE - Debug logging
    } catch (const std::exception &e) {
      std::cerr << "Error during control logger shutdown: " << e.what()
                << std::endl; // LCOV_EXCL_LINE - Error handling
    }
  }
}

void ControlLogger::logControlUpdate(const std::string &command,
                                     double steering, double throttle) {
  try {
    std::lock_guard<std::mutex> lock(log_mutex);
    if (!log_file.is_open() || !log_file.good()) {
      std::cerr << "Control log file is not open or not in good state"
                << std::endl; // LCOV_EXCL_LINE - Error handling
      return;
    }

    log_file << getTimestamp() << " - "
             << "Command: " << command << ", Steering: " << steering
             << ", Throttle: " << throttle << std::endl
             << std::flush;

    // Debug output
    std::cout << "Logged control update - Steering: " << steering
              << ", Throttle: " << throttle
              << std::endl; // LCOV_EXCL_LINE - Debug logging
  } catch (const std::exception &e) {
    std::cerr << "Error logging control update: " << e.what()
              << std::endl; // LCOV_EXCL_LINE - Error handling
  }
}

void ControlLogger::logError(const std::string &errorMessage) {
  try {
    std::lock_guard<std::mutex> lock(log_mutex);
    if (!log_file.is_open() || !log_file.good()) {
      std::cerr << "Control log file is not open or not in good state"
                << std::endl; // LCOV_EXCL_LINE - Error handling
      return;
    }

    log_file << getTimestamp() << " - ERROR - " << errorMessage << std::endl
             << std::flush;

    std::cout << "Logged control error: " << errorMessage
              << std::endl; // LCOV_EXCL_LINE - Debug logging
  } catch (const std::exception &e) {
    std::cerr << "Error logging error message: " << e.what()
              << std::endl; // LCOV_EXCL_LINE - Error handling
  }
}

std::string ControlLogger::getTimestamp() const {
  try {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << '.'
       << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
  } catch (const std::exception &e) {
    std::cerr << "Error generating timestamp: " << e.what()
              << std::endl; // LCOV_EXCL_LINE - Error handling
    return "ERROR_TIMESTAMP";
  }
}
