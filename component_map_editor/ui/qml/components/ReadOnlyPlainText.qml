import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    id: plainTextPanel
    property string panelText: ""
    property int preferredHeight: 120

    Layout.fillWidth: true
    Layout.preferredHeight: preferredHeight
    clip: true

    TextArea {
        text: plainTextPanel.panelText || ""
        readOnly: true
        wrapMode: TextArea.NoWrap
        selectByMouse: true
        font.family: "monospace"
        font.pixelSize: 12
    }
}
