#include "ZmqPublisher.hpp"

ZmqPublisher::ZmqPublisher(const std::string &address, zmq::context_t &context,
                           bool test_mode)
    : _context(context), _socket(context, zmq::socket_type::pub),
      _address(address), _test_mode(test_mode), _is_connected(false) {

  if (!test_mode) {
    try {
      // Set HWM to 1 to only keep latest message
      int hwm = 100;
      _socket.set(zmq::sockopt::sndhwm, hwm);

      // NOTE: Conflate option removed from publisher - it should only be used
      // on subscribers The conflate option on publishers can cause round-robin
      // distribution instead of broadcast

      // Set zero linger period for clean exits
      int linger = 0;
      _socket.set(zmq::sockopt::linger, linger);

      // NOTE: ZMQ_TCP_NODELAY not available in this ZMQ version
      // The immediate option below provides similar low-latency behavior

      // Send messages immediately instead of batching
      _socket.set(zmq::sockopt::immediate, 1);

      _socket.bind(address);
      _is_connected = true;
    } catch (const zmq::error_t &e) {
      std::cerr << "ZMQ Error initializing publisher: " << e.what()
                << std::endl; // LCOV_EXCL_LINE - ZMQ error handling
      _is_connected = false;
    }
  }
}

ZmqPublisher::~ZmqPublisher() {
  if (_is_connected && !_test_mode) {
    try {
      _socket.unbind(_address);
      _socket.close(); // Explicitly close the socket
    } catch (const std::exception &e) {
      std::cerr << "Error during ZmqPublisher shutdown: " << e.what()
                << std::endl; // LCOV_EXCL_LINE - Error handling
    }
  } else if (!_test_mode) {
    // Even if not connected, close the socket if it exists
    try {
      _socket.close();
    } catch (const std::exception &e) {
      // Ignore errors during cleanup
    }
  }
}

void ZmqPublisher::send(const std::string &message) {
  // In test mode, just log the message
  if (_test_mode) {
    std::cerr << "TEST MODE - PUBLISHING to " << _address << ": " << message
              << std::endl; // LCOV_EXCL_LINE - Test mode logging
    return;
  }

  // If not connected, try to log the issue but don't crash
  if (!_is_connected) {
    std::cerr << "Error: Cannot send message - publisher not connected"
              << std::endl; // LCOV_EXCL_LINE - Error handling
    return;
  }

  try {
    std::cerr << "PUBLISHING to " << _address << ": " << message
              << std::endl; // LCOV_EXCL_LINE - Debug logging

    // Handle empty messages by sending a special marker
    const std::string &msgToSend =
        message.empty() ? "<EMPTY_MESSAGE>" : message;

    zmq::message_t msg(msgToSend.size());
    memcpy(msg.data(), msgToSend.c_str(), msgToSend.size());
    _socket.send(msg, zmq::send_flags::none);
  } catch (const zmq::error_t &e) {
    std::cerr << "ZMQ Error sending message: " << e.what()
              << std::endl; // LCOV_EXCL_LINE - ZMQ error handling
  } catch (const std::exception &e) {
    std::cerr << "Error sending message: " << e.what()
              << std::endl; // LCOV_EXCL_LINE - Error handling
  }
}

bool ZmqPublisher::isConnected() const {
  return _is_connected || _test_mode; // In test mode, pretend we're connected
}
