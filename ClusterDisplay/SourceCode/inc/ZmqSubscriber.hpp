#ifndef ZMQSUBSCRIBER_HPP
#define ZMQSUBSCRIBER_HPP

# include <QObject>
# include <QSocketNotifier>
# include <zmq.hpp>
# include <memory> // For unique poiner

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
    // The context of the socket. Manages thread amount nder the hood.
    zmq::context_t  _context;
    // The socket itself, through which the values will be received from the publisher.
    zmq::socket_t   _socket;
    // The notifier which will manage the socket and send activates the slot which manages the incoming messages.
    std::unique_ptr<QSocketNotifier> _notifier;
};

#endif // ZMQSUBSCRIBER_HPP
