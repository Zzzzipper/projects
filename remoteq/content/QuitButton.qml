import QtQuick 2.0
Image {
    source: "quit.png"
    scale: quitMouse.pressed ? 0.8 : 1.0
    MouseArea {
        id: quitMouse
        anchors.fill: parent
        anchors.margins: -10
        onClicked: Qt.quit()
    }
}
