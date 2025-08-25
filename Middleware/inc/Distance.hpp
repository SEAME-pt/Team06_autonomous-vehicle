#ifndef DISTANCE_HPP
#define DISTANCE_HPP

#include "CanMessageBus.hpp"
#include "ISensor.hpp"
#include <memory>
#include <unordered_map>
#include <functional>
#include <chrono>

class Distance : public ISensor, public ICanConsumer, public std::enable_shared_from_this<Distance> {
public:
    explicit Distance();
  ~Distance();

    // ISensor interface
    const std::string& getName() const override;
  void updateSensorData() override;
    std::unordered_map<std::string, std::shared_ptr<SensorData>> getSensorData() const override;

    // ICanConsumer interface
    void onCanMessage(const CanMessage& message) override;
    uint16_t getCanId() const override { return canId; }

    // Lifecycle management
    void start();
    void stop();

    // Set speed data accessor
    void setSpeedDataAccessor(std::function<std::shared_ptr<SensorData>()> accessor);

private:
  void readSensor() override;
  void checkUpdated() override;
  void calculateCollisionRisk();
  double calculateStoppingDistance(double speed_mps) const;
  double calculateTimeToCollision(double distance_cm, double speed_mps, double closing_rate_mps) const;
  bool isObjectApproaching() const;

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

    // Speed data accessor for collision detection
    std::function<std::shared_ptr<SensorData>()> speed_data_accessor;

    // Collision detection parameters (all in metric units)
    static constexpr double MAX_SPEED_KPH = 13.0;           // Maximum vehicle speed
    static constexpr double MAX_DISTANCE_CM = 500.0;        // Maximum sensor range
    static constexpr double DECELERATION_MPS2 = 2.0;        // Assumed braking deceleration (m/s²)
    static constexpr double REACTION_TIME_S = 1.0;          // Human reaction time (seconds)
    static constexpr double EMERGENCY_TIME_S = 0.5;         // Time for emergency braking (seconds)
    static constexpr double WARNING_SAFETY_FACTOR = 2.0;    // Safety factor for warning
    static constexpr double EMERGENCY_SAFETY_FACTOR = 1.2;  // Safety factor for emergency braking

    // Distance tracking for approach detection
    struct DistanceHistory {
        uint16_t distance_cm;
        std::chrono::steady_clock::time_point timestamp;
    };

    static constexpr size_t HISTORY_SIZE = 5;
    DistanceHistory distance_history[HISTORY_SIZE];
    size_t history_index = 0;
    bool history_full = false;

    // Current state
    std::atomic<uint16_t> current_distance_cm{0};
    std::atomic<bool> object_approaching{false};
    std::atomic<int> risk_level{0}; // 0 = safe, 1 = warning, 2 = emergency
};

#endif
