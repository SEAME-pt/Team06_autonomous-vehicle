#ifndef MOCKBACKMOTORS_HPP
#define MOCKBACKMOTORS_HPP

#include "BackMotors.hpp"
#include <map>
#include <memory>

class MockBackMotors : public IBackMotors {
private:
  std::map<int, int> motor_pwm_values;  // channel -> value
  std::map<uint8_t, uint8_t> registers; // reg -> value
  bool initialized;
  bool i2c_opened;
  int current_speed;
  int fd_motor;
  bool simulate_i2c_failure;

public:
  MockBackMotors()
      : initialized(false), i2c_opened(false), current_speed(0), fd_motor(1),
        simulate_i2c_failure(false) {}

  ~MockBackMotors() override = default;

  void open_i2c_bus() override {
    if (simulate_i2c_failure) {
      throw std::runtime_error("Simulated I2C bus open failure");
    }
    i2c_opened = true;
  }

  bool init_motors() override {
    if (simulate_i2c_failure) {
      return false;
    }
    initialized = true;
    return true;
  }

  bool setMotorPwm(const int channel, int value) override {
    if (simulate_i2c_failure) {
      return false;
    }
    value = std::min(std::max(value, 0), 4095); // Clamp to valid PWM range
    motor_pwm_values[channel] = value;
    return true;
  }

  void setSpeed(int speed) override {
    // Save the original speed value (will be clamped internally)
    speed = std::max(-100, std::min(100, speed));
    current_speed = speed;

    // Calculate PWM value based on speed (0-4095)
    int pwmValue = static_cast<int>(std::abs(speed) / 100.0 * 4095);

    if (speed > 0) {            // Forward
      setMotorPwm(0, pwmValue); // IN1
      setMotorPwm(1, 0);        // IN2
      setMotorPwm(2, pwmValue); // ENA

      setMotorPwm(5, pwmValue); // IN3
      setMotorPwm(6, 0);        // IN4
      setMotorPwm(7, pwmValue); // ENB
    } else if (speed < 0) {     // Backward
      setMotorPwm(0, pwmValue); // IN1
      setMotorPwm(1, pwmValue); // IN2
      setMotorPwm(2, 0);        // ENA

      setMotorPwm(5, 0);        // IN3
      setMotorPwm(6, pwmValue); // IN4
      setMotorPwm(7, pwmValue); // ENB
    } else {                    // Stop
      for (int channel = 0; channel < 9; ++channel) {
        setMotorPwm(channel, 0);
      }
    }
  }

  void emergencyBrake() override {
    // Emergency brake implementation - immediately stop all motors
    current_speed = 0;
    for (int channel = 0; channel < 9; ++channel) {
      setMotorPwm(channel, 0);
    }
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

  int getFdMotor() override { return fd_motor; }

  // Helper methods for testing
  bool isInitialized() const { return initialized; }

  bool isI2cOpened() const { return i2c_opened; }

  int getCurrentSpeed() const { return current_speed; }

  int getMotorPwm(int channel) const {
    auto it = motor_pwm_values.find(channel);
    if (it != motor_pwm_values.end()) {
      return it->second;
    }
    return 0;
  }

  uint8_t getRegisterValue(uint8_t reg) const {
    auto it = registers.find(reg);
    if (it != registers.end()) {
      return it->second;
    }
    return 0;
  }

  void setFdMotor(int fd) { fd_motor = fd; }

  // Set whether to simulate I2C failures
  void setSimulateI2cFailure(bool simulate) { simulate_i2c_failure = simulate; }

  // Clear all register values
  void clearRegisters() { registers.clear(); }
};

#endif
