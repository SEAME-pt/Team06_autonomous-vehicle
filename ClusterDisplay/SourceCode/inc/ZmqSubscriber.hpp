#ifndef ZMQSUBSCRIBER_HPP
#define ZMQSUBSCRIBER_HPP

# include <QObject>
# include <QSocketNotifier>
# include <zmq.hpp>
# include <memory>

#include <iostream>

class ZmqSubscriber : public QObject
{
    Q_OBJECT
public:
    ZmqSubscriber(const QString& address, QObject* parent = nullptr);
    ~ZmqSubscriber();
public slots:
    void    onMessageReceived();

protected:
    virtual void    _handleMsg(QString& message) = 0;
private:
    zmq::context_t  _context;
    zmq::socket_t   _socket;
    std::unique_ptr<QSocketNotifier> _notifier;
};

#endif // ZMQSUBSCRIBER_HPP
