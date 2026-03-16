// PropertyPanel.qml — Shows and edits properties of the currently selected
// component or connection.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ComponentMapEditor

Rectangle {
    id: root

    property ComponentModel component: null
    property ConnectionModel connection: null

    color: "#ffffff"
    border.color: "#e0e0e0"
    border.width: 1

    ColumnLayout {
        anchors { fill: parent; margins: 10 }
        spacing: 8

        Label {
            text: "Properties"
            font.bold: true
            font.pixelSize: 13
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            bottomPadding: 4
        }

        // ---------- Component properties ----------
        Loader {
            active: root.component !== null
            Layout.fillWidth: true
            sourceComponent: componentProps
        }

        Component {
            id: componentProps

            ColumnLayout {
                spacing: 6
                width: parent ? parent.width : 0

                Label { text: "Component"; font.bold: true; font.pixelSize: 11; color: "#888" }

                Label { text: "ID" }
                TextField {
                    Layout.fillWidth: true
                    text: root.component ? root.component.id : ""
                    onEditingFinished: if (root.component) root.component.id = text
                }

                Label { text: "Title" }
                TextField {
                    Layout.fillWidth: true
                    text: root.component ? root.component.title : ""
                    onEditingFinished: if (root.component) root.component.title = text
                }

                Label { text: "Content" }
                TextArea {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 64
                    text: root.component ? root.component.content : ""
                    wrapMode: Text.WordWrap
                    onEditingFinished: if (root.component) root.component.content = text
                }

                Label { text: "Icon" }
                TextField {
                    Layout.fillWidth: true
                    text: root.component ? root.component.icon : ""
                    placeholderText: "e.g. cube, house, user"
                    onEditingFinished: if (root.component) root.component.icon = text
                }

                Label { text: "Color" }
                TextField {
                    Layout.fillWidth: true
                    text: root.component ? root.component.color : ""
                    onEditingFinished: if (root.component) root.component.color = text
                }

                Label { text: "Type" }
                TextField {
                    Layout.fillWidth: true
                    text: root.component ? root.component.type : ""
                    onEditingFinished: if (root.component) root.component.type = text
                }

                Label { text: "Shape" }
                ComboBox {
                    Layout.fillWidth: true
                    model: ["rounded", "rectangle"]
                    currentIndex: root.component && root.component.shape === "rectangle" ? 1 : 0
                    onActivated: function(index) {
                        if (root.component) root.component.shape = model[index]
                    }
                }

                Label { text: "X" }
                SpinBox {
                    Layout.fillWidth: true
                    from: -9999; to: 9999
                    value: root.component ? Math.round(root.component.x) : 0
                    onValueModified: if (root.component) root.component.x = value
                }

                Label { text: "Y" }
                SpinBox {
                    Layout.fillWidth: true
                    from: -9999; to: 9999
                    value: root.component ? Math.round(root.component.y) : 0
                    onValueModified: if (root.component) root.component.y = value
                }

                Label { text: "Width" }
                SpinBox {
                    Layout.fillWidth: true
                    from: 10; to: 9999
                    value: root.component ? Math.round(root.component.width) : 96
                    onValueModified: if (root.component) root.component.width = value
                }

                Label { text: "Height" }
                SpinBox {
                    Layout.fillWidth: true
                    from: 10; to: 9999
                    value: root.component ? Math.round(root.component.height) : 96
                    onValueModified: if (root.component) root.component.height = value
                }
            }
        }

        // ---------- Connection properties ----------
        Loader {
            active: root.connection !== null
            Layout.fillWidth: true
            sourceComponent: connectionProps
        }

        Component {
            id: connectionProps

            ColumnLayout {
                spacing: 6
                width: parent ? parent.width : 0

                function sideIndexForValue(value) {
                    for (let i = 0; i < sideModel.length; ++i) {
                        if (sideModel[i].value === value)
                            return i
                    }
                    return 0
                }

                readonly property var sideModel: [
                    { text: "Auto", value: ConnectionModel.SideAuto },
                    { text: "Top", value: ConnectionModel.SideTop },
                    { text: "Right", value: ConnectionModel.SideRight },
                    { text: "Bottom", value: ConnectionModel.SideBottom },
                    { text: "Left", value: ConnectionModel.SideLeft }
                ]

                Label { text: "Connection"; font.bold: true; font.pixelSize: 11; color: "#888" }

                Label { text: "ID" }
                TextField {
                    Layout.fillWidth: true
                    text: root.connection ? root.connection.id : ""
                    onEditingFinished: if (root.connection) root.connection.id = text
                }

                Label { text: "Source" }
                TextField {
                    Layout.fillWidth: true
                    text: root.connection ? root.connection.sourceId : ""
                    onEditingFinished: if (root.connection) root.connection.sourceId = text
                }

                Label { text: "Target" }
                TextField {
                    Layout.fillWidth: true
                    text: root.connection ? root.connection.targetId : ""
                    onEditingFinished: if (root.connection) root.connection.targetId = text
                }

                Label { text: "Label" }
                TextField {
                    Layout.fillWidth: true
                    text: root.connection ? root.connection.label : ""
                    onEditingFinished: if (root.connection) root.connection.label = text
                }

                Label { text: "Source Side" }
                ComboBox {
                    Layout.fillWidth: true
                    model: connectionProps.sideModel
                    textRole: "text"
                    currentIndex: root.connection
                                  ? connectionProps.sideIndexForValue(root.connection.sourceSide)
                                  : 0
                    onActivated: function(index) {
                        if (root.connection)
                            root.connection.sourceSide = connectionProps.sideModel[index].value
                    }
                }

                Label { text: "Target Side" }
                ComboBox {
                    Layout.fillWidth: true
                    model: connectionProps.sideModel
                    textRole: "text"
                    currentIndex: root.connection
                                  ? connectionProps.sideIndexForValue(root.connection.targetSide)
                                  : 0
                    onActivated: function(index) {
                        if (root.connection)
                            root.connection.targetSide = connectionProps.sideModel[index].value
                    }
                }
            }
        }

        // Placeholder when nothing is selected
        Label {
            visible: root.component === null && root.connection === null
            text: "Select a component or connection\nto view its properties."
            wrapMode: Text.WordWrap
            color: "#aaa"
            font.pixelSize: 12
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        Item { Layout.fillHeight: true }
    }
}
