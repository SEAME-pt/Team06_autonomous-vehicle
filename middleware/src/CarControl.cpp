#include "../inc/CarControl.hpp"

CarControl::CarControl() : _running(false), _currenntGear(0), _currentSpeed(0)  {

}

CarControl::~CarControl() {
}

void CarControl::processJoystick() {
	int joy_fd = open("/dev/input/js0", O_RDONLY);
	if (joy_fd < 0) {
		std::cerr << "Erro ao abrir o joystick" << std::endl;
		return;
	}

	struct js_event js;

	while (_running) {
		ssize_t n = read(joy_fd, &js, sizeof(js));
		std::cout << "n:		" << n << "\n";
		std::cout << "Type:		" <<  static_cast<int>(js.type) << "\n";
		std::cout << "Number:	" <<  static_cast<int>(js.number) << "\n";

		if (n == sizeof(js)) {
			js.type &= ~JS_EVENT_INIT; // Ignora eventos de inicialização
			if (js.type == JS_EVENT_BUTTON){
				if (js.number == 4 && js.value == 1) { //Left Button BTN_TL
					_shiftDown();
					std::cout << "Shift Down: " << _currenntGear + 1 << ", Max speed" << _gear[_currenntGear] << std::endl;
				}
				else if (js.number == 5 && js.value == 1) { //Right Button BTN_TR
					_shiftUp();
					std::cout << "Shift Up: " << _currenntGear + 1 << ", Max speed" << _gear[_currenntGear] << std::endl;
				}
				else if ((js.number == 7 || js.number == 6) && js.value == 1) { //Start/Select Button BTN_START
					_backMotors.setSpeed(0);
					_running = false;
					std::cout << "Stop" << std::endl;
					break;
				}
			}
			else if (js.type == JS_EVENT_AXIS) {
				if (js.number == 0) { // ABS_X
					_fServo.set_steering( js.value / 32767.0 * 100);
				}
				else if (js.number == 5) { // ABS_RZ
					if (js.value == 0){
						_backMotors.setSpeed(0);
					}
					else {
						_backMotors.setSpeed(js.value / 32767.0 * 100);
					}
				}
			}
		}
		else if (n < 0) {
			std::cerr << "Erro ao ler o joystick" << std::endl;
			_running = false;
			break;
		}
	}
	close (joy_fd);
}

void CarControl::start() {
	_running = true;
	_controlThread = std::thread(&CarControl::processJoystick, this);
}

void CarControl::stop() {
	_running = false;
	if (_controlThread.joinable()) {
		_controlThread.join();
	}
	_backMotors.setSpeed(0);
	_fServo.set_steering(0);
}

void CarControl::_shiftDown() {
	if (_currenntGear > 0) {
		_currenntGear--;
	}
}

void CarControl::_shiftUp() {
	if (_currenntGear < 4) {
		_currenntGear++;
	}
}

