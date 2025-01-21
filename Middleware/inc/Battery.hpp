#pragma once

#include "ISensor.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <map>
#include <thread>
#include <mutex>
#include <ctime>
#include <cmath>

class Battery : public ISensor {
private:
    SensorData sensorData;
    int i2c_fd;                 // File descriptor for I2C
    int i2c_bus = 1;            // I2C bus number
    uint8_t adc_address = 0x41; // I2C ADC address

    const float ADC_REF = 3.3f;       // Reference voltage of ADC
    const uint16_t ADC_MAX = 65535;   // Maximum value of ADC (16-bit)
    const float VOLTAGE_DIVIDER = 17.0f; // Adjusted for 3S 18650 battery (12.6V max)

    const float MAX_VOLTAGE = 12.6f;
    const float MIN_VOLTAGE = 9.0f;
    const float NOMINAL_VOLTAGE = 11.1f;

    std::mutex mtx;
    bool charging;

    int readI2CBlockData(uint8_t reg, uint8_t* data, size_t length);
    bool isCharging();

public:
    explicit Battery(const std::string& name);
    ~Battery();

    bool getCharging();
    int read_adc();
    int	read_charge();
    float getVoltage();
    unsigned int getPercentage();
    void updateSensorData() override;
    std::string getName() const override;
    bool getCritical() const override;
    SensorData getSensorData() override;
    std::mutex& getMutex() override;
};
