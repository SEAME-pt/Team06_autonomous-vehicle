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
            Layout.fillHeight: true
            Layout.fillWidth: true
            Text {
                anchors.centerIn: parent
                text: "Team06"
            }
        }
        TimeLabel {
            id: timeLabel
            Layout.fillHeight: true
            Layout.minimumWidth: 70 // Keeps time to the right, but with enough space to show the entire timeText
        }
    }
}
