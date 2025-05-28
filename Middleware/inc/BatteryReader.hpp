#ifndef BATTERYREADER_HPP
#define BATTERYREADER_HPP
#include <cmath>
#include <cstdint>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <linux/i2c-dev.h>
#include <map>
#include <stdexcept>
#include <sys/ioctl.h>
#include <thread>
#include <unistd.h>
#include <vector>

// Interface for battery reading functionality
class IBatteryReader {
public:
  virtual ~IBatteryReader() = default;
  virtual int read_adc(uint8_t reg) = 0;
  virtual int read_charge() = 0;
  virtual float getVoltage() = 0;
  virtual float getShunt() = 0;
  virtual unsigned int getPercentage() = 0;
  virtual bool isCharging() = 0;
};

class BatteryReader : public IBatteryReader {
private:
  int i2c_fd = -1;
  int i2c_bus = 1;
  uint8_t adc_address = 0x41;
  float percent_old = 100.0f;
  bool test_mode = false;
  std::map<uint8_t, int> test_adc_values;
  int test_charge_value = 0;

  const float ADC_REF = 3.3f;     // Reference voltage of ADC = 3.3V (ADC
                                  // --Analog-to-Digital Converter-- )
  const uint16_t ADC_MAX = 65535; // Maximum value of ADC = 2^16-1(16-bit)
  const float VOLTAGE_DIVIDER =
      17.0f; // Ajustado para bateria 3S 18650 (12.6V máx)

public:
  const float MAX_VOLTAGE = 12.6f; // Maximum voltage of battery = 12.6V
  const float MIN_VOLTAGE = 9.0f;  // Minimum voltage of battery = 9.0V

  // Constructor with test_mode parameter
  BatteryReader(bool test_mode = false);
  ~BatteryReader() override;

  // Hardware interface methods
  int read_adc(uint8_t reg) override;
  int read_charge() override;

  // Data access methods
  float getVoltage() override;
  float getShunt() override;
  unsigned int getPercentage() override;
  bool isCharging() override;

  // Test mode methods
  bool isInTestMode() const { return test_mode; }
  void setTestAdcValue(uint8_t reg, int value);
  void setTestChargeValue(int value);
};

#endif
