// BenchmarkPanel.qml — Phase 0 performance instrumentation panel.
//
// Attach to the bottom of the example ApplicationWindow.
// Provides:
//   • Scenario loading (1k / 5k / 10k nodes via C++ BenchmarkHelper)
//   • Live memory readout (RSS via MemoryMonitor, polled every 2 s)
//   • Telemetry toggle — start/stop PerformanceTelemetry collection
//   • Live stats row — camera-update and drag-event p50/p95
//
// Color coding for stat values:
//   green  ≤ 16.7 ms  (60 fps budget)
//   orange  16.7–33 ms
//   red    > 33 ms
import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import ComponentMapEditor

Rectangle {
    id: root

    required property GraphModel         graph
    // Use var-typed references because we need GraphCanvas custom properties
    // (panX/panY) and window methods (requestUpdate) that are not part of the
    // base static QML type declarations.
    required property var                appWindow
    required property var                canvas
    required property PerformanceTelemetry telemetry
    required property FrameSwapTelemetry frameTelemetry

    property bool expanded: false
    readonly property int minValidFrameSamples: 100
    readonly property bool frameSampleValid: frameTelemetry.frameSwapSampleCount >= minValidFrameSamples
    property bool autoPanRunning: false
    property real autoPanRemainingSec: 0
    property bool runSeriesActive: false
    property int runSeriesTotal: 3
    property int runSeriesCurrent: 0
    property var runSeriesResults: []
    property string runSeriesSummary: ""
    property bool manualDragWindowActive: false
    property bool matrixActive: false
    property int matrixDatasetIndex: 0
    property int matrixStageIndex: 0
    property real matrixRemainingSec: 0
    property var matrixResults: []
    property string matrixSummary: ""
    readonly property var matrixDatasets: [1000, 5000, 10000]
    readonly property var matrixStageNames: ["Idle30s", "Pan10s", "Zoom10s"]
    property int manualDragInteractions: 0
    readonly property int manualDragTarget: 200

    readonly property int autoPanDurationMs: 20000
    readonly property int manualDragDurationMs: 10000
    readonly property int matrixIdleDurationMs: 30000
    readonly property int matrixPanDurationMs: 10000
    readonly property int matrixZoomDurationMs: 10000

    readonly property real frameP95TargetMs: 16.7
    readonly property real cameraP95TargetMs: 16.7
    readonly property real dragP95TargetMs: 8.0

    color:  "#f5f5f5"
    implicitHeight: expanded ? 152 : 36
    clip:   true

    Behavior on implicitHeight {
        NumberAnimation { duration: 140; easing.type: Easing.OutQuad }
    }

    // ── C++ helpers ──────────────────────────────────────────────────────────
    BenchmarkHelper { id: benchHelper }
    MemoryMonitor   { id: memMonitor  }

    // ── Memory polling ───────────────────────────────────────────────────────
    property int _rssKb:         0
    property int _baselineRssKb: 0

    Timer {
        interval: 2000
        repeat:   true
        running:  true
        onTriggered:       root._rssKb = memMonitor.rssKb()
        Component.onCompleted: root._rssKb = memMonitor.rssKb()
    }

    function _captureBaseline() {
        _baselineRssKb = memMonitor.rssKb()
        _rssKb         = _baselineRssKb
    }

    function _startMeasuring() {
        root.telemetry.reset()
        root.frameTelemetry.reset()
        root.telemetry.enabled = true
        root.frameTelemetry.enabled = true
        root.manualDragWindowActive = false
    }

    function _stopMeasuringAndReport(tag) {
        root.telemetry.enabled = false
        root.frameTelemetry.enabled = false
        root.telemetry.computeStats()
        root.frameTelemetry.computeStats()

        var reportTag = tag
        if (!reportTag || reportTag.length === 0) {
            reportTag = "Baseline — nodes=" + root.graph.componentCount
                        + " edges=" + root.graph.connectionCount
        }

        root.telemetry.report(reportTag)
        root.frameTelemetry.report(reportTag)

        var hasDrag = root.telemetry.dragLatencySampleCount > 0
        return {
            "frameSwapP95": root.frameTelemetry.frameSwapP95,
            "cameraP95": root.telemetry.frameTimeP95,
            "dragP95": root.telemetry.dragLatencyP95,
            "dragSamples": root.telemetry.dragLatencySampleCount,
            "framePass": root.frameTelemetry.frameSwapP95 > 0
                         && root.frameTelemetry.frameSwapP95 <= root.frameP95TargetMs,
            "cameraPass": root.telemetry.frameTimeP95 > 0
                          && root.telemetry.frameTimeP95 <= root.cameraP95TargetMs,
            "dragPass": hasDrag && root.telemetry.dragLatencyP95 <= root.dragP95TargetMs
        }
    }

    function _startNextSeriesRun() {
        if (root.runSeriesCurrent >= root.runSeriesTotal) {
            root.runSeriesActive = false
            var allPass = root.runSeriesResults.length === root.runSeriesTotal
            for (var i = 0; i < root.runSeriesResults.length; ++i) {
                var run = root.runSeriesResults[i]
                allPass = allPass && run.framePass && run.cameraPass && run.dragPass
            }
            root.runSeriesSummary = allPass
                    ? "Series PASS: all 3 runs met frame/camera/drag p95 targets"
                    : "Series FAIL: at least one run missed frame/camera/drag p95 targets"
            return
        }

        root.runSeriesCurrent += 1
        root._startMeasuring()
        root.autoPanRunning = true
        autoPanTimer.startPanX = root.canvas.panX
        autoPanTimer.startPanY = root.canvas.panY
        autoPanTimer.startedMs = Date.now()
        root.autoPanRemainingSec = root.autoPanDurationMs / 1000.0
        root.manualDragWindowActive = false
        autoPanTimer.running = true
    }

    function _matrixStageDurationMs(stageIndex) {
        if (stageIndex === 0)
            return root.matrixIdleDurationMs
        if (stageIndex === 1)
            return root.matrixPanDurationMs
        return root.matrixZoomDurationMs
    }

    function _startValidationMatrix() {
        autoPanTimer.running = false
        root.autoPanRunning = false
        root.autoPanRemainingSec = 0
        root.runSeriesActive = false
        root.manualDragWindowActive = false

        root.matrixActive = true
        root.matrixDatasetIndex = 0
        root.matrixStageIndex = 0
        root.matrixResults = []
        root.matrixSummary = ""
        root._startNextMatrixStage()
    }

    function _startNextMatrixStage() {
        if (!root.matrixActive)
            return

        if (root.matrixDatasetIndex >= root.matrixDatasets.length) {
            root.matrixActive = false
            root.matrixRemainingSec = 0
            root.matrixSummary = "Validation matrix complete: " + root.matrixResults.length
                               + " stage runs across " + root.matrixDatasets.length + " datasets"
            return
        }

        var datasetSize = root.matrixDatasets[root.matrixDatasetIndex]
        if (root.matrixStageIndex === 0) {
            root._captureBaseline()
            if (root.canvas && root.canvas.nodeRenderer)
                root.canvas.nodeRenderer.renderNodes = true
            benchHelper.populateGraph(root.graph, datasetSize)
        }

        root._startMeasuring()
        matrixTimer.datasetSize = datasetSize
        matrixTimer.stageIndex = root.matrixStageIndex
        matrixTimer.startPanX = root.canvas ? root.canvas.panX : 0
        matrixTimer.startPanY = root.canvas ? root.canvas.panY : 0
        matrixTimer.startZoom = root.canvas ? root.canvas.zoom : 1.0
        matrixTimer.zoomInDirection = true
        matrixTimer.startedMs = Date.now()
        root.matrixRemainingSec = root._matrixStageDurationMs(root.matrixStageIndex) / 1000.0
        matrixTimer.running = true
    }

    function _advanceMatrixStage() {
        root.matrixStageIndex += 1
        if (root.matrixStageIndex >= root.matrixStageNames.length) {
            root.matrixStageIndex = 0
            root.matrixDatasetIndex += 1
        }
        root._startNextMatrixStage()
    }

    function _samplingStatusText() {
        if (!root.telemetry.enabled) {
            return root.frameSampleValid
                    ? "Sample valid"
                    : "Invalid sample size (need >= " + root.minValidFrameSamples
                      + ", got " + root.frameTelemetry.frameSwapSampleCount + ")"
        }

        if (root.autoPanRunning) {
            if (root.runSeriesActive) {
                if (root.manualDragWindowActive) {
                    return "Manual drag now (series " + root.runSeriesCurrent + "/"
                         + root.runSeriesTotal + ", "
                         + root.autoPanRemainingSec.toFixed(1) + "s left)"
                }
                return "Sampling auto-pan (series " + root.runSeriesCurrent + "/"
                     + root.runSeriesTotal + ", "
                     + root.autoPanRemainingSec.toFixed(1) + "s left)..."
            }

            return "Sampling (auto-pan " + root.autoPanRemainingSec.toFixed(1)
                 + "s left)..."
        }

        if (root.matrixActive) {
            return "Matrix " + root.matrixStageNames[root.matrixStageIndex]
                 + " (dataset " + root.matrixDatasets[root.matrixDatasetIndex]
                 + ", " + root.matrixRemainingSec.toFixed(1) + "s left)..."
        }

        return "Sampling..."
    }

    Timer {
        id: autoPanTimer
        interval: 12
        repeat: true
        running: false

        property double startedMs: 0
        property real startPanX: 0
        property real startPanY: 0

        onTriggered: {
            var elapsedMs = Date.now() - startedMs
            var t = elapsedMs / 1000.0
            var amp = 260.0
            if (root.canvas && (!root.runSeriesActive || !root.manualDragWindowActive)) {
                // Smooth Lissajous-like motion to continuously trigger redraw.
                root.canvas.panX = startPanX + Math.sin(t * 2.4) * amp
                root.canvas.panY = startPanY + Math.cos(t * 1.8) * amp * 0.6
            }

            // Force a render request even if scene updates get coalesced.
            if (root.appWindow)
                root.appWindow.requestUpdate()

            if (root.runSeriesActive && root.manualDragWindowActive) {
                root.autoPanRemainingSec = Math.max(0, (root.manualDragDurationMs - elapsedMs) / 1000.0)
            } else {
                root.autoPanRemainingSec = Math.max(0, (root.autoPanDurationMs - elapsedMs) / 1000.0)
            }

            if (root.runSeriesActive && !root.manualDragWindowActive
                    && elapsedMs >= root.autoPanDurationMs) {
                // Give users a manual drag window after each auto-pan phase.
                root.manualDragWindowActive = true
                startedMs = Date.now()
                root.autoPanRemainingSec = root.manualDragDurationMs / 1000.0
                return
            }

            if ((!root.runSeriesActive && elapsedMs >= root.autoPanDurationMs)
                    || (root.runSeriesActive && root.manualDragWindowActive
                        && elapsedMs >= root.manualDragDurationMs)) {
                running = false
                root.autoPanRunning = false
                root.manualDragWindowActive = false
                root.autoPanRemainingSec = 0
                if (root.runSeriesActive) {
                    var result = root._stopMeasuringAndReport(
                                "Series run " + root.runSeriesCurrent + "/" + root.runSeriesTotal
                                + " — nodes=" + root.graph.componentCount
                                + " edges=" + root.graph.connectionCount)
                    root.runSeriesResults = root.runSeriesResults.concat([result])
                    root._startNextSeriesRun()
                } else {
                    root._stopMeasuringAndReport()
                }
            }
        }
    }

    Timer {
        id: matrixTimer
        interval: 12
        repeat: true
        running: false

        property double startedMs: 0
        property real startPanX: 0
        property real startPanY: 0
        property real startZoom: 1.0
        property bool zoomInDirection: true
        property int stageIndex: 0
        property int datasetSize: 0

        onTriggered: {
            if (!root.matrixActive || !root.canvas)
                return

            var elapsedMs = Date.now() - startedMs
            var durationMs = root._matrixStageDurationMs(stageIndex)
            var t = elapsedMs / 1000.0

            if (stageIndex === 1) {
                var amp = 220.0
                root.canvas.panX = startPanX + Math.sin(t * 2.5) * amp
                root.canvas.panY = startPanY + Math.cos(t * 1.9) * amp * 0.6
            } else if (stageIndex === 2) {
                root.canvas.mouseViewPos = Qt.point(root.canvas.width * 0.5,
                                                    root.canvas.height * 0.5)
                var factor = zoomInDirection ? 1.01 : (1.0 / 1.01)
                root.canvas.zoomAtCursor(factor)
                if ((elapsedMs % 420) < interval)
                    zoomInDirection = !zoomInDirection
            }

            if (root.appWindow)
                root.appWindow.requestUpdate()

            root.matrixRemainingSec = Math.max(0, (durationMs - elapsedMs) / 1000.0)

            if (elapsedMs >= durationMs) {
                running = false
                var stageName = root.matrixStageNames[stageIndex]
                var result = root._stopMeasuringAndReport(
                            "Matrix " + stageName + " — nodes=" + datasetSize
                            + " edges=" + root.graph.connectionCount)
                result.dataset = datasetSize
                result.stage = stageName
                root.matrixResults = root.matrixResults.concat([result])
                root._advanceMatrixStage()
            }
        }
    }

    // ── Layout ───────────────────────────────────────────────────────────────
    ColumnLayout {
        anchors {
            fill:         parent
            leftMargin:   8
            rightMargin:  8
            topMargin:    4
            bottomMargin: 4
        }
        spacing: 4

        // ── Row 1: always-visible header ─────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            ToolButton {
                text:         root.expanded ? "▲ Benchmark" : "▼ Benchmark"
                font.pixelSize: 12
                implicitHeight: 28
                onClicked:    root.expanded = !root.expanded
            }

            Label {
                text: "Nodes: " + root.graph.componentCount
                      + "  Edges: " + root.graph.connectionCount
                font.pixelSize: 11
                color: "#555"
            }

            Item { Layout.fillWidth: true }

            Label {
                text: "RSS: " + (root._rssKb / 1024).toFixed(1) + " MB"
                      + "   Δ " + ((root._rssKb - root._baselineRssKb) / 1024).toFixed(1) + " MB"
                                            + (root.canvas && root.canvas.nodeRenderer
                                                     ? "   Renderer≈" + root.canvas.nodeRenderer.rendererMemoryEstimateMb().toFixed(1) + " MB"
                                                         + "   LabelCache=" + root.canvas.nodeRenderer.labelTextureCacheSize()
                                                     : "")
                font.pixelSize: 11
                color: "#555"
            }
        }

        // ── Row 2: scenario load buttons ─────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            visible: root.expanded

            Label { text: "Load:"; font.pixelSize: 11 }

            Repeater {
                model: [1000, 5000, 10000, 20000]
                delegate: Button {
                    required property int modelData
                    text:           modelData >= 1000 ? (modelData / 1000) + "k" : String(modelData)
                    font.pixelSize: 11
                    implicitHeight: 26
                    implicitWidth:  44
                    onClicked: {
                        root._captureBaseline()
                        if (root.canvas && root.canvas.nodeRenderer)
                            root.canvas.nodeRenderer.renderNodes = true
                        benchHelper.populateGraph(root.graph, modelData)
                    }
                }
            }

            Button {
                text:           "Clear"
                font.pixelSize: 11
                implicitHeight: 26
                implicitWidth:  52
                onClicked: {
                    autoPanTimer.running = false
                    matrixTimer.running = false
                    root.autoPanRunning = false
                    root.autoPanRemainingSec = 0
                    root.runSeriesActive = false
                    root.runSeriesCurrent = 0
                    root.runSeriesResults = []
                    root.runSeriesSummary = ""
                    root.manualDragWindowActive = false
                    root.matrixActive = false
                    root.matrixDatasetIndex = 0
                    root.matrixStageIndex = 0
                    root.matrixRemainingSec = 0
                    root.matrixResults = []
                    root.matrixSummary = ""
                    root.manualDragInteractions = 0
                    root.telemetry.enabled = false
                    root.frameTelemetry.enabled = false
                    if (root.canvas) {
                        if (root.canvas.nodeRenderer)
                            root.canvas.nodeRenderer.renderNodes = false
                        root.canvas.tempConnectionDragging = false
                        root.canvas.nodeInteractionActive = false
                        root.canvas.enableBackgroundDrag = true
                        root.canvas.selectedComponent = null
                        root.canvas.selectedConnection = null
                    }
                    root.graph.clear()
                    root._captureBaseline()
                }
            }
        }

        // ── Row 3: telemetry controls ─────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: root.expanded

            Button {
                text:           root.telemetry.enabled ? "■  Stop & Report" : "●  Start Measuring"
                font.pixelSize: 11
                implicitHeight: 26
                implicitWidth:  140
                highlighted:    root.telemetry.enabled
                onClicked: {
                    if (root.telemetry.enabled) {
                        autoPanTimer.running = false
                        matrixTimer.running = false
                        root.autoPanRunning = false
                        root.runSeriesActive = false
                        root.manualDragWindowActive = false
                        root.matrixActive = false
                        root.matrixRemainingSec = 0
                        root._stopMeasuringAndReport()
                    } else {
                        root._startMeasuring()
                    }
                }
            }

            Button {
                text: root.autoPanRunning ? "Running..." : "▶ Auto-pan 20s"
                enabled: !root.autoPanRunning && root.canvas !== null
                font.pixelSize: 11
                implicitHeight: 26
                implicitWidth:  110
                onClicked: {
                    if (!root.canvas)
                        return
                    matrixTimer.running = false
                    root.matrixActive = false
                    root.matrixRemainingSec = 0
                    root._startMeasuring()
                    root.runSeriesActive = false
                    root.autoPanRunning = true
                    autoPanTimer.startPanX = root.canvas.panX
                    autoPanTimer.startPanY = root.canvas.panY
                    autoPanTimer.startedMs = Date.now()
                    root.autoPanRemainingSec = root.autoPanDurationMs / 1000.0
                    root.manualDragWindowActive = false
                    autoPanTimer.running = true
                }
            }

            Button {
                text: root.runSeriesActive
                      ? "Series " + root.runSeriesCurrent + "/" + root.runSeriesTotal
                      : "▶ Auto-pan x3"
                enabled: !root.runSeriesActive && !root.autoPanRunning && root.canvas !== null
                font.pixelSize: 11
                implicitHeight: 26
                implicitWidth:  108
                onClicked: {
                    if (!root.canvas)
                        return
                    matrixTimer.running = false
                    root.matrixActive = false
                    root.matrixRemainingSec = 0
                    root.runSeriesActive = true
                    root.runSeriesCurrent = 0
                    root.runSeriesResults = []
                    root.runSeriesSummary = ""
                    root._startNextSeriesRun()
                }
            }

            Button {
                text: root.matrixActive
                      ? "Matrix " + (root.matrixDatasetIndex + 1) + "/" + root.matrixDatasets.length
                      : "▶ Validation Matrix"
                enabled: !root.matrixActive && !root.autoPanRunning && !root.runSeriesActive && root.canvas !== null
                font.pixelSize: 11
                implicitHeight: 26
                implicitWidth:  132
                onClicked: {
                    if (!root.canvas)
                        return
                    root._startValidationMatrix()
                }
            }

            Button {
                text:           "↻ Refresh Stats"
                font.pixelSize: 11
                implicitHeight: 26
                implicitWidth:  96
                onClicked: {
                    root.telemetry.computeStats()
                    root.frameTelemetry.computeStats()
                }
            }

            Item { Layout.fillWidth: true }

                        Label {
                                text: root._samplingStatusText()
                                font.pixelSize: 11
                                color: root.telemetry.enabled
                                             ? "#1565c0"
                                             : (root.frameSampleValid ? "#2e7d32" : "#c62828")
                        }
        }

        // ── Row 4: live statistics ────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 20
            visible: root.expanded

            Repeater {
                model: [
                    { lbl: "Swap p50", val: root.frameTelemetry.frameSwapP50, n: root.frameTelemetry.frameSwapSampleCount },
                    { lbl: "Swap p95", val: root.frameTelemetry.frameSwapP95, n: -1 },
                    { lbl: "Cam p50",  val: root.telemetry.frameTimeP50,   n: root.telemetry.frameTimeSampleCount  },
                    { lbl: "Cam p95",  val: root.telemetry.frameTimeP95,   n: -1 },
                    { lbl: "Drag p50", val: root.telemetry.dragLatencyP50, n: root.telemetry.dragLatencySampleCount },
                    { lbl: "Drag p95", val: root.telemetry.dragLatencyP95, n: -1 }
                ]

                delegate: Column {
                    required property var modelData
                    spacing: 1

                    Label {
                        text:           modelData.lbl
                        font.pixelSize: 10
                        color:          "#888"
                    }
                    Label {
                        text: modelData.val.toFixed(1) + " ms"
                              + (modelData.n >= 0 ? "  (n=" + modelData.n + ")" : "")
                        font.pixelSize: 12
                        font.bold:      true
                        color: {
                            if (modelData.val <= 0)          return "#888"
                            if (modelData.val <= 16.7)       return "#2e7d32"
                            if (modelData.val <= 33.0)       return "#e65100"
                            return "#c62828"
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root.expanded && root.runSeriesSummary.length > 0

            Label {
                Layout.fillWidth: true
                text: root.runSeriesSummary
                font.pixelSize: 11
                color: root.runSeriesSummary.indexOf("PASS") >= 0 ? "#2e7d32" : "#c62828"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root.expanded
            spacing: 8

            Label {
                text: "Manual drag: " + root.manualDragInteractions + "/" + root.manualDragTarget
                font.pixelSize: 11
                color: root.manualDragInteractions >= root.manualDragTarget ? "#2e7d32" : "#555"
            }

            Button {
                text: "+1"
                font.pixelSize: 11
                implicitHeight: 24
                implicitWidth: 34
                onClicked: root.manualDragInteractions += 1
            }

            Button {
                text: "+10"
                font.pixelSize: 11
                implicitHeight: 24
                implicitWidth: 40
                onClicked: root.manualDragInteractions += 10
            }

            Button {
                text: "Reset"
                font.pixelSize: 11
                implicitHeight: 24
                implicitWidth: 52
                onClicked: root.manualDragInteractions = 0
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root.expanded && root.matrixSummary.length > 0

            Label {
                Layout.fillWidth: true
                text: root.matrixSummary
                font.pixelSize: 11
                color: "#455a64"
            }
        }
    }
}
