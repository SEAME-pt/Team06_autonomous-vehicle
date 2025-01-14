import QtQuick 2.0
import QtQuick.Layouts 1.3

Rectangle {
    //TEST VALUE: CHANGE IT FOR CPP OBJECT VALUE
    //property int percentage: 21

    id: batteryIndicator
    anchors {
        right: parent.right
        top: parent.top
        bottom: infoBar.top
    }
    width: parent.width / 8
    color: "transparent"

    Item { // An item which controls how much of the batteryIndicator is shown, effectively displaying the battery level
        id: batteryIndicatorClip
        anchors {
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        height: parent.height * batteryIconObj.percentage * 0.01
        clip: true

        ColumnLayout {
            id: batteryBars
            readonly property int barsAm : 15
            function getColor(percentage)
            {
                if (percentage > 50) //Gotta find better maths for this lol
                    return "green"
                else if (percentage > 20)
                    return "yellow"
                else
                    return "red"
            }

            anchors {
                bottom: parent.bottom
                left: parent.left
                right: parent.right
            }
            height: batteryIndicator.height
            spacing: 2
            Repeater {
                model: batteryBars.barsAm
                SimpleBar {
                    Layout.alignment: Qt.AlignRight
                    Layout.maximumWidth: parent.width * ((batteryBars.barsAm - index) * (1 / batteryBars.barsAm))
                    color: batteryBars.getColor(batteryIconObj.percentage)
                }
            }
        }
    }
    Text {
        anchors {
            bottom: parent.bottom
            left: parent.left
            leftMargin: 30
            bottomMargin: 30
        }
        font.pointSize: 15
        text: batteryIconObj.percentage + "%"
        color: "white"
    }
}
