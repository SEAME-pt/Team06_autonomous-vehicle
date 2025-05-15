#include "ZmqPublisher.hpp"

ZmqPublisher::ZmqPublisher(const std::string& address, zmq::context_t& context)
    : _context(context),
    _socket(context, zmq::socket_type::pub),
    _address(address) {
    _socket.bind(address);
}

ZmqPublisher::~ZmqPublisher() {
    _socket.unbind(_address);
}

void ZmqPublisher::send(const std::string& message) {
    // std::cerr << "PUBLISHING to " << _address << ": " << message << std::endl;
    zmq::message_t msg(message.size());
    memcpy(msg.data(), message.c_str(), message.size());
    _socket.send(msg, zmq::send_flags::none);
}
