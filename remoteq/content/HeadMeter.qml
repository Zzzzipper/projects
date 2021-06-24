import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4
import QtQuick.Layouts 1.11
import QtQuick.Window 2.11
import Qt.labs.calendar 1.0
import QtQuick.Dialogs 1.3
import UdsClient 1.0
import QtQuick.Extras 1.4

Rectangle {
    property string index: ""
//    property string fromCallee: 'This value is send signal argument'
//    //Signal send when emitted activates fun1() defined in main QML
//    signal send(string pass);
//    onSend: {
//        console.log('Signal send has been emitted');
//    }

    Component.onCompleted: {
//        forward.connect(fun2);
//        send.connect(fun1);
//        send(fromCallee);
//        send(index);
    }
//    function fun2() {
//        console.log('The function fun2() is activated from main QML')
//    }

    ColumnLayout {
        id: columnLayout
        spacing: 0
        anchors.fill: parent

        Rectangle {
            id: rectangle1
            height: 35
            color: "#ffffff"
            anchors.right: parent.right
            anchors.rightMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 0
            anchors.top: parent.top
            anchors.topMargin: 0

            RowLayout {
                id: rowLayout
                height: 35
                spacing: 0
                anchors.fill: parent

                Rectangle {
                    id: rectangle2
                    width: 200
                    height: 200
                    color: "#0066d26a"
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    Label {
                        id: label
                        text: qsTr("Channel:")
                        leftPadding: 5
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                    }
                }

                Rectangle {
                    id: rectangle3
                    width: 200
                    height: 200
                    color: "#0065d16b"
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    Label {
                        id: channelName
                        text: "channel" + index
                        horizontalAlignment: Text.AlignHLeft
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                    }
                }

                Rectangle {
                    id: rectangle4
                    width: 200
                    height: 200
                    color: "#0065d16b"
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    Label {
                        id: label2
                        text: qsTr("Status:")
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        anchors.fill: parent
                    }
                }

                Rectangle {
                    id: rectangle5
                    width: 200
                    height: 200
                    color: "#0065d16b"
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    Label {
                        id: channelStatus
                        text: qsTr("")
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        anchors.fill: parent
                    }
                }
            }
        }

        Rectangle {
            id: rectangle9
            width: 200
            height: 200
            color: "#ffffff"
            border.color: "#aca1a1"
            border.width: 1
            Layout.fillHeight: true
            Layout.fillWidth: true
        }

        Rectangle {
            id: rectangle6
            height: 45
            color: "#57646c"
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 0
            RowLayout {
                id: rowLayout1
                Rectangle {
                    id: rectangle7
                    width: 200
                    height: 200
                    color: "#0065d16b"
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    Button {
                        id: button
                        text: qsTr("Start")
                        anchors.rightMargin: 10
                        anchors.leftMargin: 10
                        anchors.bottomMargin: 10
                        anchors.topMargin: 10
                        anchors.fill: parent
                    }
                }

                Rectangle {
                    id: rectangle8
                    width: 200
                    height: 200
                    color: "#0065d16b"
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    ComboBox {
                        id: comboBox
                        anchors.rightMargin: 10
                        anchors.bottomMargin: 10
                        anchors.topMargin: 10
                        anchors.fill: parent
                    }
                }
                anchors.fill: parent
                spacing: 0
            }
            anchors.right: parent.right
            anchors.leftMargin: 0
            anchors.left: parent.left
            anchors.rightMargin: 0
        }

    }





}


















/*##^## Designer {
    D{i:2;anchors_height:200;anchors_width:200;anchors_x:306;anchors_y:173}D{i:1;anchors_height:100;anchors_width:100}
}
 ##^##*/
