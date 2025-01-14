import QtQuick 2.0
import QtQuick.Layouts 1.3

Rectangle {
    //THE NEXT TWO VALUES SHOULD BE PUT IN THE C++ FILE
    readonly property int mAX_SPEED: 20
    //property int speed: 1

    id: speedometer
    anchors {
        left: parent.left
        top: parent.top
        bottom: infoBar.top
    }
    width: parent.width / 8
    color: "transparent"

    Item { // An item which controls how much of the speedometer is shown, effectively displaying the speed
        id: speedometerClip
        anchors {
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        height: parent.height * (speedometerObj.speed / mAX_SPEED)
        clip: true

        ColumnLayout {
            id: speedBars
            readonly property int barsAm : 15
            function getColor(index)
            {
                if (barsAm - index < barsAm / 1.5) //Gotta find better maths for this lol
                    return "green"
                else if (barsAm - index < barsAm - (barsAm / 6))
                    return "yellow"
                else
                    return "red"
            }

            anchors {
                bottom: parent.bottom
                left: parent.left
                right: parent.right
            }
            height: speedometer.height
            spacing: 2
            Repeater {
                model: speedBars.barsAm
                SimpleBar {
                    Layout.maximumWidth: parent.width * ((speedBars.barsAm - index) * (1 / speedBars.barsAm))
                    color: speedBars.getColor(index)
                }
            }
        }
    }

    Text {
        anchors {
            bottom: parent.bottom
            right: parent.right
            rightMargin: 30
            bottomMargin: 30
        }
        font.pointSize: 15
        text: speedometerObj.speed + " km/h"
        color: "white"
    }
}
