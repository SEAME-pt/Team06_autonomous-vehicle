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
      _zmq_publisher.send("throttle:0;steering:0;");
    }
    if (_controller.getButton(Y_BUTTON)) {
      static bool y_button_prev_state = false;
      bool y_button_current_state = _controller.getButton(Y_BUTTON);

      // Toggle auto mode on Y button press (rising edge)
      if (y_button_current_state && !y_button_prev_state) {
        _auto_mode = !_auto_mode;
        std::cout << "Y button pressed, toggling AUTO mode: " << (_auto_mode ? "ON" : "OFF") << std::endl;
        std::string auto_mode_msg = "auto_mode:" + std::to_string(_auto_mode ? 1 : 0) + ";";
        _zmq_publisher.send(auto_mode_msg);
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
    _acceleration *=
        0.99; // Retorno proporcional para a velocidade do carro (desaceleração)
    if (std::abs(_turn) < 0.1) {
      _turn = 0;
    } else {
      _turn -= _turn * 0.15;
    }

    float force = _controller.getAxis(3); // eixo Y do analógico direito
    if (force != 0) {
      _acceleration -=
          (force * 0.55f); // Aceleração proporcional ao valor de force
    }
    std::string throttleMsg = "throttle:" + std::to_string(_acceleration) + ";";
    // _zmq_publisher.send(throttleMsg);

    float gear =
        _controller.getAxis(0);  // eixo horizontal (X) do analógico esquerdo.
    if (std::abs(gear) > 0.1f) { // Zona morta
      // Maior sensibilidade com ajuste exponencial
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

    // Only send manual control commands when AUTO mode is disabled
    if (!_auto_mode) {
      std::string combinedMsg = throttleMsg + steeringMsg;
      _zmq_publisher.send(combinedMsg);
    } else {
      // Print periodic status when in AUTO mode
      static int auto_mode_counter = 0;
      if (auto_mode_counter++ % 100 == 0) {
        std::cout << "AUTO MODE ACTIVE - Manual controls disabled" << std::endl;
      }
    }
  }
  std::cout << "Exiting transmission loop, sending zero values" << std::endl;
  _zmq_publisher.send("throttle:0;steering:0;");
  return;
}

void ControlTransmitter::zmqPublish(std::string message) {
  std::cout << "Publishing message: " << message << std::endl;
  _zmq_publisher.send(message);
}
