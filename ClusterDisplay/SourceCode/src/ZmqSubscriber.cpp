#include "ZmqSubscriber.hpp"

ZmqSubscriber::ZmqSubscriber(const QString& address, QObject* parent)
    : QObject(parent), _context(1), _socket(_context, zmq::socket_type::sub)
{
    std::cout<<"lololol"<<std::endl;
    _socket.connect(address.toStdString());
    std::cout<<"lololol"<<std::endl;
    _socket.set(zmq::sockopt::subscribe, "");


    int socketFd = _socket.get(zmq::sockopt::fd);
    _notifier = std::unique_ptr<QSocketNotifier>(
                new QSocketNotifier(socketFd, QSocketNotifier::Read));
    connect(_notifier.get(), &QSocketNotifier::activated,
            this, &ZmqSubscriber::onMessageReceived);
    std::cout << "FUCK" << std::endl;
}

ZmqSubscriber::~ZmqSubscriber(){}

void    ZmqSubscriber::onMessageReceived()
{
    while (true)
    {
        zmq::message_t  message;
        zmq::recv_result_t  result = _socket.recv(message, zmq::recv_flags::dontwait);
        if (!result)
            break;
        std::string messageStr = message.to_string();
        std::cout << "raw message: " << messageStr << std::endl;
        QString msgContent = QString::fromStdString(message.to_string());
        _handleMsg(msgContent);
    }
}
