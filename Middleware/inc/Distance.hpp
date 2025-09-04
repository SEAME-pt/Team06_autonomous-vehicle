#ifndef DISTANCE_HPP
#define DISTANCE_HPP

#include "CanMessageBus.hpp"
#include "ISensor.hpp"
#include <chrono>
#include <functional>
#include <memory>
#include <unordered_map>

class Distance : public ISensor,
                 public ICanConsumer,
                 public std::enable_shared_from_this<Distance> {
public:
  explicit Distance();
  ~Distance();

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

  // Set emergency brake callback (replaces ZMQ publisher)
  void setEmergencyBrakeCallback(std::function<void(bool)> callback);

private:
  void readSensor() override;
  void checkUpdated() override;
  void calculateCollisionRisk(bool has_new_data);
  void triggerEmergencyBrake(bool emergency_active);

  static constexpr uint16_t canId = 0x101;
  static constexpr uint16_t canId2 = 0x181;
  static constexpr uint16_t canId3 = 0x581;
  std::string _name;
  std::unordered_map<std::string, std::shared_ptr<SensorData>> _sensorData;

  // Thread safety for CAN message handling
  mutable std::mutex data_mutex;
  std::atomic<bool> new_data_available{false};
  std::atomic<bool> subscribed{false};

  // Latest CAN message data
  uint8_t latest_data[8] = {0};
  uint8_t latest_length = 0;
  std::chrono::steady_clock::time_point latest_timestamp;

  // Emergency brake callback for direct communication (replaces ZMQ publisher)
  std::function<void(bool)> emergency_brake_callback;

  // Distance-based collision detection parameters
  static constexpr double MAX_DISTANCE_CM = 100.0; // Maximum sensor range
  // Fixed thresholds:
  // - Emergency: 20cm
  // - Warning: 25cm

  // Current state
  std::atomic<uint16_t> current_distance_cm{0};
  std::atomic<int> risk_level{0}; // 0 = safe, 1 = warning, 2 = emergency
  std::atomic<bool> emergency_brake_active{false};
};

#endif
