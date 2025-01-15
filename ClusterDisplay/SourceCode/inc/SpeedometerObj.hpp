#ifndef SPEEDOMETEROBJ_HPP
#define SPEEDOMETEROBJ_HPP

#include <QObject>
#include <QQmlEngine>
#include "ZmqSubscriber.hpp"
//#include "TestSpeedSensor.hpp"

# define SPEED_UPDATE_RATE 100
# define ADDRESS "tcp://localhost:5555"

class SpeedometerObj : public ZmqSubscriber
{
    Q_OBJECT
    Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY speedChanged FINAL)

public:
    explicit SpeedometerObj(QObject *parent = nullptr);
    ~SpeedometerObj();

    double     speed(void) const;
    void    setSpeed(int newSpeed);

signals:
    void    speedChanged(double);

protected:
    void    _handleMsg(QString& message) override;
private:
    double              m_speed;
};

#endif // SPEEDOMETEROBJ_HPP
