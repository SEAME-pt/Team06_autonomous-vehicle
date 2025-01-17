import QtQuick 2.0

Item {
    //anchors.fill: parent
    Text {
        id: time
        text: Qt.formatDateTime(new Date(), "hh:mm:ss")
        anchors.centerIn: parent
    }

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: {
            time.text = Qt.formatDateTime(new Date(), "hh:mm:ss")
        }
    }
}
