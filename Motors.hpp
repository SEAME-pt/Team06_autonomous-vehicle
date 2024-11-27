#ifndef MOTORS_HPP
#define MOTORS_HPP

#include <thread>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>    // Para close(), usleep()
#include <fcntl.h>     // Para open()
#include <sys/ioctl.h> 
#include <linux/i2c-dev.h>// Interface padrão do Linux para I2C


class Motors{
private:
	std::string i2c_device;
	const int _servoAddr = 0x40;
	const int _motorAddr = 0x60;
	const int _maxAngle = 90;
	const int _servoCenterPwm = 307;	
	const int _servoLeftPwm = 225;
	const int _servoRightPwm = 389;
	const int _sterringChannel = 0;

	int _fdServo;
	int _fdMotor;
	int _currentSpeed = 0;
	int _currentAngle;
	bool _running;

public:
	Motors();
	~Motors();
	bool init_servo();
	bool setServoPwm(const int channel, int on_value, int off_value);
	bool init_motors();
	bool setMotorPwm(const int channel, int value);


	void set_steering(int angle);
	void setSpeed(int speed);

	void writeByteData(int fd, uint8_t reg, uint8_t value);
	uint8_t readByteData(int addr, uint8_t reg);

};

#endif