#ifndef QTSUBSCRIBER_HPP
#define QTSUBSCRIBER_HPP

#include <QObject>
#include <QQmlEngine>
#include <QTimer>
#include <zmq.hpp>

class QtSubscriber : public QObject
{
    Q_OBJECT

public:
    QtSubscriber(QObject *parent = nullptr);
    ~QtSubscriber();

    void    startUpdating(void);

private slots:
    void    checkForMessages();
private:
    zmq::context_t  _zmqContext;
    zmq::socket_t   _socket;
};

#endif // QTSUBSCRIBER_HPP
