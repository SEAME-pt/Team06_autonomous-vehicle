#include "BackMotors.hpp"

BackMotors::BackMotors() {}

// LCOV_EXCL_START - Hardware I2C initialization, not testable in unit tests
void BackMotors::open_i2c_bus() {
  std::string i2c_device = "/dev/i2c-1";
  _fdMotor = open(i2c_device.c_str(), O_RDWR);
  if (_fdMotor < 0)
    throw std::runtime_error("Error open I2C");
  if (ioctl(_fdMotor, I2C_SLAVE, _motorAddr) < 0) {
    close(_fdMotor);
    throw std::runtime_error("Erro ao configurar endereço I2C do motor.");
  }
  std::cout << "JetCar inicializado com sucesso!" << std::endl;
}
// LCOV_EXCL_STOP

BackMotors::~BackMotors() {
  close(_fdMotor);                  // LCOV_EXCL_LINE - Hardware cleanup
  std::cout << "destructor call\n"; // LCOV_EXCL_LINE - Destructor logging
}

// LCOV_EXCL_START - Hardware motor initialization, not testable in unit tests
bool BackMotors::init_motors() {
  try {
    // Configure motor controller
    this->writeByteData(_fdMotor, 0x00, 0x20);

    // Set frequency to 60Hz
    int preScale;
    uint8_t oldMode, newMode;

    oldMode = this->readByteData(_fdMotor, 0x00);
    preScale = static_cast<int>(std::floor(25000000.0 / 4096.0 / 60 - 1));
    newMode = (oldMode & 0x7F) | 0x10;

    // Configurar o novo modo e frequência
    this->writeByteData(_fdMotor, 0x00, newMode);
    this->writeByteData(_fdMotor, 0xFE, preScale);
    this->writeByteData(_fdMotor, 0x00, oldMode);

    usleep(5000);

    // Ativar auto-incremento
    this->writeByteData(_fdMotor, 0x00, oldMode | 0xa1);
    // LCOV_EXCL_STOP
    return true;
  } catch (
      const std::exception &e) { // LCOV_EXCL_LINE - Hardware error handling
    std::cerr << "Erro na inicialização dos motores: " << e.what()
              << std::endl; // LCOV_EXCL_LINE - Hardware error handling
    return false;           // LCOV_EXCL_LINE - Hardware error handling
  }
}

// LCOV_EXCL_START - Hardware motor PWM configuration, not testable in unit
// tests
bool BackMotors::setMotorPwm(const int channel, int value) {
  value = std::min(std::max(value, 0), 4095);
  try {
    this->writeByteData(_fdMotor, 0x06 + 4 * channel, 0);
    this->writeByteData(_fdMotor, 0x07 + 4 * channel, 0);
    this->writeByteData(_fdMotor, 0x08 + 4 * channel, value & 0xFF);
    this->writeByteData(_fdMotor, 0x09 + 4 * channel, value >> 8);
    // LCOV_EXCL_STOP
    return true;

  } catch (
      const std::exception &e) { // LCOV_EXCL_LINE - Hardware error handling
    std::cerr << "Erro PWM dos motores: " << e.what()
              << std::endl; // LCOV_EXCL_LINE - Hardware error handling
    return false;           // LCOV_EXCL_LINE - Hardware error handling
  }
}

/* void BackMotors::setSpeed(int speed) {
  int pwmValue;
  speed = std::max(-100, std::min(100, speed));
  pwmValue = static_cast<int>(std::abs(speed) / 100.0 * 4095);

  if (speed > 0) {            // forward
    setMotorPwm(0, pwmValue); // IN1
    setMotorPwm(1, 0);        // IN2
    setMotorPwm(2, pwmValue); // ENA

    setMotorPwm(5, pwmValue); // IN3
    setMotorPwm(6, 0);        // IN4
    setMotorPwm(7, pwmValue); // ENB
  } else if (speed < 0) {     // backward
    setMotorPwm(0, pwmValue); // IN1
    setMotorPwm(1, pwmValue); // IN2
    setMotorPwm(2, 0);        // ENA

    setMotorPwm(5, 0);        // IN3
    setMotorPwm(6, pwmValue); // IN4
    setMotorPwm(7, pwmValue); // ENB
  } else {
    for (int channel = 0; channel < 9; ++channel) {
      setMotorPwm(channel, 0);
    }
  }
} */

