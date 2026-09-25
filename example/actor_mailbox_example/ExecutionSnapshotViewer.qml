import QtQuick 2.15
import QtQuick.Controls 2.15

import ActorMailboxExample 1.0

Item {
    id: root

    property var snapshot: null

    implicitWidth: jsonViewer.implicitWidth
    implicitHeight: jsonViewer.implicitHeight

    function snapshotToJson(snapshot) {
        if (!snapshot)
            return {};

        console.log("snapshotToJson called with snapshot:", JSON.stringify(snapshot));

        return {
            "componentId": snapshot.componentId,
            "componentSnapshot": snapshot.componentSnapshot,
            "inputTokens": snapshot.inputTokens,
            "result": {
                "success": snapshot.result.success,
                "outputState": snapshot.result.outputState,
                "message": snapshot.result.message
            },
            "outputTokens": snapshot.outputTokens
        };
    }

    JsonViewer {
        id: jsonViewer

        anchors.fill: parent

        title: "Execution Snapshot"

        value: root.snapshot ? root.snapshotToJson(root.snapshot) : {}
        expanded: true
    }
}
