#ifndef TESTSPEEDSENSOR_H
#define TESTSPEEDSENSOR_H

#include <mutex>
#include <thread>

class TestSpeedSensor
{
public:
    TestSpeedSensor();
    ~TestSpeedSensor();

    double   getSpeed(void);

private:
    float   m_speed;
    std::mutex  speedMutex;

    std::thread m_timer;
    void    _increaseSpeed(double increase);
};

#endif // TESTSPEEDSENSOR_H
