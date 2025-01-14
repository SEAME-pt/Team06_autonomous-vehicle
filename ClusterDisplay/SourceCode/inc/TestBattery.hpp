#ifndef TESTBATTERY_HPP
#define TESTBATTERY_HPP

#include <mutex>
#include <thread>

class TestBattery
{
public:
    TestBattery();
    ~TestBattery();

    float   getPercentage(void);

private:
    float   m_percentage;
    std::mutex  percentageMutex;

    std::thread m_timer;
    void    _updatePercentage(void);
};

#endif // TESTBATTERY_HPP
