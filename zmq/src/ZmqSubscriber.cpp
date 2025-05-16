#include "ZmqSubscriber.hpp"
#include <chrono>

ZmqSubscriber::ZmqSubscriber(const std::string& address, zmq::context_t& context, bool test_mode)
    : _context(context),
    _socket(context, zmq::socket_type::sub),
    _address(address),
    _test_mode(test_mode),
    _is_connected(false) {

    if (!test_mode) {
        try {
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
            _is_connected = true;
        } catch (const zmq::error_t& e) {
            std::cerr << "ZMQ Error initializing subscriber: " << e.what() << std::endl;
            _is_connected = false;
        }
    }
}

ZmqSubscriber::~ZmqSubscriber() {
    if (_is_connected && !_test_mode) {
        try {
            _socket.disconnect(_address);
        } catch (const std::exception& e) {
            std::cerr << "Error during ZmqSubscriber shutdown: " << e.what() << std::endl;
        }
    }
}

std::string ZmqSubscriber::receive(int timeout_ms) {
    // In test mode, return the test message if available
    if (_test_mode) {
        if (_has_test_message) {
            _has_test_message = false;
            std::cerr << "TEST MODE - RECEIVED from " << _address << ": " << _test_message << std::endl;
            return _test_message;
        }
        return "";
    }

    // If not connected, cannot receive
    if (!_is_connected) {
        std::cerr << "Error: Cannot receive message - subscriber not connected" << std::endl;
        return "";
    }

    try {
        zmq::message_t msg;
        zmq::recv_flags flags = zmq::recv_flags::dontwait;

        // If timeout is specified, try to receive with timeout
        if (timeout_ms > 0) {
            // Poll for specified timeout
            zmq::pollitem_t items[] = {
                { static_cast<void*>(_socket), 0, ZMQ_POLLIN, 0 }
            };

            // Use std::chrono::milliseconds instead of a long integer
            zmq::poll(&items[0], 1, std::chrono::milliseconds(timeout_ms));

            if (!(items[0].revents & ZMQ_POLLIN)) {
                return ""; // No message available after timeout
            }
        }

        zmq::recv_result_t result = _socket.recv(msg, flags);

        if (!result) {
            return "";  // No message available, return immediately
        }

        std::string message(static_cast<char*>(msg.data()), msg.size());

        // Check for special empty message marker
        if (message == "<EMPTY_MESSAGE>") {
            std::cerr << "RECEIVED from " << _address << ": <empty message>" << std::endl;
            return "";
        }

        std::cerr << "RECEIVED from " << _address << ": " << message << std::endl;
        return message;
    } catch (const zmq::error_t& e) {
        std::cerr << "ZMQ Error receiving message: " << e.what() << std::endl;
        return "";
    } catch (const std::exception& e) {
        std::cerr << "Error receiving message: " << e.what() << std::endl;
        return "";
    }
}

bool ZmqSubscriber::isConnected() const {
    return _is_connected || _test_mode; // In test mode, pretend we're connected
}

void ZmqSubscriber::setTestMessage(const std::string& message) {
    if (_test_mode) {
        _test_message = message;
        _has_test_message = true;
    }
}
