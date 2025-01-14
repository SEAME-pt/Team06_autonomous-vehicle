#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "QtSubscriber.hpp"

#include "TestBattery.hpp"
#include "BatteryIconObj.hpp"
#include "TestSpeedSensor.hpp"
#include "SpeedometerObj.hpp"
#include <zmq.hpp>


int main(int argc, char *argv[])
{
    QtSubscriber    subscriber;



    TestBattery     tb;
    BatteryIconObj  bio(nullptr, &tb);
    TestSpeedSensor tss;
    SpeedometerObj  so(nullptr, &tss);

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    QQmlContext*    context(engine.rootContext());
    context->setContextProperty("batteryIconObj", &bio); //Should be taken out
    context->setContextProperty("speedometerObj", &so); //Should be taken out

    bio.startUpdating();
    so.startUpdating();
    subscriber.startUpdating();
    return app.exec();
}
