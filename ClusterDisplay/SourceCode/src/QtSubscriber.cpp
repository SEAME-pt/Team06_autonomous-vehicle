#include "QtSubscriber.hpp"
#include <iostream>

QtSubscriber::QtSubscriber(QObject *parent)
    : QObject(parent), _zmqContext(1),
      _socket(_zmqContext, zmq::socket_type::sub)
{
    _socket.set(zmq::sockopt::conflate, true);
    _socket.connect("tcp://localhost:5556");
    _socket.set(zmq::sockopt::subscribe, "");

}

QtSubscriber::~QtSubscriber(){}

void    QtSubscriber::startUpdating(void)
{
    std::cout << "the fuck?" << std::endl;
    QTimer* updater = new QTimer(this);
    connect(updater, &QTimer::timeout, this, &QtSubscriber::checkForMessages);
    updater->start(1000);
}

void    QtSubscriber::checkForMessages()
{
    zmq::message_t  message;
    std::cout << "lol?" << std::endl;
    while (_socket.recv(message, zmq::recv_flags::dontwait))
    {
        QString receivedData = QString::fromStdString(message.to_string());
        std::cout << qPrintable(receivedData) << std::endl;
    }
}
