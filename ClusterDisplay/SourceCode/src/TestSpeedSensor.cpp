#include "TestSpeedSensor.hpp"

# include <chrono>
# include <thread>
# include <iostream>
# include <QDebug>

TestSpeedSensor::TestSpeedSensor(): m_speed(0.0)
{
    //qDebug("TestSpeedSensor constructor called");
    m_timer =  std::thread([this]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        while (getSpeed() < 18.0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            _increaseSpeed(1.5);
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
        while (getSpeed() > 0.0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            _increaseSpeed(-0.7);
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
}

