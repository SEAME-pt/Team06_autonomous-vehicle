#ifndef SPEED_HPP
#define SPEED_HPP

#include "ISensor.hpp"
#include "MockCanReader.hpp"
#include "CanReader.hpp"
#include <memory>
#include <unordered_map>

class Speed : public ISensor {
public:
    // Constructor with dependency injection
    explicit Speed(std::shared_ptr<ICanReader> canReader = nullptr);
    ~Speed();
    const std::string& getName() const override;
    void updateSensorData() override;
    std::unordered_map<std::string, std::shared_ptr<SensorData>> getSensorData() const override;

private:
    void readSensor() override;
    void checkUpdated() override;
    void calculateOdo();

    std::string _name;
    std::unordered_map<std::string, std::shared_ptr<SensorData>> _sensorData;
    uint16_t canId = 0x100;
    uint8_t buffer[8];
    uint8_t length = 0;
    std::shared_ptr<ICanReader> can;
};

#endif
