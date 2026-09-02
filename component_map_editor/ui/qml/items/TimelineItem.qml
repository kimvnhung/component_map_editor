import QtQuick
import QtQuick.Controls

Item {
    required property string event
    required property int tick
    required property var payload

    width: ListView.view.width

    implicitHeight: body.implicitHeight + 8

    Row {

        anchors.fill: parent

        spacing: 10

        //---------------------------------
        // Timeline
        //---------------------------------

        Item {

            width: 32

            anchors.top: parent.top
            anchors.bottom: parent.bottom

            Rectangle {

                width: 2
                color: "#808080"

                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.bottom: parent.bottom
            }

            Rectangle {

                width: 12
                height: 12

                radius: 6

                color: {
                    switch (event) {
                    case "ERROR":
                        return "#e53935";
                    case "WARN":
                        return "#ff9800";
                    case "DEBUG":
                        return "#42a5f5";
                    default:
                        return "#43a047";
                    }
                }

                anchors.horizontalCenter: parent.horizontalCenter

                y: 10
            }
        }

        //---------------------------------
        // Content
        //---------------------------------

        Rectangle {
            id: body

            width: parent.width - 50

            implicitHeight: column.implicitHeight + 4

            Column {
                id: column

                anchors.fill: parent

                anchors.margins: 8

                spacing: 4

                Row {

                    spacing: 12

                    Text {

                        text: tick

                        color: "#AAAAAA"
                    }

                    Text {

                        text: event

                        font.bold: true
                    }
                }

                Text {

                    width: parent.width

                    wrapMode: Text.WordWrap

                    text: JSON.stringify(payload, null, 2)
                }
            }
        }
    }
}
