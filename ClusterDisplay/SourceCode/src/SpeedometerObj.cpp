#include "SpeedometerObj.hpp"
#include <iostream>

SpeedometerObj::SpeedometerObj(QObject *parent)
    : ZmqSubscriber(SPEEDOMETER_ADDRESS, parent), m_speed{0}
{
    setSpeed(0); // Needed so the first value gets displayed on screen
}
SpeedometerObj::~SpeedometerObj(){}

double SpeedometerObj::speed(void) const {return (m_speed);}
void    SpeedometerObj::setSpeed(int newSpeed)
{
    if (newSpeed == m_speed)
        return ;
    m_speed = newSpeed;
    emit speedChanged(newSpeed); // This is what makes the value be updated on screen.
}

void    SpeedometerObj::_handleMsg(QString &message)
{
    setSpeed(message.toInt());
}
