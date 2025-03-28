#ifndef ZMQ_PUBLISHER_HPP
#define ZMQ_PUBLISHER_HPP

#include <zmq.hpp>
#include <string>
#include <iostream>

class ZmqPublisher {
    public:
        ZmqPublisher(const std::string& address, zmq::context_t& context);
        ~ZmqPublisher();

        void send(const std::string& message);

    private:
        zmq::context_t& _context;
        zmq::socket_t _socket;
        std::string _address;
};

#endif

