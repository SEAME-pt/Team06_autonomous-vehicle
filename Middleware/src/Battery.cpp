#include "../inc/Battery.hpp"

Battery::Battery(const std::string& name) {
    // Open the I2C bus
    std::string i2c_device = "/dev/i2c-" + std::to_string(i2c_bus);
    i2c_fd = open(i2c_device.c_str(), O_RDWR);
    if (i2c_fd < 0) {
        throw std::runtime_error("Failed to open I2C bus");
    }

    // Set the I2C slave address
    if (ioctl(i2c_fd, I2C_SLAVE, adc_address) < 0) {
        close(i2c_fd);
        throw std::runtime_error("Failed to set I2C slave address");
    }
    // Initialize sensor data
    sensorData.value = 0;
    sensorData.timestamp = std::time(nullptr);
    sensorData.name = name;
    sensorData.critical = false;
    sensorData.updated = true;
}

Battery::~Battery() {
    if (i2c_fd >= 0) {
        close(i2c_fd);
    }
}
SensorData Battery::getSensorData() {
    std::lock_guard<std::mutex> lock(mtx);
    return sensorData;
}


void Battery::updateSensorData() {
    unsigned int tmp = sensorData.value;
    sensorData.value = getPercentage();
    sensorData.timestamp = std::time(nullptr);
    if (tmp != sensorData.value) {
        sensorData.updated = true;
    } else {
        sensorData.updated = false;
    }
}

int Battery::read_adc() {
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

int Battery::read_charge() {
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

float Battery::getVoltage() {
    int adc_value = read_adc();
    float voltage = adc_value * 0.004; // Each ADC bit represents 4mV
    return voltage;
}

bool Battery::isCharge(){
    int value = read_charge();
    return (value < 255 && value > 0);
}

unsigned int Battery::getPercentage() {
	float voltage = getVoltage();
	unsigned int percentage = static_cast<unsigned int>(std::round((voltage - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE) * 100.0f));
    bool charging = isCharge();

    if ( percentage > 100 ) {
        percentage = 100;
    } else if ( percentage < 1 ) {
        percentage = 1;
    } else {
        if ( charging ) {
            percentage = (percentage > sensorData.value ? percentage : sensorData.value);
        } else {
            percentage = (percentage < sensorData.value ? percentage : sensorData.value);
        }
    }
    std::cerr << "isCharge: " << charging << std::endl;
    std::cerr << "Percentage: " << percentage << std::endl;
    std::cerr << "SensorData: " << sensorData.value << std::endl;
	return percentage;
}

std::string Battery::getName() const {
    return sensorData.name;
}

std::mutex& Battery::getMutex() {
    return mtx;
}

bool Battery::getCritical() const {
    return sensorData.critical;
}
