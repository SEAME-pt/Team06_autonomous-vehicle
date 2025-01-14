#ifndef BATTERYICONOBJ_HPP
#define BATTERYICONOBJ_HPP

#include <QObject>
#include <QQmlEngine>
#include "TestBattery.hpp"

# define BATTERY_UPDATE_RATE 1500

class BatteryIconObj : public QObject
{
    Q_OBJECT
    //QML_ELEMENT
    Q_PROPERTY(int percentage READ percentage WRITE setPercentage NOTIFY percentageChanged FINAL)

public:
    explicit BatteryIconObj(QObject *parent = nullptr, TestBattery* battery = nullptr);
    ~BatteryIconObj();

    int     percentage(void) const;
    void    setPercentage(int newPercentage);
    void    startUpdating();

signals:
    void    percentageChanged(int);

private:
    int     m_percentage;
    TestBattery*    _battery;


    void    _fetchPercentage(void);
};

#endif // BATTERYICONOBJ_HPP
