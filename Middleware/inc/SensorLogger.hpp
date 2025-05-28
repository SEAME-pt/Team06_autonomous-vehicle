#ifndef SENSORLOGGER_HPP
#define SENSORLOGGER_HPP

#include "ISensor.hpp"
#include <chrono>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>

class SensorLogger {
public:
  explicit SensorLogger(
      const std::string &log_file_path = "sensor_updates.log");
  ~SensorLogger();

  void logSensorUpdate(const std::shared_ptr<SensorData> &sensorData);
  void logError(const std::string &sensorName, const std::string &errorMessage);

private:
  std::ofstream log_file;
  std::mutex log_mutex;
  std::string getTimestamp() const;
};

#endif
