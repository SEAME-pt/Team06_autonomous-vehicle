#ifndef SPEED_HPP
#define SPEED_HPP

#include "ISensor.hpp"
#include "CanReader.hpp"

class Speed : public ISensor {
public:
    Speed(const std::string& name);
    ~Speed();
    const std::string& getName() const override;
    void updateSensorData() override;
    bool getCritical() const override;
    const SensorData& getSensorData() const override;
    std::mutex& getMutex() override;
    bool getUpdated() const override;
    void readSpeed();

private:
    SensorData sensorData;
    std::mutex mtx;
    uint16_t canId = 0x100;
    uint8_t buffer[8];
    uint8_t length = 0;
    uint16_t speed;
    uint16_t rpm;
    unsigned int old;
    CanReader can;
};

#endif