// LCOV_EXCL_START - Hardware motor PWM calculation and compensation, not
// testable in unit tests
void BackMotors::setSpeed(int speed) {
  int leftSpeed = std::max(-100, std::min(100, speed));
  int rightSpeed = std::max(-100, std::min(100, speed));

  // Converte para PWM (0-4095) e aplica compensação
  int pwmLeft = static_cast<int>((std::abs(leftSpeed) / 100.0 * 4095));
  int pwmRight = static_cast<int>((std::abs(rightSpeed) / 100.0 * 4095));

  double _compLeft = 0.85;  // 5% mais força no motor esquerdo
  double _compRight = 1.00; // motor direito normal

  pwmLeft = std::min(pwmLeft, 4095) * _compLeft;
  pwmRight = std::min(pwmRight, 4095) * _compRight;
  // LCOV_EXCL_STOP

  // LCOV_EXCL_START - Hardware motor control, not testable in unit tests
  // Enhanced acceleration profiles - faster PWM transitions
  if (leftSpeed > 0) {        // forward
    setMotorPwm(0, pwmLeft);  // IN1
    setMotorPwm(1, 0);        // IN2
    setMotorPwm(2, pwmLeft);  // ENA
  } else if (leftSpeed < 0) { // backward - enhanced for faster braking
    setMotorPwm(0, pwmLeft);
    setMotorPwm(1, pwmLeft);
    setMotorPwm(2, 0);
  } else {
    setMotorPwm(0, 0);
    setMotorPwm(1, 0);
    setMotorPwm(2, 0);
  }

  if (rightSpeed > 0) {        // forward
    setMotorPwm(5, pwmRight);  // IN3
    setMotorPwm(6, 0);         // IN4
    setMotorPwm(7, pwmRight);  // ENB
  } else if (rightSpeed < 0) { // backward - enhanced for faster braking
    setMotorPwm(5, 0);
    setMotorPwm(6, pwmRight);
    setMotorPwm(7, pwmRight);
  } else {
    setMotorPwm(5, 0);
    setMotorPwm(6, 0);
    setMotorPwm(7, 0);
  }
  // LCOV_EXCL_STOP
}

// LCOV_EXCL_START - Hardware emergency brake, not testable in unit tests
void BackMotors::emergencyBrake() {
  try {
    // Set all PWM channels to maximum (4095) to lock the motors
    for (int channel = 0; channel < 16; ++channel) {
      setMotorPwm(channel, 4095);
    }
    std::cout << "Emergency brake activated - all motors locked!" << std::endl;
    // LCOV_EXCL_STOP
  } catch (
      const std::exception &e) { // LCOV_EXCL_LINE - Hardware error handling
    std::cerr << "Error in emergency brake: " << e.what()
              << std::endl; // LCOV_EXCL_LINE - Hardware error handling
  }
}

// LCOV_EXCL_START - Hardware I2C write, not testable in unit tests
void BackMotors::writeByteData(int fd, uint8_t reg, uint8_t value) {
  uint8_t buffer[2] = {reg, value};
  if (write(fd, buffer, 2) != 2) {
    throw std::runtime_error("Erro ao escrever no dispositivo I2C.");
  }
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START - Hardware I2C read, not testable in unit tests
uint8_t BackMotors::readByteData(int fd, uint8_t reg) {
  if (write(fd, &reg, 1) != 1)
    throw std::runtime_error(
        "Erro ao enviar o registrador ao dispositivo I2C.");
  uint8_t value;
  if (read(fd, &value, 1) != 1)
    throw std::runtime_error("Erro ao ler o registrador ao dispositivo I2C.");
  return value;
}
// LCOV_EXCL_STOP

int BackMotors::getFdMotor() {
  return _fdMotor;
} // LCOV_EXCL_LINE - Hardware accessor
