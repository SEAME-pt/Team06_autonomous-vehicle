#ifndef MOCKBATTERYREADER_HPP
#define MOCKBATTERYREADER_HPP

#include "BatteryReader.hpp"

class MockBatteryReader : public IBatteryReader {
public:
    MockBatteryReader() :
        _voltage(12.0f),
        _shunt(0.0f),
        _percentage(80),
        _isCharging(false) {}

    ~MockBatteryReader() override = default;

    // Allow setting mock values for testing
    void setVoltage(float voltage) { _voltage = voltage; }
    void setShunt(float shunt) { _shunt = shunt; }
    void setPercentage(unsigned int percentage) { _percentage = percentage; }
    void setCharging(bool charging) { _isCharging = charging; }

    // IBatteryReader interface implementation
    int read_adc(uint8_t reg) override {
        // Mock implementation that returns values based on register
        if (reg == 0x01) {
            return static_cast<int>(_shunt * 100000); // Convert to ADC value (10µV per bit)
        } else if (reg == 0x02) {
            return static_cast<int>(_voltage * 250); // Convert to ADC value (4mV per bit)
        }
        return 0;
    }

    int read_charge() override {
        return _isCharging ? 1 : 0;
    }

    float getVoltage() override {
        return _voltage;
    }

    float getShunt() override {
        return _shunt;
    }

    unsigned int getPercentage() override {
        return _percentage;
    }

    bool isCharging() override {
        return _isCharging;
    }

private:
    float _voltage;
    float _shunt;
    unsigned int _percentage;
    bool _isCharging;
};

#endif
