#pragma once

#include <linux/joystick.h>



class BackMotors;
class FServo;

class CarControl {
private:
	BackMotors			_backMotors;
	FServo				_fServo;

	std::atomic<bool>	_running;
	std::thread			_controlThread;
	int 				_currenntGear;
	double				_currentSpeed;
	double 				_gear[5] = {20, 40, 60, 80, 100};

	void _shiftDown();
	void _shiftUp();

public:
	CarControl();
	~CarControl();

	void processJoystick();
	void process();
	void start();
	void stop();
};

#include "BackMotors.hpp"
#include "FServo.hpp"