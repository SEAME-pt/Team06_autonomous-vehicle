#include "ZmqPublisher.hpp"

ZmqPublisher::ZmqPublisher(const std::string& address, zmq::context_t& context)
    : _context(context),
    _socket(context, zmq::socket_type::pub),
    _address(address) {

    // Set HWM to 1 to only keep latest message
    int hwm = 1;
    _socket.set(zmq::sockopt::sndhwm, hwm);

    // Enable conflate option to only keep most recent message
    int conflate = 1;
    _socket.set(zmq::sockopt::conflate, conflate);

    // Set zero linger period for clean exits
    int linger = 0;
    _socket.set(zmq::sockopt::linger, linger);

    // Disable Nagle's algorithm for TCP connections
    int tcp_nodelay = 1;
    _socket.set(zmq::sockopt::ipv6, tcp_nodelay);  // This option also disables Nagle's algorithm

    _socket.bind(address);
}

ZmqPublisher::~ZmqPublisher() {
    _socket.unbind(_address);
}

void ZmqPublisher::send(const std::string& message) {
    std::cerr << "PUBLISHING to " << _address << ": " << message << std::endl;
    zmq::message_t msg(message.size());
    memcpy(msg.data(), message.c_str(), message.size());
    _socket.send(msg, zmq::send_flags::none);
}
