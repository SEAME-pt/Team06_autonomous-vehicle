#ifndef IPUBLISHER_HPP
#define IPUBLISHER_HPP

#include <string>
#include <zmq.hpp>

class IPublisher {
public:
    virtual ~IPublisher() = default;
    virtual void send(const std::string& message) = 0;
};

// Concrete implementation using ZeroMQ
class ZmqPublisher : public IPublisher {
public:
    ZmqPublisher(const std::string& address, zmq::context_t& context)
        : socket(context, zmq::socket_type::pub) {
        socket.bind(address);
    }

    ~ZmqPublisher() override = default;

    void send(const std::string& message) override {
        zmq::message_t zmq_message(message.data(), message.size());
        socket.send(zmq_message, zmq::send_flags::none);
    }

private:
    zmq::socket_t socket;
};

#endif
