#include "ControlAssembly.hpp"

ControlAssembly::ControlAssembly(const std::string &address,
                                 zmq::context_t &context,
                                 std::shared_ptr<IBackMotors> backMotors,
                                 std::shared_ptr<IFServo> fServo)
    : zmq_subscriber(address, context), stop_flag(false),
      _backMotors(backMotors ? backMotors : std::make_shared<BackMotors>()),
      _fServo(fServo ? fServo : std::make_shared<FServo>()),
      _logger("control_updates.log") {
  std::cout << "ControlAssembly initialized with ZMQ address: " << address
            << std::endl;

  // Initialize motors and servo
  try {
    _backMotors->open_i2c_bus();
    _fServo->open_i2c_bus();

    // Initialize motors
    if (!_backMotors->init_motors()) {
      throw std::runtime_error("Failed to initialize BackMotors");
    }

    // Initialize servo
    if (!_fServo->init_servo()) {
      throw std::runtime_error("Failed to initialize FServo");
    }

    std::cout << "Motors and servo initialized successfully" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error during initialization: " << e.what() << std::endl;
    throw; // Re-throw the exception to notify the caller
  }
}

ControlAssembly::~ControlAssembly() {
  std::cout << "ControlAssembly shutting down" << std::endl;
  stop();
  _backMotors->setSpeed(0);
  _fServo->set_steering(0);
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

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  std::cout << "Message receiver thread stopping" << std::endl;
}

void ControlAssembly::handleMessage(const std::string &message) {
  std::unordered_map<std::string, double> values;
  std::stringstream ss(message);
  std::string token;
  std::cout << "Parsing message: " << message << std::endl;
  while (std::getline(ss, token, ';')) {
    if (token.empty())
      continue;

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
    _fServo->set_steering(0);
    _backMotors->setSpeed(0);
    _logger.logControlUpdate("init", 0, 0);
    return;
  }

  double steering = 0.0;
  double throttle = 0.0;

  // Apply steering if present in the message
  if (values.find("steering") != values.end()) {
    steering = values["steering"];
    std::cout << "Setting steering to: " << steering << std::endl;
    _fServo->set_steering(static_cast<int>(steering));
  }

  // Apply throttle if present in the message
  if (values.find("throttle") != values.end()) {
    throttle = values["throttle"];
    std::cout << "Setting throttle to: " << throttle << std::endl;
    _backMotors->setSpeed(throttle);
  }

  // Log the control update
  _logger.logControlUpdate(message, steering, throttle);
}
