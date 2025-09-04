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
  float _acceleration = 0;
  float _turn = 0;
  bool _onClick = false;
  bool _auto_mode = false;
  bool _emergency_brake_active = false; // Flag to track emergency brake state

  // New acceleration control parameters
  float _max_acceleration_rate =
      1.0f; // Reduced from 2.0f for much gentler acceleration
  float _max_deceleration_rate =
      1.5f;                       // Reduced from 3.0f for much gentler braking
  float _brake_multiplier = 5.0f; // Emergency brake force multiplier
};

#endif
