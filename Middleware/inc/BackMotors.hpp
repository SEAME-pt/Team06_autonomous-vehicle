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


class BackMotors{
private:
	const int _motorAddr = 0x60;

public:
	int _fdMotor;
	BackMotors();
	virtual ~BackMotors();
	virtual void open_i2c_bus();
	virtual bool init_motors();
	virtual bool setMotorPwm(const int channel, int value);
	virtual void setSpeed(int speed);

	virtual void writeByteData(int fd, uint8_t reg, uint8_t value);
	virtual uint8_t readByteData(int fd, uint8_t reg);

	int getFdMotor();
};

#endif
