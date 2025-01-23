import QtQuick 2.9
import QtQuick.Window 2.2

Window {
    visible: true
    width: 1024
    height: 600
    title: qsTr("Hello World")

    color: "black"
    InfoBar {
        id: infoBar
    }

    Speedometer {
        id: speedometer
    }

    BatteryIndicator {
        id: batteryIndicator
    }
}
