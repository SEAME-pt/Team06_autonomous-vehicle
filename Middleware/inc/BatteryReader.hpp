#ifndef BATTERYREADER_HPP
#define BATTERYREADER_HPP
#include <map>
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
#include <cmath>


class BatteryReader {
private:
	int i2c_fd;
	int i2c_bus = 1;
	uint8_t adc_address = 0x41;
	float percent_old = 100.0f;

	const float ADC_REF = 3.3f; // Reference voltage of ADC = 3.3V (ADC --Analog-to-Digital Converter-- )
	const uint16_t ADC_MAX = 65535; // Maximum value of ADC = 2^16-1(16-bit)
	const float VOLTAGE_DIVIDER =  17.0f;  // Ajustado para bateria 3S 18650 (12.6V máx)

	const float MAX_VOLTAGE = 12.6f; // Maximum voltage of battery = 12.6V
	const float MIN_VOLTAGE = 9.0f; // Minimum voltage of battery = 9.0V

public:

	BatteryReader();
	~BatteryReader();

	int	read_adc(uint8_t reg );
	int	read_charge();
	float getVoltage();
	float getShunt();
	unsigned int getPercentage();
	bool isCharging();
};

#endif
