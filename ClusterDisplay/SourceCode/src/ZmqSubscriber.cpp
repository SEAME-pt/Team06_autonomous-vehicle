#include "ZmqSubscriber.hpp"
#include <QDateTime>

ZmqSubscriber::ZmqSubscriber(const QString& address, QObject* parent)
    : QObject(parent), _context(1), _socket(_context, zmq::socket_type::sub)
{
    // Sets the address the socket should read from and subscribes to it.
    _socket.connect(address.toStdString());
    _socket.set(zmq::sockopt::subscribe, "");

    // Sets up a socket notifier to send a signal in case of activity.
    int socketFd = _socket.get(zmq::sockopt::fd);
    _notifier = std::unique_ptr<QSocketNotifier>(
                new QSocketNotifier(socketFd, QSocketNotifier::Read));
    connect(_notifier.get(), &QSocketNotifier::activated,
            this, &ZmqSubscriber::onMessageReceived);
}

ZmqSubscriber::~ZmqSubscriber(){}

void    ZmqSubscriber::onMessageReceived()
{
    // Gets messages while there are messages to get.
    while (true)
    {
        zmq::message_t  message;
        zmq::recv_result_t  result = _socket.recv(message, zmq::recv_flags::dontwait);
        if (!result)
            break;
        std::string messageStr = message.to_string();
        QString msgContent = QString::fromStdString(message.to_string());
        // Calls pure virtual method so each child class can deal with the message.
        _handleMsg(msgContent);
    }
}
