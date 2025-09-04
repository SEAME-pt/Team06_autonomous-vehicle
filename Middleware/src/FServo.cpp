#include "FServo.hpp"

FServo::FServo() {}

// LCOV_EXCL_START - Hardware I2C initialization, not testable in unit tests
void FServo::open_i2c_bus() {
  i2c_device = "/dev/i2c-1";
  _fdServo = open(i2c_device.c_str(), O_RDWR);
  if (_fdServo < 0)
    throw std::runtime_error("Error open I2C");
  if (ioctl(_fdServo, I2C_SLAVE, _servoAddr) < 0) {
    close(_fdServo);
    throw std::runtime_error("Erro ao configurar endereço I2C do servo.");
  }
}
// LCOV_EXCL_STOP

FServo::~FServo() {
  close(_fdServo);                  // LCOV_EXCL_LINE - Hardware cleanup
  std::cout << "destructor call\n"; // LCOV_EXCL_LINE - Destructor logging
}

// LCOV_EXCL_START - Hardware servo initialization, not testable in unit tests
bool FServo::init_servo() {
  try {
    // Reset PCA9685
    this->writeByteData(_fdServo, 0x00, 0x06);
    usleep(100000); // Aguarda 100 ms

    // Setup servo control
    this->writeByteData(_fdServo, 0x00, 0x10);
    usleep(100000);

    // Set frequency (~50Hz)
    this->writeByteData(_fdServo, 0xFE, 0x79);
    usleep(100000);

    // Configure MODE2
    this->writeByteData(_fdServo, 0x01, 0x04);
    usleep(100000);

    // Enable auto-increment
    this->writeByteData(_fdServo, 0x00, 0x20);
    usleep(100000);
    // LCOV_EXCL_STOP
    return true;
  } catch (
      const std::exception &e) { // LCOV_EXCL_LINE - Hardware error handling
    std::cerr << "Erro ao inicializar o servo: " << e.what()
              << std::endl; // LCOV_EXCL_LINE - Hardware error handling
    return false;           // LCOV_EXCL_LINE - Hardware error handling
  }
}

bool FServo::setServoPwm(const int channel, int on_value, int off_value) {
  try {
    // LCOV_EXCL_START - Hardware servo PWM configuration, not testable in unit
    // tests
    this->writeByteData(_fdServo, 0x06 + 4 * channel, on_value & 0xFF);
    this->writeByteData(_fdServo, 0x07 + 4 * channel, on_value >> 8);
    this->writeByteData(_fdServo, 0x08 + 4 * channel, off_value & 0xFF);
    this->writeByteData(_fdServo, 0x09 + 4 * channel, off_value >> 8);
    // LCOV_EXCL_STOP
    return true;
  } catch (
      const std::exception &e) { // LCOV_EXCL_LINE - Hardware error handling
    std::cerr << "Erro ao configurar PWM do servo: " << e.what()
              << std::endl; // LCOV_EXCL_LINE - Hardware error handling
    return false;           // LCOV_EXCL_LINE - Hardware error handling
  }
}

void FServo::set_steering(int angle) {
  /* """Set steering angle (-90 to +90 degrees)""" */
  angle = std::max(-_maxAngle, std::min(_maxAngle, angle));

  // LCOV_EXCL_START - Hardware servo PWM calculation and control, not testable
  // in unit tests
  int pwm;
  if (angle < 0) {
    // Calcula o PWM para ângulo negativo
    pwm = static_cast<int>(_servoCenterPwm +
                           (static_cast<float>(angle) / _maxAngle) *
                               (_servoCenterPwm - _servoLeftPwm));
  } else if (angle > 0) {
    // Calcula o PWM para ângulo positivo
    pwm = static_cast<int>(_servoCenterPwm +
                           (static_cast<float>(angle) / _maxAngle) *
                               (_servoRightPwm - _servoCenterPwm));
  } else
    pwm = _servoCenterPwm;
  setServoPwm(_sterringChannel, 0, pwm);
  // LCOV_EXCL_STOP
  _currentAngle = angle;
}

// LCOV_EXCL_START - Hardware I2C write, not testable in unit tests
void FServo::writeByteData(int fd, uint8_t reg, uint8_t value) {
  uint8_t buffer[2] = {reg, value};
  if (write(fd, buffer, 2) != 2) {
    throw std::runtime_error("Erro ao escrever no dispositivo I2C.");
  }
  // LCOV_EXCL_STOP
}

// LCOV_EXCL_START - Hardware I2C read, not testable in unit tests
uint8_t FServo::readByteData(int fd, uint8_t reg) {
  if (write(fd, &reg, 1) != 1)
    throw std::runtime_error(
        "Erro ao enviar o registrador ao dispositivo I2C.");
  uint8_t value;
  if (read(fd, &value, 1) != 1)
    throw std::runtime_error("Erro ao ler o registrador ao dispositivo I2C.");
  return value;
  // LCOV_EXCL_STOP
}
