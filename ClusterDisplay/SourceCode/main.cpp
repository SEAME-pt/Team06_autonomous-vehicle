#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "BatteryIconObj.hpp"
#include "SpeedometerObj.hpp"
#include <zmq.hpp>


int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Test objects. Sending simulated values through zmq. They should be removed afterwards
    //TestBattery     tb;
    //TestSpeedSensor tss;

    // The subscriber objects. They read from the zmq socket and update the displayed values.
    BatteryIconObj  bio(nullptr);
    SpeedometerObj  so(nullptr);

    // Exposes the above objects to qml so it can access the updated values
    QQmlApplicationEngine engine;
    QQmlContext*    context(engine.rootContext());
    context->setContextProperty("batteryIconObj", &bio);
    context->setContextProperty("speedometerObj", &so);

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
