#include "ControlAssembly.hpp"

ControlAssembly::ControlAssembly(const std::string &address,
                                 zmq::context_t &context,
                                 std::shared_ptr<IBackMotors> backMotors,
                                 std::shared_ptr<IFServo> fServo,
                                 std::shared_ptr<ZmqPublisher> clusterPublisher)
    : zmq_subscriber(address, context), stop_flag(false), emergency_brake_active(false),
      auto_mode_active(false), is_braking(false), _context(context),
      _backMotors(backMotors ? backMotors : std::make_shared<BackMotors>()),
      _fServo(fServo ? fServo : std::make_shared<FServo>()),
      _clusterPublisher(clusterPublisher),
      _logger("control_updates.log") {
  std::cout << "ControlAssembly initialized with ZMQ address: " << address
            << std::endl;

  // Initialize autonomous control subscriber
  const std::string autonomous_address = "tcp://127.0.0.1:5560";
  _autonomousSubscriber = std::make_unique<ZmqSubscriber>(autonomous_address, context);
  std::cout << "Autonomous control subscriber initialized with address: " << autonomous_address << std::endl;

  // Initialize emergency brake subscriber
  const std::string emergency_brake_address = "tcp://127.0.0.1:5561";
  _emergencyBrakeSubscriber = std::make_unique<ZmqSubscriber>(emergency_brake_address, context);
  std::cout << "Emergency brake subscriber initialized with address: " << emergency_brake_address << std::endl;

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

  // Send initial mode status (manual mode)
  sendModeStatus(false);
}

ControlAssembly::~ControlAssembly() {
  std::cout << "ControlAssembly shutting down" << std::endl;

  // Deactivate auto mode and send status
  if (auto_mode_active.load()) {
    auto_mode_active.store(false);
    sendModeStatus(false);
  }

  stop();
  _backMotors->setSpeed(0);
  _fServo->set_steering(0);
  std::cout << "Motor speed and steering set to 0" << std::endl;
}

void ControlAssembly::setSpeedDataAccessor(std::function<std::shared_ptr<SensorData>()> accessor) {
  speed_data_accessor = accessor;
  std::cout << "Speed data accessor set for intelligent emergency braking" << std::endl;
}

void ControlAssembly::start() {
  std::cout << "Starting ControlAssembly message receiver threads" << std::endl;
  stop_flag = false;
  _listenerThread = std::thread(&ControlAssembly::receiveMessages, this);
  _autonomousListenerThread = std::thread(&ControlAssembly::receiveAutonomousMessages, this);
  _emergencyBrakeListenerThread = std::thread(&ControlAssembly::receiveEmergencyBrakeMessages, this);
}

