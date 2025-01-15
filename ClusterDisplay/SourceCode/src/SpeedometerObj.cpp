#include "SpeedometerObj.hpp"
#include <iostream>

SpeedometerObj::SpeedometerObj(QObject *parent)
    : ZmqSubscriber(ADDRESS, parent), m_speed{0}
{
    //qDebug("SpeedometerObj Constructor called.");
    setSpeed(0);
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
    std::cout<<"FINALY: " << newSpeed << std::endl;
    //qDebug("Speed setter was called with value %i", newSpeed);
    if (newSpeed == m_speed)
        return ;
    m_speed = newSpeed;
    emit speedChanged(newSpeed);
}

void    SpeedometerObj::_handleMsg(QString &message)
{
    setSpeed(message.toInt());
}
