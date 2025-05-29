#ifndef CONTROLTRANSMITTER_HPP
#define CONTROLTRANSMITTER_HPP

#include "Controller.hpp"
#include "ZmqPublisher.hpp"
#include <cmath>
#include <iostream>
#include <string>
#include <zmq.hpp>

class ControlTransmitter {
public:
  ControlTransmitter(const std::string &zmq_address,
                     zmq::context_t &zmq_context);
  ~ControlTransmitter();

  // Initialize controller and return whether it was successful
  bool initController();

  void startTransmitting();

private:
  void zmqPublish(std::string message);

  ZmqPublisher _zmq_publisher;
  Controller _controller;
  double _acceleration = 0;
  double _turn = 0;
  bool _onClick = false;
};

#endif