void ControlAssembly::stop() {
  std::cout << "Stopping ControlAssembly" << std::endl;
  if (!stop_flag.exchange(true)) {
    if (_listenerThread.joinable()) {
      _listenerThread.join();
      std::cout << "Manual control receiver thread joined" << std::endl;
    }
    if (_autonomousListenerThread.joinable()) {
      _autonomousListenerThread.join();
      std::cout << "Autonomous control receiver thread joined" << std::endl;
    }
    if (_emergencyBrakeListenerThread.joinable()) {
      _emergencyBrakeListenerThread.join();
      std::cout << "Emergency brake receiver thread joined" << std::endl;
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

    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Reduced from 50ms to 10ms
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
    emergency_brake_active.store(false);
    _fServo->set_steering(0);
    _backMotors->setSpeed(0);
    _logger.logControlUpdate("init", 0, 0);
    return;
  }

  // Handle emergency brake commands with highest priority
  if (values.find("emergency_brake") != values.end()) {
    bool emergency_brake = (values["emergency_brake"] != 0.0);
    bool was_active = emergency_brake_active.exchange(emergency_brake);

    if (was_active != emergency_brake) {
      if (emergency_brake) {
        std::cout << "EMERGENCY BRAKE ACTIVATED - Intelligent braking engaged!" << std::endl;
        performEmergencyBraking(); // Use intelligent braking
        _logger.logControlUpdate("emergency_brake_activated", 0, 0);
      } else {
        std::cout << "Emergency brake deactivated - Normal control resumed" << std::endl;
        is_braking.store(false);
        _backMotors->setSpeed(0); // Ensure we stop when deactivating
        _logger.logControlUpdate("emergency_brake_deactivated", 0, 0);
      }
    }
    return; // Emergency brake commands are handled immediately and exclusively
  }

  // Handle AUTO mode toggle commands with second priority
  if (values.find("auto_mode") != values.end()) {
    bool new_auto_mode = (values["auto_mode"] != 0.0);
    bool was_auto_active = auto_mode_active.exchange(new_auto_mode);

    if (was_auto_active != new_auto_mode) {
      if (new_auto_mode) {
        std::cout << "AUTO MODE ACTIVATED - Switching to autonomous control" << std::endl;
        _logger.logControlUpdate("auto_mode_activated", 0, 0);
      } else {
        std::cout << "AUTO MODE DEACTIVATED - Switching to manual control" << std::endl;
        _logger.logControlUpdate("auto_mode_deactivated", 0, 0);
      }

      // Spam the mode change message to ensure cluster receives it
      for (int i = 0; i < 10; i++) {
        sendModeStatus(new_auto_mode);
      }
    }
    return; // Auto mode commands are handled immediately and exclusively
  }

  double steering = 0.0;
  double throttle = 0.0;

  // Only process manual control commands if AUTO mode is not active
  if (!auto_mode_active.load()) {
    // Apply steering if present in the message (emergency brake doesn't affect steering)
    if (values.find("steering") != values.end()) {
      steering = values["steering"];
      std::cout << "Setting steering to: " << steering << std::endl;
      _fServo->set_steering(static_cast<int>(steering));
    }

    // Apply throttle if present in the message, but only if emergency brake is NOT active
    if (values.find("throttle") != values.end()) {
      throttle = values["throttle"];

      if (emergency_brake_active.load()) {
        std::cout << "Emergency brake active - applying intelligent braking instead of throttle: " << throttle << std::endl;
        performEmergencyBraking(); // Use intelligent braking instead of just setting to 0
        throttle = 0; // Log as 0 for consistency
      } else {
        std::cout << "Setting throttle to: " << throttle << std::endl;
        _backMotors->setSpeed(throttle);
      }
    }

    // Log the control update
    _logger.logControlUpdate(message, steering, throttle);

    // Send mode status continuously (manual mode)
    sendModeStatus(false);
  } else {
    std::cout << "AUTO mode active - ignoring manual control commands" << std::endl;
  }
}

void ControlAssembly::receiveAutonomousMessages() {
  std::cout << "Autonomous control receiver thread started" << std::endl;
  while (!stop_flag) {
    std::string message = _autonomousSubscriber->receive();

    if (!message.empty()) {
      std::cout << "Received autonomous control message: " << message << std::endl;
      handleAutonomousMessage(message);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Reduced from 50ms to 10ms
  }
  std::cout << "Autonomous control receiver thread stopping" << std::endl;
}

void ControlAssembly::handleAutonomousMessage(const std::string &message) {
  // Only process autonomous commands if AUTO mode is active
  bool current_auto_mode = auto_mode_active.load();
  if (!current_auto_mode) {
    std::cout << "MANUAL MODE ACTIVE - Ignoring autonomous control command: " << message << std::endl;
    return;
  }

  std::cout << "AUTO MODE ACTIVE - Processing autonomous control command: " << message << std::endl;

  std::unordered_map<std::string, double> values;
  std::stringstream ss(message);
  std::string token;
  std::cout << "Parsing autonomous message: " << message << std::endl;

  while (std::getline(ss, token, ';')) {
    if (token.empty())
      continue;

    std::string key;
    double value;
    std::stringstream ss_token(token);
    std::getline(ss_token, key, ':');
    ss_token >> value;
    values[key] = value;
    std::cout << "Parsed autonomous key: '" << key << "', value: " << value << std::endl;
  }

  double steering = 0.0;
  double throttle = 0.0;

  // Apply autonomous steering
  if (values.find("steering") != values.end()) {
    steering = values["steering"];
    std::cout << "Setting autonomous steering to: " << steering << std::endl;
    _fServo->set_steering(static_cast<int>(steering));
  }

  // Apply autonomous throttle, but only if emergency brake is NOT active
  if (values.find("throttle") != values.end()) {
    throttle = values["throttle"];

    if (emergency_brake_active.load()) {
      std::cout << "Emergency brake active - applying intelligent braking instead of autonomous throttle: " << throttle << std::endl;
      performEmergencyBraking(); // Use intelligent braking instead of just setting to 0
      throttle = 0; // Log as 0 for consistency
    } else {
      std::cout << "Setting autonomous throttle to: " << throttle << std::endl;
      _backMotors->setSpeed(throttle);
    }
  }

  // Log the autonomous control update
  _logger.logControlUpdate("AUTO:" + message, steering, throttle);

  // Send mode status continuously (auto mode)
  sendModeStatus(true);
}

void ControlAssembly::sendModeStatus(bool auto_mode_active) {
  if (_clusterPublisher) {
    std::string mode_message = "mode:" + std::to_string(auto_mode_active ? 1 : 0) + ";";

    // Only print status occasionally to avoid spam
    static int mode_counter = 0;
    if (mode_counter++ % 100 == 0) {
      std::cout << "Sending mode status to cluster: " << mode_message << std::endl;
    }

    _clusterPublisher->send(mode_message);
  } else {
    static int warning_counter = 0;
    if (warning_counter++ % 1000 == 0) {
      std::cout << "Warning: Cluster publisher not available, cannot send mode status" << std::endl;
    }
  }
}

void ControlAssembly::receiveEmergencyBrakeMessages() {
  std::cout << "Emergency brake receiver thread started" << std::endl;
  while (!stop_flag) {
    std::string message = _emergencyBrakeSubscriber->receive();

    if (!message.empty()) {
      // Minimal logging for emergency brake - speed is critical
      handleEmergencyBrakeMessage(message);
    }

    // Continuous intelligent braking if emergency brake is active
    if (emergency_brake_active.load()) {
      performEmergencyBraking();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Ultra-fast polling for emergency brake
  }
  std::cout << "Emergency brake receiver thread stopping" << std::endl;
}

void ControlAssembly::handleEmergencyBrakeMessage(const std::string &message) {
  std::unordered_map<std::string, double> values;
  std::stringstream ss(message);
  std::string token;

  while (std::getline(ss, token, ';')) {
    if (token.empty())
      continue;

    std::string key;
    double value;
    std::stringstream ss_token(token);
    std::getline(ss_token, key, ':');
    ss_token >> value;
    values[key] = value;
  }

  // Handle emergency brake commands with highest priority
  if (values.find("emergency_brake") != values.end()) {
    bool emergency_brake = (values["emergency_brake"] != 0.0);
    bool was_active = emergency_brake_active.exchange(emergency_brake);

    if (was_active != emergency_brake) {
      if (emergency_brake) {
        std::cout << "EMERGENCY BRAKE ACTIVATED - Intelligent braking engaged!" << std::endl;
        performEmergencyBraking(); // Use intelligent braking
        _logger.logControlUpdate("emergency_brake_activated", 0, 0);
      } else {
        std::cout << "Emergency brake deactivated - Normal control resumed" << std::endl;
        is_braking.store(false);
        _backMotors->setSpeed(0); // Ensure we stop when deactivating
        _logger.logControlUpdate("emergency_brake_deactivated", 0, 0);
      }
    }
  }
}

void ControlAssembly::performEmergencyBraking() {
  // Get current speed for intelligent braking
  uint32_t current_speed_mms = 0; // Speed in mm/s
  if (speed_data_accessor) {
    auto speed_data = speed_data_accessor();
    if (speed_data) {
      current_speed_mms = speed_data->value.load();
    }
  }

  // Convert mm/s to a rough equivalent for motor control
  // Assuming motor speed range is roughly -100 to +100
  // and current speed indicates forward motion
  if (current_speed_mms > 10) { // Vehicle is moving forward (>10 mm/s ~= 0.036 km/h)
    // Apply strong reverse braking until stopped
    std::cout << "EMERGENCY BRAKING: Applying reverse force (-100) to stop vehicle (current speed: "
              << current_speed_mms << " mm/s)" << std::endl;
    _backMotors->setSpeed(-100);
    is_braking.store(true);
  } else {
    // Vehicle is stopped or moving very slowly, just set to 0
    std::cout << "EMERGENCY BRAKING: Vehicle stopped, setting speed to 0 (current speed: "
              << current_speed_mms << " mm/s)" << std::endl;
    _backMotors->setSpeed(0);
    is_braking.store(false);
  }
}
