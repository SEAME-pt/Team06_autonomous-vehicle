#include "BatteryReader.hpp"

BatteryReader::BatteryReader() {
    try {
        std::string i2c_device = "/dev/i2c-" + std::to_string(i2c_bus);
        i2c_fd = open(i2c_device.c_str(), O_RDWR);
        if (i2c_fd < 0) {
            throw std::runtime_error("Failed to open I2C bus: " + i2c_device);
        }

        if (ioctl(i2c_fd, I2C_SLAVE, adc_address) < 0) {
            close(i2c_fd);
            throw std::runtime_error("Failed to set I2C slave address: " + std::to_string(adc_address));
        }
    } catch (...) {
        if (i2c_fd >= 0) {
            close(i2c_fd);
        }
        throw;
    }
}


BatteryReader::~BatteryReader() {
    if (i2c_fd >= 0) {
        close(i2c_fd);
    }
}

int BatteryReader::read_adc() {
    uint8_t reg = 0x02;
    uint8_t data[2];

    if (write(i2c_fd, &reg, 1) != 1) {
        throw std::runtime_error("Write failure on the I2C bus");
    }

    if (read(i2c_fd, data, 2) != 2) {
        throw std::runtime_error("Failed to read from I2C bus");
    }

    int raw_value = (data[0] << 8) | data[1];  //Combines the two bytes read into a raw value
    int raw = (raw_value >> 3) & 0x1FFF;

    return (raw_value >> 3) & 0x1FFF; // Remove the 3 least significant bits (status) and return the remaining 13 bits
}

int BatteryReader::read_charge() {
    uint8_t reg = 0x01; //
    if (write(i2c_fd, &reg, 1) != 1) {
        throw std::runtime_error("Error sending I2C command.");
    }
    uint8_t value;
    if (read(i2c_fd, &value, 1) != 1) {
        throw std::runtime_error("Error reading value from I2C register.");
    }

    return static_cast<int>(value);
}

float BatteryReader::getVoltage() {
    int adc_value = read_adc();
    float voltage = adc_value * 0.004; // Each ADC bit represents 4mV
    return voltage;
}

bool BatteryReader::isCharging(){
    int value = read_charge();
    return (value < 255 && value > 0);
}

unsigned int BatteryReader::getPercentage() {
	float voltage = getVoltage();
	float percentage = std::round((voltage - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE) * 100.0f);
    if (percentage < 1.0f)
        percentage = 1.0f;
    else if (percentage > 100.0f)
        percentage = 100.0f;

    return static_cast<unsigned int>(percentage);
}

std::string BatteryReader::getStatus() {
	float voltage = getVoltage();
	if (voltage >= 12.0f)
		return "FULL";
	else if (voltage >= 11.1f)
		return "GOOD";
	else if (voltage >= 10.2f)
		return "LOW";
	else
		return "CRITICAL";
}

std::map<std::string, float> BatteryReader::get_battery_info() {
    float voltage = getVoltage();
    float percentage = getPercentage();
    int adc_value = read_adc();
    int adc_charge = read_charge();
    float adc_voltage = (adc_value * ADC_REF) / ADC_MAX;

    std::map<std::string, float> info = {
        {"voltage", voltage},
        {"percentage", percentage},
        {"percentage round",std::round(percentage)},
        {"raw_adc", static_cast<float>(adc_value)},
        {"adc_voltage", adc_voltage},
    };

    return info;
}
