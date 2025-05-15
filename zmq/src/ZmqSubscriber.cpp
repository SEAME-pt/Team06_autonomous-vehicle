#include "ZmqSubscriber.hpp"

ZmqSubscriber::ZmqSubscriber(const std::string& address, zmq::context_t& context) : _context(context), _socket(context, zmq::socket_type::sub), _address(address) {
        _socket.connect(address);
        _socket.set(zmq::sockopt::subscribe, ""); // Subscribe to all messages
        _socket.setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
        _socket.setsockopt(ZMQ_RCVHWM, &hwm, sizeof(hwm));
    }

ZmqSubscriber::~ZmqSubscriber() {
        _socket.disconnect(_address);
    }

std::string ZmqSubscriber::receive() {
    zmq::message_t msg;
    zmq::recv_result_t result = _socket.recv(msg, zmq::recv_flags::dontwait);

    if (!result) {
        return "";  // No message available, return immediately
    }

    std::string message(static_cast<char*>(msg.data()), msg.size());
    std::cerr << "RECEIVED from " << _address << ": " << message << std::endl;
    return message;
}
