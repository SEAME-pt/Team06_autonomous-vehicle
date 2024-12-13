#pragma once

#include <thread>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>    // Para close(), usleep()
#include <fcntl.h>     // Para open()
#include <sys/ioctl.h> 
#include <linux/i2c-dev.h>// Interface padrão do Linux para I2C

#include <csignal> // Biblioteca para manipulação de sinais

#include <gpiod.h> // Biblioteca libgpiod
#include <atomic>
#include <chrono> // Para cálculos de tempo
#include <csignal> // Biblioteca para manipulação de sinais

class FServo{
private:
	std::string i2c_device;
	const int _servoAddr = 0x40;
	const int _maxAngle = 90;
	const int _servoCenterPwm = 320;
	const int _servoLeftPwm = 320 - 200;
	const int _servoRightPwm = 320 + 200;
	const int _sterringChannel = 0;

	int _fdServo;
	int _currentAngle;
public:
	FServo();
	~FServo();
	bool init_servo();
	bool setServoPwm(const int channel, int on_value, int off_value);

	void writeByteData(int fd, uint8_t reg, uint8_t value);
	uint8_t readByteData(int addr, uint8_t reg);
};