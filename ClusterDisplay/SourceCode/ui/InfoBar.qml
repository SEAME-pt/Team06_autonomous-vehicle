import QtQuick 2.0
import QtQuick.Layouts 1.3

Rectangle {
    id: infoBar
    anchors {
        left: parent.left
        right: parent.right
        bottom: parent.bottom
    }
    color: "grey"
    height: parent.height / 16

    RowLayout {
        anchors.fill: parent
        Item {
            anchors.fill: parent
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
    }
}
