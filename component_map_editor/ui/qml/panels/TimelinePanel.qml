import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import ComponentMapEditor

Item {
    property alias model: listview.model
    ColumnLayout {
        anchors.fill: parent
        Label {
            Layout.fillWidth: true
            Layout.preferredHeight: 20
            text: "Timeline"
            font.bold: true
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 20
            spacing: 6
            Label {
                text: "Regex pattern:"
            }
            TextField {
                Layout.fillWidth: true
                placeholderText: "Enter regex pattern"
                onTextChanged: {
                    if (listview.model && listview.model.regexFilter !== undefined)
                        listview.model.regexFilter = text;
                }
            }
        }
        ListView {
            id: listview
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 6
            clip: true
            delegate: TimelineItem {}
        }
    }
}
