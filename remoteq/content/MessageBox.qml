import QtQuick 2.2
import QtQuick.Controls 1.2

Rectangle {
    id: mainWrapper
    color: "#80000000"
    x: 0;
    y: 0;
    width: parent.width;
    height: parent.height;
    opacity: 0;
    Behavior on opacity { NumberAnimation { duration: 500; easing.type: Easing.OutExpo } }
    visible: opacity > 0

    property string text;

    MouseArea {
        anchors.fill: parent;
        preventStealing: true
    }

    signal finished(bool ok);

    function init() {
        opacity = 1;
        msgB.scale = 1.0;
    }

    Rectangle {
        id: msgB
        color: "#323232"
        gradient: Gradient {
            GradientStop { position: 0; color: "#323232" }
            GradientStop { position: 1; color: "#252525" }
        }
        //radius: 7

//        width: parent.width * 0.6;
//        height: cont.height + 20 * 2;
        anchors.centerIn: parent;
        scale: 0.6
        Behavior on scale { NumberAnimation { duration: 500; easing.type: Easing.OutExpo } }
        Behavior on height { NumberAnimation { duration: 500; easing.type: Easing.OutExpo } }

        Column {
            id: cont
            width: parent.width;
            y: 20;
            spacing: 20;
            Text {
                color: "#ffffff"
                horizontalAlignment: Text.AlignHCenter
                anchors.horizontalCenter: parent.horizontalCenter
                font {
                    bold: false;
                    pixelSize: 16;
                }
                wrapMode: Text.WordWrap;
                text: mainWrapper.text;
            }
            Button {
                anchors.margins: 10
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                text: "OK"
                onClicked: {
                    mainWrapper.opacity = 0;
                    msgB.scale = 0.6;
                    mainWrapper.finished(true);
                }
            }
        }
    }
}
