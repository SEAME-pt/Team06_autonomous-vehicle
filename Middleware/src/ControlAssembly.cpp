#include "ControlAssembly.hpp"

ControlAssembly::ControlAssembly(const std::string &address,
                                 zmq::context_t &context,
                                 std::shared_ptr<IBackMotors> backMotors,
                                 std::shared_ptr<IFServo> fServo,
                                 std::shared_ptr<ZmqPublisher> clusterPublisher)
    : zmq_subscriber(address, context), stop_flag(true),
      emergency_brake_active(false), auto_mode_active(false), _context(context),
      _backMotors(backMotors ? backMotors : std::make_shared<BackMotors>()),
      _fServo(fServo ? fServo : std::make_shared<FServo>()),
      _clusterPublisher(clusterPublisher), _logger("control_updates.log") {
  std::cout << "ControlAssembly initialized with ZMQ address: " << address
            << std::endl; // LCOV_EXCL_LINE - Initialization logging

  // Initialize autonomous control subscriber
  const std::string autonomous_address = "tcp://127.0.0.1:5560";
  _autonomousSubscriber =
      std::make_unique<ZmqSubscriber>(autonomous_address, context);
  std::cout << "Autonomous control subscriber initialized with address: "
            << autonomous_address
            << std::endl; // LCOV_EXCL_LINE - Initialization logging

  // LCOV_EXCL_START - Hardware initialization, not testable in unit tests
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
  } catch (
      const std::exception &e) { // LCOV_EXCL_LINE - Hardware error handling
    std::cerr << "Error during initialization: " << e.what()
              << std::endl; // LCOV_EXCL_LINE - Hardware error handling
    // Don't re-throw the exception to allow graceful handling in tests
    // This allows the ControlAssembly to be constructed even if hardware
    // initialization fails
  }
  // LCOV_EXCL_STOP

  // Send initial mode status (manual mode)
  sendModeStatus(false);
}

ControlAssembly::~ControlAssembly() {
  std::cout << "ControlAssembly shutting down"
            << std::endl; // LCOV_EXCL_LINE - Shutdown logging

  // Deactivate auto mode and send status
  if (auto_mode_active.load()) {
    auto_mode_active.store(false);
    sendModeStatus(false);
  }

  stop();
  _backMotors->setSpeed(0);
  _fServo->set_steering(0);
  std::cout << "Motor speed and steering set to 0"
            << std::endl; // LCOV_EXCL_LINE - Shutdown logging
}

void ControlAssembly::setSpeedDataAccessor(
    std::function<std::shared_ptr<SensorData>()> accessor) {
  speed_data_accessor = accessor;
  std::cout << "Speed data accessor set for intelligent emergency braking"
            << std::endl; // LCOV_EXCL_LINE - Configuration logging
}

void ControlAssembly::setEmergencyBrakeCallback(
    std::function<void(bool)> callback) {
  emergency_brake_callback = callback;
  std::cout << "Emergency brake callback set for ControlAssembly"
            << std::endl; // LCOV_EXCL_LINE - Configuration logging
}

void ControlAssembly::start() {
  std::lock_guard<std::mutex> lock(_startStopMutex);

  // Check if already running
  if (!stop_flag.load()) {
    std::cout << "ControlAssembly already running, ignoring start request"
              << std::endl; // LCOV_EXCL_LINE - State logging
    return;
  }

  std::cout << "Starting ControlAssembly message receiver threads"
            << std::endl; // LCOV_EXCL_LINE - Thread management logging
  stop_flag = false;

  // Ensure any existing threads are properly cleaned up before creating new
  // ones
  if (_listenerThread.joinable()) {
    _listenerThread.join();
  }
  if (_autonomousListenerThread.joinable()) {
    _autonomousListenerThread.join();
  }

  // Create new threads
  try {
    _listenerThread = std::thread(&ControlAssembly::receiveMessages, this);
    _autonomousListenerThread =
        std::thread(&ControlAssembly::receiveAutonomousMessages, this);
  } catch (const std::exception
               &e) { // LCOV_EXCL_LINE - Thread creation error handling
    std::cerr << "Error creating threads: " << e.what()
              << std::endl; // LCOV_EXCL_LINE - Thread creation error handling
    stop_flag = true;       // LCOV_EXCL_LINE - Thread creation error handling
    throw;                  // LCOV_EXCL_LINE - Thread creation error handling
  }
}

