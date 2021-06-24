import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4
import QtQuick.Layouts 1.11
import QtQuick.Window 2.11
import Qt.labs.calendar 1.0
import QtQuick.Dialogs 1.3
import UdsClient 1.0
import QtQuick.Extras 1.4
import "qrc:/content"

ApplicationWindow {
    id: mainWindow
    width: 250
    height: 600
    flags: Qt.CustomizeWindowHint
    color: "#6d6060"
    visible: true

//    signal forward //This signal activates fun2() defined in QML string
//    onForward: {
//        console.log("forward signal is emitted in main QML")
//    }
//    Rectangle {
//        id: dummy
//        Component.onCompleted: {
//            forward();
//        }
//    }

//    function fun1(argument1) {
//        console.log("A function fun1()in the main QML file is invoked")
//        console.log("Returned parameter from the QML string = ", argument1)
//        forward();
//    }

    function message(msg, finished) {
        var alert = Qt.createComponent("qrc:/content/MessageBox.qml").createObject(mainWindow, { text: msg });
        alert.onFinished.connect(function(ok) {
            if (ok) {
                if (finished)
                    finished();
            }
            alert.destroy(500);
        });
        alert.init();
        return alert;
    }

    MouseArea{
        id: mouseArea
        property int prevX: 0
        property int prevY: 0
        anchors.fill: parent
        onPressed: {prevX=mouse.x; prevY=mouse.y}
        onPositionChanged:{
            var deltaX = mouse.x - prevX;
            mainWindow.x += deltaX;
            prevX = mouse.x - deltaX;

            var deltaY = mouse.y - prevY
            mainWindow.y += deltaY;
            prevY = mouse.y - deltaY;
        }
    }

    UdsClient {
        id: client
        onHaveError: {
            console.log(message);
            mainWindow.message(message, function() { console.log("OK clicked"); });
        }
        onChannelInfo: {
            var parts = info.split(",");
            if(parts.length > 1) {
                var h = headerBox.height / (parts.length - 1),
                w = headerBox.width;
                var component = Qt.createComponent("qrc:/content/HeadMeter.qml");
                for(var i = 1; i < parts.length; ++i) {
                    var dynamicObject = component.createObject(headerBox, {
                                                                   "id": "channel" + (i-1),
                                                                   "index": "" + (i-1),
                                                                   "x": 0,
                                                                   "y": (i-1)*h,
                                                                   "height": h,
                                                                   "width" : w,
                                                               });
                }
            }
        }
    }

    Component.onCompleted: {
        //        mainWindow.showFullScreen();
        client.getChannelInfo();
    }

    ColumnLayout {
        id: columnLayout
        spacing: 0
        anchors.fill: parent

        Rectangle {
            id: rectangle
            width: 200
            height: 30
            color: "#00000000"
            Layout.maximumHeight: 40
            Layout.fillHeight: true
            Layout.minimumHeight: 0
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.fillWidth: true

            RowLayout {
                id: rowLayout
                anchors.fill: parent
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                Layout.fillHeight: true
                Layout.fillWidth: true

                Button {
                    id: buttonSendHttp
                    width: 60
                    height: 35
                    text: qsTr("Http Send")
                    Layout.bottomMargin: 5
                    Layout.fillHeight: true
                    Layout.fillWidth: false
                    Layout.maximumWidth: 60
                    Layout.topMargin: 5
                    Layout.leftMargin: 5
                    onClicked: {
                        client.connect();
                    }
                }

                Button {
                    id: buttonSendSocket
                    width: 60
                    height: 35
                    text: qsTr("SslSocket")
                    Layout.bottomMargin: 5
                    Layout.fillHeight: true
                    Layout.maximumWidth: 60
                    Layout.minimumWidth: 0
                    Layout.minimumHeight: 0
                    Layout.maximumHeight: 65535
                    Layout.topMargin: 5
                    onClicked: {
                        client.getStatus("0");
                    }
                }

                Rectangle {
                    id: rectangle1
                    width: 200
                    height: 200
                    color: "#00000000"
                    Layout.minimumHeight: 1
                    Layout.preferredHeight: 3
                    Layout.fillWidth: true
                    Layout.fillHeight: false
                }

                QuitButton {
                    id: buttonClose
                    Layout.topMargin: 2
                    Layout.fillHeight: false
                    Layout.fillWidth: false
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 10
                }
            }
        }

        Rectangle {
            id: rectangle2
            width: 200
            height: 200
            color: "#ffffff"
            Layout.fillHeight: true
            Layout.fillWidth: true

            ColumnLayout {
                id: headerBox
                anchors.fill: parent
            }
        }
    }
}



























/*##^## Designer {
    D{i:1;anchors_height:100;anchors_width:100;anchors_x:268;anchors_y:153}D{i:8;anchors_height:20;anchors_width:80;anchors_x:"-10";anchors_y:418}
D{i:5;anchors_height:30;anchors_width:100;anchors_x:0;anchors_y:11}D{i:11;anchors_height:100;anchors_width:100}
}
 ##^##*/
