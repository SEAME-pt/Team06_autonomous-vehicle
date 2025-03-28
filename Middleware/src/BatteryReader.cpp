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

int BatteryReader::read_adc(uint8_t reg) {
    uint8_t data[2];

    if (write(i2c_fd, &reg, 1) != 1) {
        throw std::runtime_error("Write failure on the I2C bus");
    }

    if (read(i2c_fd, data, 2) != 2) {
        throw std::runtime_error("Failed to read from I2C bus");
    }

    int raw_value = (data[0] << 8) | data[1];  // Combina os dois bytes lidos em um valor bruto
    int raw = (raw_value >> 3) & 0x1FFF;

    return (raw_value >> 3) & 0x1FFF; // Remove os 3 bits menos significativos (status) & retorna os 13 bits restantes
}


int BatteryReader::read_charge() {
    uint8_t reg = 0x01;

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
    int adc_value = read_adc(0x02);
    float voltage = adc_value * 0.004; // Cada bit do ADC representa 4mV
    return voltage;
}

float BatteryReader::getShunt() {
    int adc_value = read_adc(0x01);
    float voltage = adc_value * 0.00001; // Cada bit do ADC representa 10µV
    return voltage;
}

bool BatteryReader::isCharging(){
    int value = read_charge();
    return (value < 255 && value > 0);
}

unsigned int BatteryReader::getPercentage() {
	float voltage = getVoltage();
    float loadVoltage = voltage + getShunt();
    float percentage = (loadVoltage - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE) * 100.0f;
    if (percentage < 1.0f)
        percentage = 1.0f;
    else if (percentage > 100.0f)
        percentage = 100.0f;

    return static_cast<unsigned int>(percentage);
}
