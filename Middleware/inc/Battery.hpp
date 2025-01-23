#ifndef BATTERY_HPP
#define BATTERY_HPP

#include "BatteryReader.hpp"
#include "ISensor.hpp"

class Battery : public ISensor {
public:
    Battery(const std::string& name);
    ~Battery();
    std::string getName() const override;
    void updateSensorData() override;
    bool getCritical() const override;
    SensorData getSensorData() override;
    std::mutex& getMutex() override;
    unsigned int calcPercentage();
    bool getCharging();

private:
    SensorData sensorData;
    std::mutex mtx;
    BatteryReader batteryReader;
    bool charging;
    unsigned int percentage;
};

#endif
