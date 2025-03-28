#ifndef CONTROLASSEMBLY_HPP
#define CONTROLASSEMBLY_HPP

#include "ZmqSubscriber.hpp"
#include "BackMotors.hpp"
#include "FServo.hpp"
#include <iostream>
#include <chrono>
#include <cstdio>
#include <cstdarg>
#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <unordered_map>
#include <sstream>

class ControlAssembly {
public:
    ControlAssembly(const std::string& address, zmq::context_t& context);
    ~ControlAssembly();

    void start();
    void stop();

    ZmqSubscriber zmq_subscriber;

private:
    void receiveMessages();
    void handleMessage(const std::string& message);

    std::thread _listenerThread;
    std::atomic<bool> stop_flag;

    BackMotors	_backMotors;
	FServo		_fServo;
};

#endif
