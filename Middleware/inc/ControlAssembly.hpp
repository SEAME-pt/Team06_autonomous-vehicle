#ifndef CONTROLASSEMBLY_HPP
#define CONTROLASSEMBLY_HPP

#include "BackMotors.hpp"
#include "ControlLogger.hpp"
#include "FServo.hpp"
#include "ZmqSubscriber.hpp"
#include "ZmqPublisher.hpp"
#include "ISensor.hpp" // Add for speed data access
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
#include <functional> // Add for speed data accessor

class ControlAssembly {
public:
  ControlAssembly(const std::string &address, zmq::context_t &context,
                  std::shared_ptr<IBackMotors> backMotors = nullptr,
                  std::shared_ptr<IFServo> fServo = nullptr,
                  std::shared_ptr<ZmqPublisher> clusterPublisher = nullptr);
  ~ControlAssembly();

  void start();
  void stop();

  // Set speed data accessor for intelligent emergency braking
  void setSpeedDataAccessor(std::function<std::shared_ptr<SensorData>()> accessor);

  // Set emergency brake callback for direct communication with Distance sensor
  void setEmergencyBrakeCallback(std::function<void(bool)> callback);

  // Handle emergency brake from callback (public so it can be called from Distance sensor)
  void handleEmergencyBrake(bool emergency_active);

  ZmqSubscriber zmq_subscriber;

private:
  void receiveMessages();
  void handleMessage(const std::string &message);
  void receiveAutonomousMessages();
  void handleAutonomousMessage(const std::string &message);
  void sendModeStatus(bool auto_mode_active);
  void performEmergencyBraking(); // Intelligent emergency braking method

  std::thread _listenerThread;
  std::thread _autonomousListenerThread;
  std::atomic<bool> stop_flag;
  std::atomic<bool> emergency_brake_active;
  std::atomic<bool> auto_mode_active;
  std::atomic<bool> is_braking; // Track if we're actively braking

  // Speed data accessor for intelligent braking
  std::function<std::shared_ptr<SensorData>()> speed_data_accessor;

  // Emergency brake callback for direct communication
  std::function<void(bool)> emergency_brake_callback;

  // ZMQ components
  std::unique_ptr<ZmqSubscriber> _autonomousSubscriber;
  std::shared_ptr<ZmqPublisher> _clusterPublisher;
  zmq::context_t &_context;

  std::shared_ptr<IBackMotors> _backMotors;
  std::shared_ptr<IFServo> _fServo;
  ControlLogger _logger;
};

#endif
