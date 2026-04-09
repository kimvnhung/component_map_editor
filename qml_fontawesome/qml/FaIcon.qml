import QtQuick
import QmlFontAwesome

Text {
    id: root

    property string iconName: ""
    property int iconStyle: FontAwesome.Solid

    text: FontAwesome.icon(iconName, iconStyle)
    font.family: FontAwesome.family(iconStyle)
    font.weight: FontAwesome.weight(iconStyle)
    renderType: Text.NativeRendering
    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignHCenter
}
