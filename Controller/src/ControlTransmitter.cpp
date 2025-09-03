#include "ControlTransmitter.hpp"

ControlTransmitter::ControlTransmitter(const std::string &zmq_address,
                                       zmq::context_t &zmq_context)
    : _zmq_publisher(zmq_address, zmq_context) {
  std::cout << "ControlTransmitter initialized with address: " << zmq_address
            << std::endl;
  _zmq_publisher.send("init;");
  std::cout << "Sent init message" << std::endl;
}

ControlTransmitter::~ControlTransmitter() {
  std::cout << "ControlTransmitter shutting down, sending zero values"
            << std::endl;
  _zmq_publisher.send("throttle:0;steering:0;");
}

bool ControlTransmitter::initController() {
  std::cout << "Initializing controller..." << std::endl;
  _controller.openDevice();
  bool connected = _controller.isConnected();
  std::cout << "Controller " << (connected ? "connected" : "failed to connect")
            << std::endl;
  return connected;
}

void ControlTransmitter::startTransmitting() {
  std::cout << "Starting transmission loop..." << std::endl;
  while (true) {
    _onClick = false;
    if (!_controller.readEvent()) {
      std::cout << "Controller read event failed, exiting loop" << std::endl;
      break;
    }
    if (_controller.getButton(X_BUTTON)) {
      _onClick = true;
      std::cout << "X button pressed, sending stop command" << std::endl;
      _acceleration = 0; // Immediate stop
      _zmq_publisher.send("throttle:0;steering:0;");
    }

    // Handle Y button toggle for auto mode (always works regardless of auto
    // mode state)
    bool send_auto_mode = false;
    {
      static bool y_button_prev_state = false;
      bool y_button_current_state = _controller.getButton(Y_BUTTON);

      // Toggle auto mode on Y button press (rising edge)
      if (y_button_current_state && !y_button_prev_state) {
        _auto_mode = !_auto_mode;
        std::cout << "Y button pressed, toggling AUTO mode: "
                  << (_auto_mode ? "ON" : "OFF") << std::endl;
      }

      // Send auto mode status while Y button is pressed
      if (y_button_current_state) {
        send_auto_mode = true;
      }

      y_button_prev_state = y_button_current_state;
    }
    if (_controller.getButton(SELECT_BUTTON)) {
      std::cout << "Select button pressed" << std::endl;
    }
    if (_controller.getButton(START_BUTTON)) {
      std::cout << "Start button pressed" << std::endl;
    }
    if (_controller.getButton(HOME_BUTTON)) {
      std::cout << "Home button pressed" << std::endl;
    }

    // Apply much more gradual deceleration when no input
    _acceleration *= 0.98; // Changed from 0.95 to 0.98 for much more gradual
                           // natural deceleration

    if (std::abs(_turn) < 0.1) {
      _turn = 0;
    } else {
      _turn -=
          _turn * 0.25; // Changed from 0.15 to 0.25 for faster steering return
    }

    float force = _controller.getAxis(3); // eixo Y do analógico direito

    // Process normal throttle input
    if (force != 0) {
      // Direct scaling: joystick input (-1.0 to 1.0) -> throttle (-100 to +100)
      // With acceleration factor for responsiveness
      // INVERTED: Negate force to fix control direction
      float target_throttle =
          -force *
          100.0f; // Restored to full range for maximum power when needed

      std::cout << "Joystick force: " << force
                << ", Target throttle: " << target_throttle
                << ", Current: " << _acceleration << std::endl;

      // Move towards target throttle much more gradually
      float throttle_diff = target_throttle - _acceleration;
      float max_change =
          (throttle_diff > 0)
              ? _max_acceleration_rate * 2.0f
              : _max_deceleration_rate * 2.0f; // Reduced from 5.0f to 2.0f

      if (std::abs(throttle_diff) <= max_change) {
        // Close to target, snap to it
        _acceleration = target_throttle;
      } else {
        // Move towards target at max rate
        _acceleration += (throttle_diff > 0) ? max_change : -max_change;
      }

      // Clamp to full throttle bounds for maximum power capability
      _acceleration =
          std::max(-100.0f, std::min(_acceleration,
                                     100.0f)); // Restored to full ±100 range

      std::cout << "Final throttle: " << _acceleration << std::endl;
    }

    std::string throttleMsg = "throttle:" + std::to_string(_acceleration) + ";";
    // _zmq_publisher.send(throttleMsg);

    float gear =
        _controller.getAxis(0);  // eixo horizontal (X) do analógico esquerdo.
    if (std::abs(gear) > 0.1f) { // Zona morta
      // Maior sensibilidade com ajuste exponencial
      // FIXED: Removed inversion - original direction was correct
      _turn = (gear > 0 ? 1 : -1) * std::pow(std::abs(gear), 1.5f) * 5.0f;
      if (_turn < -4.5f)
        _turn = -4.5f;
      if (_turn > 4.5f)
        _turn = 4.5f;
    } else {
      _turn = 0;
    }

    int steeringAngle = static_cast<int>(_turn * 30);
    steeringAngle = std::max(
        -45, std::min(steeringAngle, 45)); // Limite entre -45 e 45 graus
    std::string steeringMsg = "steering:" + std::to_string(steeringAngle) + ";";

    // Send manual control commands (middleware handles auto mode protection)
    std::string combinedMsg = throttleMsg + steeringMsg;

    // Include auto mode status while Y button is pressed
    if (send_auto_mode) {
      combinedMsg += "auto_mode:" + std::to_string(_auto_mode ? 1 : 0) + ";";
    }

    _zmq_publisher.send(combinedMsg);
  }
  std::cout << "Exiting transmission loop, sending zero values" << std::endl;
  _zmq_publisher.send("throttle:0;steering:0;");
  return;
}

void ControlTransmitter::zmqPublish(std::string message) {
  std::cout << "Publishing message: " << message << std::endl;
  _zmq_publisher.send(message);
}
