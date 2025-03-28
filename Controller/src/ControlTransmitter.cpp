#include "ControlTransmitter.hpp"

ControlTransmitter::ControlTransmitter(const std::string& zmq_address, zmq::context_t& zmq_context)
    : _zmq_publisher(zmq_address, zmq_context) {
    _zmq_publisher.send("init;");
}

ControlTransmitter::~ControlTransmitter() {
    _zmq_publisher.send("throttle:0;steering:0;");
}

void ControlTransmitter::startTransmitting() {
    while (true) {
        _onClick = false;
        if (!_controller.readEvent()) {
            break;
        }
        if (_controller.getButton(X_BUTTON)) {
            _onClick = true;
            _zmq_publisher.send("throttle:0;steering:0;");
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
        _acceleration *= 0.99; // Retorno proporcional para a velocidade do carro (desaceleração)
        if (std::abs(_turn) < 0.1) {
            _turn = 0;
        } else {
            _turn -= _turn *0.15;
        }

		float force = _controller.getAxis(3); // eixo Y do analógico direito
		if (force != 0){
			_acceleration -= (force * 0.55f); // Aceleração proporcional ao valor de force
		}
        _zmq_publisher.send("throttle:" + std::to_string(_acceleration) + ";");

		float gear = _controller.getAxis(0); // eixo horizontal (X) do analógico esquerdo.
		if (std::abs(gear) > 0.1f) { // Zona morta
		    // Maior sensibilidade com ajuste exponencial
		    _turn = (gear > 0 ? 1 : -1) * std::pow(std::abs(gear), 1.5f) * 5.0f;
		    if (_turn < -4.5f) _turn = -4.5f;
		    if (_turn > 4.5f) _turn = 4.5f;
		}
		else {
		    _turn = 0;
		}

		int steeringAngle = static_cast<int>(_turn * 30);
		steeringAngle = std::max(-45, std::min(steeringAngle, 45)); // Limite entre -45 e 45 graus
		_zmq_publisher.send("steering:" + std::to_string(steeringAngle) + ";");
    }
    _zmq_publisher.send("throttle:0;steering:0;");
    return;
}

void ControlTransmitter::zmqPublish(std::string message) {
    _zmq_publisher.send(message);
}
