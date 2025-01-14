#include "SpeedometerObj.hpp"
#include <QTimer>

SpeedometerObj::SpeedometerObj(QObject *parent, TestSpeedSensor* speedSensor)
    : QObject{parent}, m_speed{0}, _speedSensor{speedSensor}
{
    //qDebug("SpeedometerObj Constructor called.");
    _fetchSpeed();
}

SpeedometerObj::~SpeedometerObj()
{
    //qDebug("SpeedometerObj destructor called.");
}

double SpeedometerObj::speed(void) const
{
    //qDebug("Speed getter called");
    return m_speed;
}

void    SpeedometerObj::setSpeed(int newSpeed)
{
    //qDebug("Speed setter was called with value %i", newSpeed);
    if (newSpeed == m_speed)
        return ;
    m_speed = newSpeed;
    emit speedChanged(newSpeed);
}

void    SpeedometerObj::startUpdating(void)
{
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &SpeedometerObj::_fetchSpeed);
    timer->start(SPEED_UPDATE_RATE);
}

void    SpeedometerObj::_fetchSpeed(void)
{
    double newSpeed = 0;
    if (_speedSensor)
        newSpeed = static_cast<double>(_speedSensor->getSpeed());
    setSpeed(newSpeed);
}
