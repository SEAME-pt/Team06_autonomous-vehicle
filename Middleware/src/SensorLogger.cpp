#include "SensorLogger.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>

SensorLogger::SensorLogger(const std::string &log_file_path) {
  std::cout << "Initializing logger with file: " << log_file_path << std::endl;

  // Try to create/open the file first in read mode to check if it exists
  std::ifstream check_file(log_file_path.c_str());
  if (!check_file) {
    // File doesn't exist, create it
    std::ofstream create_file(log_file_path.c_str());
    create_file.close();
    std::cout << "Created new log file" << std::endl;
  } else {
    check_file.close();
  }

  // Open with both append and out mode to ensure proper file creation and
  // appending
  log_file.open(log_file_path.c_str(), std::ios::app | std::ios::out);
  if (!log_file.is_open()) {
    std::cerr << "Failed to open log file: " << log_file_path << std::endl;
    throw std::runtime_error("Failed to open log file: " + log_file_path);
  }

  std::cout << "Log file opened successfully" << std::endl;

  // Write session start marker and immediately flush
  std::lock_guard<std::mutex> lock(log_mutex);
  log_file << "\n=== Sensor Logging Session Started at " << getTimestamp()
           << " ===\n"
           << std::flush;

  // Verify file is still good after writing
  if (!log_file.good()) {
    std::cerr << "Failed to write to log file after opening" << std::endl;
    throw std::runtime_error("Failed to write to log file after opening");
  }

  std::cout << "Logger initialization complete" << std::endl;
}

SensorLogger::~SensorLogger() {
  if (log_file.is_open()) {
    try {
      std::lock_guard<std::mutex> lock(log_mutex);
      log_file << "\n=== Sensor Logging Session Ended at " << getTimestamp()
               << " ===\n"
               << std::flush;
      log_file.close();
      std::cout << "Logger shutdown complete" << std::endl;
    } catch (const std::exception &e) {
      std::cerr << "Error during logger shutdown: " << e.what() << std::endl;
    }
  }
}

void SensorLogger::logSensorUpdate(
    const std::shared_ptr<SensorData> &sensorData) {
  if (!sensorData)
    return;

  try {
    std::lock_guard<std::mutex> lock(log_mutex);
    if (!log_file.is_open() || !log_file.good()) {
      std::cerr << "Log file is not open or not in good state" << std::endl;
      return;
    }

    // Only log if the value has actually changed
    if (sensorData->value != sensorData->oldValue) {
      log_file << getTimestamp() << " - " << "Sensor: " << sensorData->name
               << ", Value: " << sensorData->value
               << ", Old Value: " << sensorData->oldValue
               << ", Critical: " << (sensorData->critical ? "Yes" : "No")
               << std::endl
               << std::flush;

      // Debug output
      std::cout << "Logged update for sensor: " << sensorData->name
                << " (Value: " << sensorData->value
                << ", Old: " << sensorData->oldValue << ")" << std::endl;
    }
  } catch (const std::exception &e) {
    std::cerr << "Error logging sensor update: " << e.what() << std::endl;
  }
}

void SensorLogger::logError(const std::string &sensorName,
                            const std::string &errorMessage) {
  try {
    std::lock_guard<std::mutex> lock(log_mutex);
    if (!log_file.is_open() || !log_file.good()) {
      std::cerr << "Log file is not open or not in good state" << std::endl;
      return;
    }

    log_file << getTimestamp() << " - ERROR - " << "Sensor: " << sensorName
             << ", Error: " << errorMessage << std::endl
             << std::flush;

    std::cout << "Logged error for sensor: " << sensorName << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error logging error message: " << e.what() << std::endl;
  }
}

std::string SensorLogger::getTimestamp() const {
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
    std::cerr << "Error generating timestamp: " << e.what() << std::endl;
    return "ERROR_TIMESTAMP";
  }
}
