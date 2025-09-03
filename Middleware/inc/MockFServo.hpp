#ifndef MOCKFSERVO_HPP
#define MOCKFSERVO_HPP

#include "FServo.hpp"
#include <map>
#include <memory>

class MockFServo : public IFServo {
private:
  std::map<int, std::pair<int, int>>
      servo_pwm_values;                 // channel -> (on_value, off_value)
  std::map<uint8_t, uint8_t> registers; // reg -> value
  int angle;
  bool initialized;
  bool i2c_opened;
  bool simulate_i2c_failure;
  int fd_servo = 1; // Default fd value for testing

public:
  MockFServo()
      : angle(0), initialized(false), i2c_opened(false),
        simulate_i2c_failure(false) {}

  ~MockFServo() override = default;

  void open_i2c_bus() override {
    if (simulate_i2c_failure) {
      throw std::runtime_error("Simulated I2C bus open failure");
    }
    i2c_opened = true;
  }

  bool init_servo() override {
    if (simulate_i2c_failure) {
      return false;
    }

    // Simulate the real FServo::init_servo() register writes
    // Reset PCA9685
    writeByteData(fd_servo, 0x00, 0x06);

    // Setup servo control
    writeByteData(fd_servo, 0x00, 0x10);

    // Set frequency (~50Hz)
    writeByteData(fd_servo, 0xFE, 0x79);

    // Configure MODE2
    writeByteData(fd_servo, 0x01, 0x04);

    // Enable auto-increment
    writeByteData(fd_servo, 0x00, 0x20);

    initialized = true;
    return true;
  }

  bool setServoPwm(const int channel, int on_value, int off_value) override {
    if (simulate_i2c_failure) {
      return false;
    }
    servo_pwm_values[channel] = std::make_pair(on_value, off_value);
    return true;
  }

  void set_steering(int newAngle) override {
    angle = newAngle;

    // Calculate PWM value for the angle (similar to the real implementation)
    int pwm_value;
    const int servo_center_pwm = 320;
    const int servo_left_pwm = 170;  // 320 - 150
    const int servo_right_pwm = 470; // 320 + 150
    const int max_angle = 90;

    // Clamp angle to [-max_angle, max_angle]
    int clamped_angle = std::max(-max_angle, std::min(max_angle, newAngle));

    if (clamped_angle < 0) {
      // Negative angle (left)
      pwm_value = static_cast<int>(
          servo_center_pwm + (static_cast<float>(clamped_angle) / max_angle) *
                                 (servo_center_pwm - servo_left_pwm));
    } else if (clamped_angle > 0) {
      // Positive angle (right)
      pwm_value = static_cast<int>(
          servo_center_pwm + (static_cast<float>(clamped_angle) / max_angle) *
                                 (servo_right_pwm - servo_center_pwm));
    } else {
      // Center position
      pwm_value = servo_center_pwm;
    }

    // Set the PWM value for the steering channel (0)
    setServoPwm(0, 0, pwm_value);
  }

  void writeByteData(int fd, uint8_t reg, uint8_t value) override {
    if (simulate_i2c_failure) {
      throw std::runtime_error("Simulated I2C write failure");
    }
    registers[reg] = value;
  }

  uint8_t readByteData(int fd, uint8_t reg) override {
    if (simulate_i2c_failure) {
      throw std::runtime_error("Simulated I2C read failure");
    }
    return registers[reg];
  }

  // Helper methods for testing
  int getFd() const { return fd_servo; }

  void setFd(int fd) { fd_servo = fd; }

  bool isInitialized() const { return initialized; }

  bool isI2cOpened() const { return i2c_opened; }

  int getSteeringAngle() const { return angle; }

  std::pair<int, int> getServoPwm(int channel) const {
    auto it = servo_pwm_values.find(channel);
    if (it != servo_pwm_values.end()) {
      return it->second;
    }
    return std::make_pair(0, 0);
  }

  uint8_t getRegisterValue(uint8_t reg) const {
    auto it = registers.find(reg);
    if (it != registers.end()) {
      return it->second;
    }
    return 0;
  }

  // Set whether to simulate I2C failures
  void setSimulateI2cFailure(bool simulate) { simulate_i2c_failure = simulate; }

  // Clear all register values
  void clearRegisters() { registers.clear(); }
};

#endif
