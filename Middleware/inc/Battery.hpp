#ifndef BATTERY_HPP
#define BATTERY_HPP

#include "BatteryReader.hpp"
#include "ISensor.hpp"

class Battery : public ISensor {
public:
    Battery(const std::string& name);
    ~Battery();
    const std::string& getName() const override;
    void updateSensorData() override;
    bool getCritical() const override;
    const SensorData& getSensorData() const override;
    std::mutex& getMutex() override;
    unsigned int calcPercentage();
    bool getCharging() const;
    bool getUpdated() const override;
    const unsigned int& getValue() const override;


private:
    SensorData sensorData;
    std::mutex mtx;
    BatteryReader batteryReader;
    bool charging;
    unsigned int percentage;
};

#endif
