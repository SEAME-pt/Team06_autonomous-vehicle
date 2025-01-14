#include "BatteryIconObj.hpp"
#include <QTimer>

BatteryIconObj::BatteryIconObj(QObject *parent, TestBattery* battery)
    : QObject{parent}, m_percentage{0}, _battery{battery}
{
    //qDebug("BatteryIconObj Constructor called.");
    _fetchPercentage();
}

BatteryIconObj::~BatteryIconObj()
{
    //qDebug("BatteryIconObj destructor called.");
}

int BatteryIconObj::percentage(void) const
{
    //qDebug("Percentage getter called");
    return m_percentage;
}

void    BatteryIconObj::setPercentage(int newPercentage)
{
    //qDebug("Percentage setter was called with value %i", newPercentage);
    if (newPercentage == m_percentage)
        return ;
    m_percentage = newPercentage;
    emit percentageChanged(newPercentage);
}

void    BatteryIconObj::startUpdating(void)
{
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &BatteryIconObj::_fetchPercentage);
    timer->start(BATTERY_UPDATE_RATE);
}

void    BatteryIconObj::_fetchPercentage(void)
{
    int newPercentage = 0;
    if (_battery)
        newPercentage = static_cast<int>(_battery->getPercentage());
    setPercentage(newPercentage);
}
