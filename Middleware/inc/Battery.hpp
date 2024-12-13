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
#include <vector>
#include <thread>

class Battery : public ISensor {
private:
	int i2c_fd;
	int i2c_bus = 1;
	uint8_t adc_address = 0x41;

	const float ADC_REF = 3.3f; // Reference voltage of ADC = 3.3V
	const uint16_t ADC_MAX = 65535; // Maximum value of ADC = 2^16-1(16-bit)
	const float VOLTAGE_DIVIDER =  17.0f;  // Ajustado para bateria 3S 18650 (12.6V máx)

	const float MAX_VOLTAGE = 12.6f; // Maximum voltage of battery = 12.6V
	const float MIN_VOLTAGE = 9.0f; // Minimum voltage of battery = 9.0V
	const float NOMINAL_VOLTAGE = 11.1f; // Nominal voltage of battery = 11.1V

	int readI2CBlockData(uint8_t reg, uint8_t *data, size_t length);

    std::string _name = "Battery";

public:

	Battery();
	~Battery();

	int	read_adc();
	float getVoltage();
	float getPercentage();
	std::string getStatus(float voltage);
	std::vector<float> get_cell_voltages(float total_voltage); // Estimates cell voltages
	std::map<std::string, float> get_battery_info(); // Returns all battery info as a map
    std::string getName() const;
    float getValue() const;
};
