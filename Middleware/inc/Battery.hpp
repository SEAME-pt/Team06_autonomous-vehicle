#ifndef BATTERY_HPP
#define BATTERY_HPP

#include "BatteryReader.hpp"
#include "ISensor.hpp"
#include <memory>
#include <unordered_map>

class Battery : public ISensor {
public:
    // Constructor with dependency injection
    explicit Battery(std::shared_ptr<IBatteryReader> reader = std::make_shared<BatteryReader>());
    ~Battery();
    const std::string& getName() const override;
    void updateSensorData() override;
    std::unordered_map<std::string, std::shared_ptr<SensorData>> getSensorData() const override;
    bool getCharging() const;

private:
    void readSensor() override;
    void checkUpdated() override;

    std::string _name;
    std::unordered_map<std::string, std::shared_ptr<SensorData>> _sensorData;
    std::shared_ptr<IBatteryReader> batteryReader;
};

#endif
