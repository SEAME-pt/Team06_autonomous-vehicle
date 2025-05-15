#include "SensorLogger.hpp"
#include <iomanip>
#include <sstream>

SensorLogger::SensorLogger(const std::string& log_file_path) {
    log_file.open(log_file_path, std::ios::app);
    if (!log_file.is_open()) {
        throw std::runtime_error("Failed to open log file: " + log_file_path);
    }
    log_file << "\n=== Sensor Logging Session Started at " << getTimestamp() << " ===\n";
    log_file.flush();
}

SensorLogger::~SensorLogger() {
    if (log_file.is_open()) {
        log_file << "\n=== Sensor Logging Session Ended at " << getTimestamp() << " ===\n";
        log_file.close();
    }
}

void SensorLogger::logSensorUpdate(const std::shared_ptr<SensorData>& sensorData) {
    if (!sensorData) return;

    std::lock_guard<std::mutex> lock(log_mutex);
    log_file << getTimestamp() << " - "
             << "Sensor: " << sensorData->name
             << ", Value: " << sensorData->value
             << ", Old Value: " << sensorData->oldValue
             << ", Critical: " << (sensorData->critical ? "Yes" : "No")
             << std::endl;
    log_file.flush();
}

void SensorLogger::logError(const std::string& sensorName, const std::string& errorMessage) {
    std::lock_guard<std::mutex> lock(log_mutex);
    log_file << getTimestamp() << " - ERROR - "
             << "Sensor: " << sensorName
             << ", Error: " << errorMessage
             << std::endl;
    log_file.flush();
}

std::string SensorLogger::getTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
       << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}
