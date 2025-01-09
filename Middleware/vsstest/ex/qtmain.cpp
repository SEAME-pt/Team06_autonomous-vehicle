#include <QApplication>
#include <QLabel>
#include <zmq.hpp>
#include <thread>
#include <QJsonDocument>
#include <QJsonObject>

void receiveSensorData(QLabel *speedLabel, QLabel *batteryLabel) {
    zmq::context_t context(1);
    zmq::socket_t subscriber(context, ZMQ_SUB);
    subscriber.connect("tcp://localhost:5555");
    subscriber.set(zmq::sockopt::subscribe, "");

    while (true) {
        zmq::message_t zmq_message;
        subscriber.recv(zmq_message, zmq::recv_flags::none);

        // Parse JSON message
        QString data = QString::fromUtf8(static_cast<char*>(zmq_message.data()), zmq_message.size());
        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
        QJsonObject root = doc.object();

        // Extract values using VSS paths
        float speed = root["Vehicle"]["Speed"].toDouble();
        float battery = root["Vehicle"]["Battery"].toDouble();

        // Update UI
        QString speedText = QString("Speed: %1 km/h").arg(speed);
        QString batteryText = QString("Battery: %1%").arg(battery);

        QMetaObject::invokeMethod(speedLabel, "setText", Q_ARG(QString, speedText));
        QMetaObject::invokeMethod(batteryLabel, "setText", Q_ARG(QString, batteryText));
    }
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QWidget window;
    QVBoxLayout layout;
    QLabel speedLabel("Speed: N/A km/h");
    QLabel batteryLabel("Battery: N/A%");

    layout.addWidget(&speedLabel);
    layout.addWidget(&batteryLabel);
    window.setLayout(&layout);

    std::thread receiver(receiveSensorData, &speedLabel, &batteryLabel);
    receiver.detach();

    window.show();
    return app.exec();
}
