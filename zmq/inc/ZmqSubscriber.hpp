#ifndef ZMQ_SUBSCRIBER_HPP
#define ZMQ_SUBSCRIBER_HPP

#include "zmq.hpp"
#include <string>
#include <iostream>
#include <memory>
#include <functional>
#include <optional>

// Interface for subscriber functionality
class ISubscriber {
public:
    virtual ~ISubscriber() = default;
    virtual std::string receive(int timeout_ms = 0) = 0;
    virtual bool isConnected() const = 0;
};

class ZmqSubscriber : public ISubscriber {
public:
    // Constructor with test_mode parameter
    ZmqSubscriber(const std::string& address, zmq::context_t& context, bool test_mode = false);
    ~ZmqSubscriber() override;

    // Receive a message with optional timeout in milliseconds
    std::string receive(int timeout_ms = 0) override;

    // Check if the subscriber is connected
    bool isConnected() const override;

    // For test mode: set the next message to be received
    void setTestMessage(const std::string& message);

private:
    zmq::context_t& _context;
    zmq::socket_t _socket;
    std::string _address;
    bool _test_mode;
    bool _is_connected;

    // For test mode
    std::string _test_message;
    bool _has_test_message = false;
};


#endif
