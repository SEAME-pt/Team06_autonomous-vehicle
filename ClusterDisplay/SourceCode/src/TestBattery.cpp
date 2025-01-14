#include "TestBattery.hpp"

# include <chrono>
# include <thread>
# include <iostream>
# include <QDebug>

TestBattery::TestBattery(): m_percentage(100.0f)
{
    //qDebug("TestBattery constructor called");
    m_timer =  std::thread([this]() {
              while (getPercentage() > 0.0f)
                  {
                  std::this_thread::sleep_for(std::chrono::seconds(1));
                  _updatePercentage();
              }
              std::cout << "Done from thread." << std::endl;
    });
}
TestBattery::~TestBattery()
{
    if (m_timer.joinable())
        m_timer.join();
}

float   TestBattery::getPercentage()
{
    std::lock_guard<std::mutex>   guard(percentageMutex);
    return m_percentage;
}

void    TestBattery::_updatePercentage(void)
{
    std::lock_guard<std::mutex>   guard(percentageMutex);
    m_percentage -= 1;
}
