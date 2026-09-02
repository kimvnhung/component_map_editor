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
                onClicked: sandbox.start()
            }
            Button {
                text: "Step"
                onClicked: sandbox.step()
            }
            Button {
                text: "Run"
                onClicked: sandbox.run()
            }
            Button {
                text: "Pause"
                onClicked: sandbox.pause()
            }
            Button {
                text: "Reset"
                onClicked: sandbox.reset()
            }
            Text {
                text: "Status: " + statusText()
                font.pixelSize: 20
            }
        }

        RowLayout {
            spacing: 10
            ColumnLayout {
                spacing: 10
                Label {
                    text: "Component state"
                    font.pixelSize: 20
                }
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: sandbox.states
                    delegate: ExecutionSnapshotViewer {
                        snapshot: modelData
                    }
                }
            }

            ColumnLayout {
                spacing: 10
                Label {
                    text: "Timeline Events"
                    font.pixelSize: 20
                }
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: sandbox.timeLine
                    delegate: JsonViewer {
                        title: title
                        value: value
                    }
                }
            }

            // showing step counter
            Text {
                text: "Step: " + sandbox.stepCounter
                font.pixelSize: 20
            }
        }
    }

    function statusText() {
        switch (sandbox.executionStatus) {
        case 0:
            return "Not started";
        case 1:
            return "Running";
        case 2:
            return "Paused";
        case 3:
            return "Stepping";
        case 4:
            return "Completed";
        case 5:
            return "Error";
        default:
            return "Unknown";
        }
    }
}