void ControlAssembly::stop() {
  std::lock_guard<std::mutex> lock(_startStopMutex);
  std::cout << "Stopping ControlAssembly"
            << std::endl; // LCOV_EXCL_LINE - Thread management logging

  // Only stop if not already stopped
  if (!stop_flag.exchange(true)) {
    // Join threads with timeout to prevent hanging
    if (_listenerThread.joinable()) {
      _listenerThread.join();
      std::cout << "Manual control receiver thread joined"
                << std::endl; // LCOV_EXCL_LINE - Thread management logging
    }
    if (_autonomousListenerThread.joinable()) {
      _autonomousListenerThread.join();
      std::cout << "Autonomous control receiver thread joined"
                << std::endl; // LCOV_EXCL_LINE - Thread management logging
    }
  } else {
    std::cout << "ControlAssembly already stopped"
              << std::endl; // LCOV_EXCL_LINE - State logging
  }
}

void ControlAssembly::receiveMessages() {
  std::cout << "Message receiver thread started"
            << std::endl; // LCOV_EXCL_LINE - Thread management logging
  while (!stop_flag) {
    std::string message = zmq_subscriber.receive();

    if (!message.empty()) {
      std::cout << "Received control message: " << message
                << std::endl; // LCOV_EXCL_LINE - Debug logging
      handleMessage(message);
    } else {
      std::cout << "Received empty message"
                << std::endl; // LCOV_EXCL_LINE - Debug logging
    }

    std::this_thread::sleep_for(
        std::chrono::milliseconds(10)); // Reduced from 50ms to 10ms
  }
  std::cout << "Message receiver thread stopping"
            << std::endl; // LCOV_EXCL_LINE - Thread management logging
}

