#ifndef SPEED_HPP
#define SPEED_HPP

#include "ISensor.hpp"
#include "CanReader.hpp"

class Speed : public ISensor {
public:
    Speed(const std::string& name);
    ~Speed();
    std::string getName() const override;
    void updateSensorData() override;
    bool getCritical() const override;
    SensorData getSensorData() override;
    std::mutex& getMutex() override;

private:
    SensorData sensorData;
    std::mutex mtx;
    uint16_t canId = 0x100;
    uint8_t buffer[8];
    uint8_t length = 0;
    uint16_t speed;
    uint16_t rpm;
    CanReader can;
};

#endif
