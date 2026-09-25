import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts
import QtQuick.Controls 2.15

import ActorMailboxExample 1.0

Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("Actor Mailbox Example")

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        RowLayout {
            spacing: 10
            Button {
                text: "Start"
                onClicked: sandbox.execute()
            }
            Button {
                text: "Stop"
                onClicked: sandbox.stop()
            }
            Text {
                text: sandbox.isRunning ? "Running" : "Stopped"
                font.pixelSize: 20
            }
        }

        ListView {
            Layout.fillHeight: true
            Layout.fillWidth: true
            model: sandbox.timeline
            delegate: Text {
                text: modelData
            }
        }
    }
}
