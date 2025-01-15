#include "TestSpeedSensor.hpp"

# include <chrono>
# include <thread>
# include <iostream>
# include <QDebug>
# include <zmq.hpp>

TestSpeedSensor::TestSpeedSensor(): m_speed(0.0)
{
    //qDebug("TestSpeedSensor constructor called");
    m_timer =  std::thread([this]() {
        zmq::context_t context (1);
        zmq::socket_t publisher (context, zmq::socket_type::pub);
        publisher.bind("tcp://*:5555");
        std::this_thread::sleep_for(std::chrono::seconds(2));
        while (getSpeed() < 19.0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            _increaseSpeed(1.5);
            int len = getSpeed() < 10.0 ? 1 : 2;
            zmq::message_t message(len);
            snprintf ((char *) message.data(), len+1 , "%d", static_cast<int>(m_speed));
            publisher.send(message, zmq::send_flags::none);
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
        while (getSpeed() > 0.0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            _increaseSpeed(-0.7);
            int len = getSpeed() < 10.0 ? 1 : 2;
            zmq::message_t message(len);
            snprintf ((char *) message.data(), len+1 , "%d", static_cast<int>(m_speed));
            publisher.send(message, zmq::send_flags::none);
        }
        //std::cout << "Done from Speed thread." << std::endl;
    });
}
TestSpeedSensor::~TestSpeedSensor()
{
    if (m_timer.joinable())
        m_timer.join();
}

double   TestSpeedSensor::getSpeed()
{
    std::lock_guard<std::mutex>   guard(speedMutex);
    return m_speed;
}

void    TestSpeedSensor::_increaseSpeed(double increase)
{
    std::lock_guard<std::mutex>   guard(speedMutex);
    m_speed += increase;
    std::cout << "from sensor: " << m_speed << std::endl;
}

