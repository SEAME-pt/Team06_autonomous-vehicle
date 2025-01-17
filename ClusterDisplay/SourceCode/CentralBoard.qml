import QtQuick 2.0

Image {
    anchors {
        top: parent.top
        left: speedometer.right
        right: batteryIndicator.left
        bottom: infoBar.top
    }

    source: "camera_icon.jpg"
}
