#pragma once

#include <chrono>
#include <cstdio>
#include <cstdarg>


#include "BackMotors.hpp"
#include "FServo.hpp"
#include "Controller.hpp"

class ControlAssembly{
private:
	BackMotors	_backMotors;
	FServo		_fServo;
	Controller	_controller;

	std::chrono::steady_clock::time_point	_last_update = std::chrono::steady_clock::now();
    const std::chrono::milliseconds			_update_interval = std::chrono::milliseconds(16); // ~60fps

    double accelaration = 0;
    double turn = 0;

public:
	ControlAssembly();
	~ControlAssembly();


};