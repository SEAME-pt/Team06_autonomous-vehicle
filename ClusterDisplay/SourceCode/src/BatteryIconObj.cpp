#include "BatteryIconObj.hpp"
#include <QTimer>

BatteryIconObj::BatteryIconObj(QObject *parent)
    : ZmqSubscriber(BATTERY_ADDRESS, parent), m_percentage{0}
{
    setPercentage(0);
};
BatteryIconObj::~BatteryIconObj(){}

int BatteryIconObj::percentage(void) const {return m_percentage;}
void    BatteryIconObj::setPercentage(int newPercentage)
{
    if (newPercentage == m_percentage)
        return ;
    m_percentage = newPercentage;
    emit percentageChanged(newPercentage);
}

void    BatteryIconObj::_handleMsg(QString &message)
{
    setPercentage(message.toInt());
}
