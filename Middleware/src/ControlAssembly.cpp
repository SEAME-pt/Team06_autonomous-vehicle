#include "ControlAssembly.hpp"

ControlAssembly::ControlAssembly(const std::string& address, zmq::context_t& context)
    : zmq_subscriber(address, context), stop_flag(false) {
    _backMotors.open_i2c_bus();
	_fServo.open_i2c_bus();
	if (!_backMotors.init_motors()){
		std::cerr << "Failed to initialize BackMotors" << std::endl;
		return;
	}
	if (!_fServo.init_servo()){
		std::cerr << "Failed to initialize FServo" << std::endl;
		return;
	}
}

ControlAssembly::~ControlAssembly() {
    stop();
    _backMotors.setSpeed(0);
	_fServo.set_steering(0);
}

void ControlAssembly::start() {
    stop_flag = false;
    _listenerThread = std::thread(&ControlAssembly::receiveMessages, this);
}

void ControlAssembly::stop() {
    if (!stop_flag.exchange(true)) {
        if (_listenerThread.joinable()) {
            _listenerThread.join();
        }
    }
}

void ControlAssembly::receiveMessages() {
    while (!stop_flag) {
        std::string message = zmq_subscriber.receive();

        if (!message.empty()) {
            handleMessage(message);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void ControlAssembly::handleMessage(const std::string& message) {
    std::unordered_map<std::string, double> values;
    std::stringstream ss(message);
    std::string token;
    while (std::getline(ss, token, ';')) {
        std::string key;
        double value;
        std::stringstream ss_token(token);
        std::getline(ss_token, key, ':');
        ss_token >> value;
        values[key] = value;
    }
    _fServo.set_steering(values["steering"]);
    _backMotors.setSpeed(values["throttle"]);
    return;
}
