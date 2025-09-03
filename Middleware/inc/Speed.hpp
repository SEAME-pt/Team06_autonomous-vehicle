#ifndef SPEED_HPP
#define SPEED_HPP

#include "CanMessageBus.hpp"
#include "ISensor.hpp"
#include <memory>
#include <unordered_map>

class Speed : public ISensor,
              public ICanConsumer,
              public std::enable_shared_from_this<Speed> {
public:
  explicit Speed();
  ~Speed();

  // ISensor interface
  const std::string &getName() const override;
  void updateSensorData() override;
  std::unordered_map<std::string, std::shared_ptr<SensorData>>
  getSensorData() const override;

  // ICanConsumer interface
  void onCanMessage(const CanMessage &message) override;
  uint16_t getCanId() const override { return canId; }

  // Lifecycle management
  void start();
  void stop();

private:
  void readSensor() override;
  void checkUpdated() override;
  void calculateSpeed();
  void calculateOdo();

  static constexpr uint16_t canId = 0x100;
  static constexpr uint16_t canId2 = 0x180;
  static constexpr uint16_t canId3 = 0x580;
  std::string _name;
  std::unordered_map<std::string, std::shared_ptr<SensorData>> _sensorData;

  // Vehicle parameters for calculations
  static constexpr unsigned int pulsesPerRevolution =
      18; // 18 holes in the disc
  static constexpr float wheelDiameter_mm =
      67.0f; // Wheel diameter in millimeters

  // Thread safety for CAN message handling
  mutable std::mutex data_mutex;
  std::atomic<bool> new_data_available{false};
  std::atomic<bool> subscribed{false};

  // Latest CAN message data
  uint8_t latest_data[8] = {0};
  uint8_t latest_length = 0;
  std::chrono::steady_clock::time_point latest_timestamp;
  std::chrono::steady_clock::time_point last_measurement_time;

  // Pulse tracking
  uint16_t last_pulse_delta = 0;
  uint32_t total_pulses = 0;

  // Odometer precision tracking
  double accumulated_distance_m = 0.0;
};

#endif
