#ifndef DISTANCE_HPP
#define DISTANCE_HPP

#include "CanReader.hpp"
#include "ISensor.hpp"
#include "MockCanReader.hpp"
#include <memory>
#include <unordered_map>

class Distance : public ISensor {
public:
  // Constructor with dependency injection
  explicit Distance(std::shared_ptr<ICanReader> canReader = nullptr);
  ~Distance();
  const std::string &getName() const override;
  void updateSensorData() override;
  std::unordered_map<std::string, std::shared_ptr<SensorData>>
  getSensorData() const override;

private:
  void readSensor() override;
  void checkUpdated() override;

  std::string _name;
  std::unordered_map<std::string, std::shared_ptr<SensorData>> _sensorData;
  uint16_t canId = 0x101;  // CAN ID for distance data from arduino
  uint8_t buffer[8];
  uint8_t length = 0;
  std::shared_ptr<ICanReader> can;
};

#endif
