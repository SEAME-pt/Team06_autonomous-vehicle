#ifndef ZMQ_SUBSCRIBER_HPP
#define ZMQ_SUBSCRIBER_HPP

#include "zmq.hpp"
#include <string>
#include <iostream>

class ZmqSubscriber {
public:
    ZmqSubscriber(const std::string& address, zmq::context_t& context);
    ~ZmqSubscriber();
    std::string receive();

private:
    zmq::context_t& _context;
    zmq::socket_t _socket;
    std::string _address;
};


#endif
