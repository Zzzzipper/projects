import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4
import QtQuick.Layouts 1.11
import QtQuick.Window 2.11
import Qt.labs.calendar 1.0

ApplicationWindow {
    id: mainWindow
    width: 400
    height: 500
    color: "#f2f1f1"
    visible: true

    //    Component.onCompleted: {
    //        mainWindow.showFullScreen();
    //    }

    ColumnLayout {
        id: columnLayout
        anchors.fill: parent

        RowLayout {
            id: rowLayout
            width: 100
            height: 100
            Layout.fillHeight: true
            Layout.fillWidth: true

            Button {
                id: buttonSendHttp
                width: 60
                text: qsTr("Http Send")
                Layout.maximumWidth: 60
                Layout.topMargin: 10
                Layout.leftMargin: 10
                onClicked: {
                    networkManager.requestHttp()
                }
            }

            Button {
                id: buttonSendSocket
                width: 60
                text: qsTr("SslSocket")
                Layout.maximumWidth: 60
                Layout.minimumWidth: 60
                Layout.minimumHeight: 40
                Layout.maximumHeight: 60
                Layout.topMargin: 10
                onClicked: {
                    networkManager.requestSocket()
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

            Button {
                id: buttonClose
                icon.source: "qrc:/images/close-app.png"
                Layout.rightMargin: 10
                Layout.topMargin: 10
                width: 39
                font.capitalization: Font.AllLowercase
                Layout.maximumWidth: 40
                Layout.minimumWidth: 40
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                ToolTip.delay: 0
                ToolTip.timeout: 2000
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Close app")
                onClicked: {
                    Qt.quit();
                }
            }


        }

        Rectangle {
            id: rectangle
            width: 200
            height: 200
            color: "#ffffff"
            Layout.rightMargin: 10
            Layout.leftMargin: 10
            Layout.bottomMargin: 10
            Layout.topMargin: 5
            Layout.fillHeight: true
            Layout.fillWidth: true

            TextEdit {
                id: textEdit
                text: qsTr("Text Edit")
                anchors.fill: parent
                font.pixelSize: 12
            }
        }

    }
}



















/*##^## Designer {
    D{i:8;anchors_height:20;anchors_width:80;anchors_x:"-10";anchors_y:418}D{i:1;anchors_height:100;anchors_width:100;anchors_x:268;anchors_y:153}
}
 ##^##*/
