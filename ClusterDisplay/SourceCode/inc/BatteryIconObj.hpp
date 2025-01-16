#ifndef BATTERYICONOBJ_HPP
#define BATTERYICONOBJ_HPP

#include <QObject>
#include <QQmlEngine>
#include "ZmqSubscriber.hpp"

# define BATTERY_ADDRESS "tcp://localhost:5556"

class BatteryIconObj : public ZmqSubscriber
{
    Q_OBJECT
    Q_PROPERTY(int percentage READ percentage WRITE setPercentage NOTIFY percentageChanged FINAL)

public:
    explicit BatteryIconObj(QObject *parent = nullptr);
    ~BatteryIconObj();

    int     percentage(void) const;
    void    setPercentage(int newPercentage);

signals:
    void    percentageChanged(int);

protected:
    void    _handleMsg(QString& message) override;

private:
    int     m_percentage;
};

#endif // BATTERYICONOBJ_HPP