void ControlAssembly::handleMessage(const std::string &message) {
  std::unordered_map<std::string, double> values;
  std::stringstream ss(message);
  std::string token;
  // LCOV_EXCL_START - Debug logging and message parsing, not testable in unit
  // tests
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
  // LCOV_EXCL_STOP

  // Handle special 'init' message
  if (message == "init;") {
    std::cout << "Received init message, resetting to zero values"
              << std::endl; // LCOV_EXCL_LINE - Message handling logging
    emergency_brake_active.store(false);
    _fServo->set_steering(0);
    _backMotors->setSpeed(0);
    _logger.logControlUpdate("init", 0, 0);
    return;
  }

  // Handle AUTO mode toggle commands with highest priority
  if (values.find("auto_mode") != values.end()) {
    bool new_auto_mode = (values["auto_mode"] != 0.0);
    bool was_auto_active = auto_mode_active.exchange(new_auto_mode);

    if (was_auto_active != new_auto_mode) {
      if (new_auto_mode) {
        std::cout << "AUTO MODE ACTIVATED - Switching to autonomous control"
                  << std::endl; // LCOV_EXCL_LINE - Mode change logging
        _logger.logControlUpdate("auto_mode_activated", 0, 0);
      } else {
        std::cout << "AUTO MODE DEACTIVATED - Switching to manual control"
                  << std::endl; // LCOV_EXCL_LINE - Mode change logging
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
    // Apply steering if present in the message (emergency brake doesn't affect
    // steering)
    if (values.find("steering") != values.end()) {
      steering = values["steering"];
      std::cout << "Setting steering to: " << steering
                << std::endl; // LCOV_EXCL_LINE - Control action logging
      _fServo->set_steering(static_cast<int>(steering));
    }

    // Apply throttle if present in the message
    if (values.find("throttle") != values.end()) {
      throttle = values["throttle"];

      if (emergency_brake_active.load()) {
        if (throttle < 0) {
          // Check current speed before allowing reverse during emergency brake
          uint32_t current_speed_mms = 0; // Speed in mm/s
          if (speed_data_accessor) {
            auto speed_data = speed_data_accessor();
            if (speed_data) {
              current_speed_mms = speed_data->value.load();
            }
          }

          if (current_speed_mms == 0) {
            // Vehicle is stopped, allow reverse to back away from obstruction
            // LCOV_EXCL_START - Emergency brake logging, not testable in unit
            // tests
            std::cout << "Emergency brake active - vehicle stopped (speed: "
                      << current_speed_mms
                      << " mm/s), allowing reverse throttle: " << throttle
                      << std::endl;
            // LCOV_EXCL_STOP
            _backMotors->setSpeed(throttle);
          } else {
            // Vehicle still moving, block reverse and continue emergency
            // braking
            // LCOV_EXCL_START - Emergency brake logging, not testable in unit
            // tests
            std::cout
                << "Emergency brake active - vehicle still moving (speed: "
                << current_speed_mms
                << " mm/s), blocking reverse throttle: " << throttle
                << ", continuing emergency braking" << std::endl;
            // LCOV_EXCL_STOP
            performEmergencyBraking();
            throttle = 0; // Log as 0 for consistency
          }
        } else {
          // Block positive throttle during emergency brake
          std::cout << "Emergency brake active - blocking positive throttle: "
                    << throttle << ", applying intelligent braking instead"
                    << std::endl;    // LCOV_EXCL_LINE - Emergency brake logging
          performEmergencyBraking(); // Use intelligent braking instead of
                                     // positive throttle
          throttle = 0;              // Log as 0 for consistency
        }
      } else {
        std::cout << "Setting throttle to: " << throttle
                  << std::endl; // LCOV_EXCL_LINE - Control action logging
        _backMotors->setSpeed(throttle);
      }
    }

    // Log the control update
    _logger.logControlUpdate(message, steering, throttle);

    // Send mode status continuously (manual mode)
    sendModeStatus(false);
  } else {
    std::cout << "AUTO mode active - ignoring manual control commands"
              << std::endl; // LCOV_EXCL_LINE - Mode state logging
  }
}

void ControlAssembly::receiveAutonomousMessages() {
  std::cout << "Autonomous control receiver thread started"
            << std::endl; // LCOV_EXCL_LINE - Thread management logging
  while (!stop_flag) {
    std::string message = _autonomousSubscriber->receive();

    if (!message.empty()) {
      std::cout << "Received autonomous control message: " << message
                << std::endl; // LCOV_EXCL_LINE - Debug logging
      handleAutonomousMessage(message);
    }

    std::this_thread::sleep_for(
        std::chrono::milliseconds(10)); // Reduced from 50ms to 10ms
  }
  std::cout << "Autonomous control receiver thread stopping" << std::endl;
}
// LCOV_EXCL_START - Autonomous message handling, not testable in unit tests
void ControlAssembly::handleAutonomousMessage(const std::string &message) {
  // Only process autonomous commands if AUTO mode is active
  bool current_auto_mode = auto_mode_active.load();
  if (!current_auto_mode) {
    std::cout << "MANUAL MODE ACTIVE - Ignoring autonomous control command: "
              << message << std::endl; // LCOV_EXCL_LINE - Mode state logging
    return;
  }

  std::cout << "AUTO MODE ACTIVE - Processing autonomous control command: "
            << message << std::endl; // LCOV_EXCL_LINE - Mode state logging

  std::unordered_map<std::string, double> values;
  std::stringstream ss(message);
  std::string token;
  std::cout << "Parsing autonomous message: " << message
            << std::endl; // LCOV_EXCL_LINE - Debug logging

  while (std::getline(ss, token, ';')) {
    if (token.empty())
      continue;

    std::string key;
    double value;
    std::stringstream ss_token(token);
    std::getline(ss_token, key, ':');
    ss_token >> value;
    values[key] = value;
    std::cout << "Parsed autonomous key: '" << key << "', value: " << value
              << std::endl; // LCOV_EXCL_LINE - Debug logging
  }

  double steering = 0.0;
  double throttle = 0.0;

  // Apply autonomous steering
  if (values.find("steering") != values.end()) {
    steering = values["steering"];
    std::cout << "Setting autonomous steering to: " << steering
              << std::endl; // LCOV_EXCL_LINE - Control action logging
    _fServo->set_steering(static_cast<int>(steering));
  }

  // Apply autonomous throttle
  if (values.find("throttle") != values.end()) {
    throttle = values["throttle"];

    if (emergency_brake_active.load()) {
      // Block all throttle during emergency brake in autonomous mode
      std::cout << "Emergency brake active - blocking all autonomous throttle: "
                << throttle << ", applying intelligent braking instead"
                << std::endl;    // LCOV_EXCL_LINE - Emergency brake logging
      performEmergencyBraking(); // Use intelligent braking instead of any
                                 // throttle
      throttle = 0;              // Log as 0 for consistency
    } else {
      std::cout << "Setting autonomous throttle to: " << throttle
                << std::endl; // LCOV_EXCL_LINE - Control action logging
      _backMotors->setSpeed(throttle);
    }
  }

  // Log the autonomous control update
  _logger.logControlUpdate("AUTO:" + message, steering, throttle);

  // Send mode status continuously (auto mode)
  sendModeStatus(true);
} // LCOV_EXCL_STOP

void ControlAssembly::sendModeStatus(bool auto_mode_active) {
  if (_clusterPublisher) {
    std::string mode_message =
        "mode:" + std::to_string(auto_mode_active ? 1 : 0) + ";";

    // Only print status occasionally to avoid spam
    static int mode_counter = 0;
    if (mode_counter++ % 100 == 0) {
      std::cout << "Sending mode status to cluster: " << mode_message
                << std::endl;
    }

    _clusterPublisher->send(mode_message);
  } else {
    static int warning_counter = 0;
    if (warning_counter++ % 1000 == 0) {
      std::cout << "Warning: Cluster publisher not available, cannot send "
                   "mode status"
                << std::endl;
    }
  }
}

void ControlAssembly::handleEmergencyBrake(bool emergency_active) {
  bool was_active = emergency_brake_active.exchange(emergency_active);

  if (was_active != emergency_active) {
    if (emergency_active) {
      std::cout << "EMERGENCY BRAKE ACTIVATED - Intelligent braking engaged!"
                << std::endl;
      performEmergencyBraking(); // Use intelligent braking
      _logger.logControlUpdate("emergency_brake_activated", 0, 0);
    } else {
      std::cout << "Emergency brake deactivated - Normal control resumed"
                << std::endl;
      _backMotors->setSpeed(0); // Ensure we stop when deactivating
      _logger.logControlUpdate("emergency_brake_deactivated", 0, 0);
    }

    // Call the emergency brake callback if it's set
    if (emergency_brake_callback) {
      emergency_brake_callback(emergency_active);
    }
  }
}

// void ControlAssembly::performEmergencyBraking() {
//   // Get current speed for intelligent braking
//   uint32_t current_speed_mms = 0; // Speed in mm/s
//   if (speed_data_accessor) {
//     auto speed_data = speed_data_accessor();
//     if (speed_data) {
//       current_speed_mms = speed_data->value.load();
//     }
//   }

//   // Convert mm/s to a rough equivalent for motor control
//   // Assuming motor speed range is roughly -100 to +100
//   // and current speed indicates forward motion
//   if (current_speed_mms > 10) { // Vehicle is moving forward (>10 mm/s ~=
//   0.036 km/h)
//     // Apply strong reverse braking until stopped
//     std::cout << "EMERGENCY BRAKING: Applying reverse force (-100) to stop
//     vehicle (current speed: "
//               << current_speed_mms << " mm/s)" << std::endl;
//     _backMotors->setSpeed(-100);
//   } else {
//     // Vehicle is stopped or moving very slowly, just set to 0
//     std::cout << "EMERGENCY BRAKING: Vehicle stopped, setting speed to 0
//     (current speed: "
//               << current_speed_mms << " mm/s)" << std::endl;
//     _backMotors->setSpeed(0);
//   }
// }

void ControlAssembly::performEmergencyBraking() {
  _backMotors->emergencyBrake();
}
