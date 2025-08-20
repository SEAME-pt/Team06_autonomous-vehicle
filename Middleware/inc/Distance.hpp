#ifndef DISTANCE_HPP
#define DISTANCE_HPP

#include "CanMessageBus.hpp"
#include "ISensor.hpp"
#include <memory>
#include <unordered_map>

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

private:
  void readSensor() override;
  void checkUpdated() override;

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
};

#endif
