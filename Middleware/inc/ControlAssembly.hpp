#ifndef CONTROLASSEMBLY_HPP
#define CONTROLASSEMBLY_HPP

#include "BackMotors.hpp"
#include "ControlLogger.hpp"
#include "FServo.hpp"
#include "ZmqSubscriber.hpp"
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

class ControlAssembly {
public:
  ControlAssembly(const std::string &address, zmq::context_t &context,
                  std::shared_ptr<IBackMotors> backMotors = nullptr,
                  std::shared_ptr<IFServo> fServo = nullptr);
  ~ControlAssembly();

  void start();
  void stop();

  ZmqSubscriber zmq_subscriber;

private:
  void receiveMessages();
  void handleMessage(const std::string &message);

  std::thread _listenerThread;
  std::atomic<bool> stop_flag;
  std::atomic<bool> emergency_brake_active;

  std::shared_ptr<IBackMotors> _backMotors;
  std::shared_ptr<IFServo> _fServo;
  ControlLogger _logger;
};

#endif
