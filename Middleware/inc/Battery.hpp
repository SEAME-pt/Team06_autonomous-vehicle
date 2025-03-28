#ifndef BATTERY_HPP
#define BATTERY_HPP

#include "BatteryReader.hpp"
#include "ISensor.hpp"
#include <memory>
#include <unordered_map>

class Battery : public ISensor {
public:
    Battery();
    ~Battery();
    const std::string& getName() const override;
    void updateSensorData() override;
    std::unordered_map<std::string, std::shared_ptr<SensorData>> getSensorData() const override;
    bool getCharging() const;

private:
    void readSensor();
    void checkUpdated();

    std::string _name;
    std::unordered_map<std::string, std::shared_ptr<SensorData>> _sensorData;
    BatteryReader batteryReader;
};

#endif
