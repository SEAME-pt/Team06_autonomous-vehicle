#ifndef ZMQ_PUBLISHER_HPP
#define ZMQ_PUBLISHER_HPP

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <zmq.hpp>

// Interface for publisher functionality
class IPublisher {
public:
  virtual ~IPublisher() = default;
  virtual void send(const std::string &message) = 0;
};

class ZmqPublisher : public IPublisher {
public:
  // Constructor with test_mode parameter
  ZmqPublisher(const std::string &address, zmq::context_t &context,
               bool test_mode = false);
  ~ZmqPublisher() override;

  // Send a message through the publisher
  void send(const std::string &message) override;

  // Test if the publisher is connected
  bool isConnected() const;

private:
  zmq::context_t &_context;
  zmq::socket_t _socket;
  std::string _address;
  bool _test_mode;
  bool _is_connected;
};

#endif
