#ifndef BACKMOTORS_HPP
#define BACKMOTORS_HPP

#include <thread>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>    // Para close(), usleep()
#include <fcntl.h>     // Para open()
#include <sys/ioctl.h>
#include <csignal> // Biblioteca para manipulação de sinais
#include <atomic>
#include <chrono>
#include <linux/i2c-dev.h>// Interface padrão do Linux para I2C

// Forward declaration
class BackMotors;

// Interface for BackMotors
class IBackMotors {
public:
	virtual ~IBackMotors() = default;
	virtual void open_i2c_bus() = 0;
	virtual bool init_motors() = 0;
	virtual bool setMotorPwm(const int channel, int value) = 0;
	virtual void setSpeed(int speed) = 0;
	virtual void writeByteData(int fd, uint8_t reg, uint8_t value) = 0;
	virtual uint8_t readByteData(int fd, uint8_t reg) = 0;
	virtual int getFdMotor() = 0;
};

class BackMotors : public IBackMotors {
private:
	const int _motorAddr = 0x60;

public:
	int _fdMotor;
	BackMotors();
	~BackMotors() override;
	void open_i2c_bus() override;
	bool init_motors() override;
	bool setMotorPwm(const int channel, int value) override;
	void setSpeed(int speed) override;

	void writeByteData(int fd, uint8_t reg, uint8_t value) override;
	uint8_t readByteData(int fd, uint8_t reg) override;

	int getFdMotor() override;
};

#endif
