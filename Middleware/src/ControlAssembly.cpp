#include "ControlAssembly.hpp"

ControlAssembly::ControlAssembly(const std::string& address, zmq::context_t& context)
    : zmq_subscriber(address, context), stop_flag(false) {
    std::cout << "ControlAssembly initialized with ZMQ address: " << address << std::endl;
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
    std::cout << "Motors and servo initialized successfully" << std::endl;
}

ControlAssembly::~ControlAssembly() {
    std::cout << "ControlAssembly shutting down" << std::endl;
    stop();
    _backMotors.setSpeed(0);
	_fServo.set_steering(0);
    std::cout << "Motor speed and steering set to 0" << std::endl;
}

void ControlAssembly::start() {
    std::cout << "Starting ControlAssembly message receiver thread" << std::endl;
    stop_flag = false;
    _listenerThread = std::thread(&ControlAssembly::receiveMessages, this);
}

void ControlAssembly::stop() {
    std::cout << "Stopping ControlAssembly" << std::endl;
    if (!stop_flag.exchange(true)) {
        if (_listenerThread.joinable()) {
            _listenerThread.join();
            std::cout << "Message receiver thread joined" << std::endl;
        }
    }
}

void ControlAssembly::receiveMessages() {
    std::cout << "Message receiver thread started" << std::endl;
    while (!stop_flag) {
        std::string message = zmq_subscriber.receive();

        if (!message.empty()) {
            std::cout << "Received control message: " << message << std::endl;
            handleMessage(message);
        } else {
            std::cout << "Received empty message" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "Message receiver thread stopping" << std::endl;
}

void ControlAssembly::handleMessage(const std::string& message) {
    std::unordered_map<std::string, double> values;
    std::stringstream ss(message);
    std::string token;
    std::cout << "Parsing message: " << message << std::endl;
    while (std::getline(ss, token, ';')) {
        if (token.empty()) continue;

        std::string key;
        double value;
        std::stringstream ss_token(token);
        std::getline(ss_token, key, ':');
        ss_token >> value;
        values[key] = value;
        std::cout << "Parsed key: '" << key << "', value: " << value << std::endl;
    }

    // Handle special 'init' message
    if (message == "init;") {
        std::cout << "Received init message, resetting to zero values" << std::endl;
        _fServo.set_steering(0);
        _backMotors.setSpeed(0);
        return;
    }

    // Apply steering if present in the message
    if (values.find("steering") != values.end()) {
        int steering = static_cast<int>(values["steering"]);
        std::cerr << "Setting steering to: " << steering << std::endl;
        _fServo.set_steering(steering);
    }

    // Apply throttle if present in the message
    if (values.find("throttle") != values.end()) {
        double throttle = values["throttle"];
        std::cerr << "Setting throttle to: " << throttle << std::endl;
        _backMotors.setSpeed(throttle);
    }
}
