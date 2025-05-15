#include "ZmqSubscriber.hpp"

ZmqSubscriber::ZmqSubscriber(const std::string& address, zmq::context_t& context) : _context(context), _socket(context, zmq::socket_type::sub), _address(address) {
    // Set HWM to 1 to only keep latest message
    int hwm = 1;
    _socket.set(zmq::sockopt::rcvhwm, hwm);

    // Enable conflate option to only keep most recent message
    int conflate = 1;
    _socket.set(zmq::sockopt::conflate, conflate);

    // Set zero linger period for clean exits
    int linger = 0;
    _socket.set(zmq::sockopt::linger, linger);

    // Disable Nagle's algorithm for TCP connections
    int tcp_nodelay = 1;
    _socket.set(zmq::sockopt::ipv6, tcp_nodelay);  // This option also disables Nagle's algorithm

    _socket.connect(address);
    _socket.set(zmq::sockopt::subscribe, ""); // Subscribe to all messages
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
