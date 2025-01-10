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

class Battery : public ISensor {
private:
    SensorData sensorData;
    int i2c_fd;                 // File descriptor for I2C
    int i2c_bus = 1;            // I2C bus number
    uint8_t adc_address = 0x41; // I2C ADC address

    const float ADC_REF = 3.3f;       // Reference voltage of ADC
    const uint16_t ADC_MAX = 65535;   // Maximum value of ADC (16-bit)
    const float VOLTAGE_DIVIDER = 17.0f; // Adjusted for 3S 18650 battery (12.6V max)

    const float MAX_VOLTAGE = 12.6f;     // Maximum voltage of the battery
    const float MIN_VOLTAGE = 9.0f;      // Minimum voltage of the battery
    const float NOMINAL_VOLTAGE = 11.1f; // Nominal voltage of the battery

    std::mutex mtx; // Mutex for thread-safe access to shared data

    int readI2CBlockData(uint8_t reg, uint8_t* data, size_t length); // Reads data from I2C
    std::string _name;

public:
    explicit Battery(const std::string& name); // Constructor with explicit specifier
    ~Battery(); // Destructor

    int read_adc(); // Reads raw ADC value
    float getVoltage(); // Gets battery voltage
    float getPercentage(); // Gets battery percentage
    std::string getStatus(float voltage); // Gets battery status (e.g., FULL, GOOD, LOW)
    std::vector<float> get_cell_voltages(float total_voltage); // Estimates cell voltages
    std::map<std::string, float> get_battery_info(); // Returns all battery info as a map
    void updateSensorData() override; // Updates sensor data

    std::string getName() const override; // Gets the name of the sensor
    SensorData getSensorData() override; // Gets sensor data
};
