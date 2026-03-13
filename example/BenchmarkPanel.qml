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
    }

    function _stopMeasuringAndReport() {
        root.telemetry.enabled = false
        root.frameTelemetry.enabled = false
        root.telemetry.computeStats()
        root.frameTelemetry.computeStats()
        root.telemetry.report("Baseline — nodes=" + root.graph.componentCount
                              + " edges=" + root.graph.connectionCount)
        root.frameTelemetry.report("Baseline — nodes=" + root.graph.componentCount
                                   + " edges=" + root.graph.connectionCount)
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
            if (root.canvas) {
                // Smooth Lissajous-like motion to continuously trigger redraw.
                root.canvas.panX = startPanX + Math.sin(t * 2.4) * amp
                root.canvas.panY = startPanY + Math.cos(t * 1.8) * amp * 0.6
            }

            // Force a render request even if scene updates get coalesced.
            if (root.appWindow)
                root.appWindow.requestUpdate()

            root.autoPanRemainingSec = Math.max(0, (20000 - elapsedMs) / 1000.0)

            if (elapsedMs >= 20000) {
                running = false
                root.autoPanRunning = false
                root.autoPanRemainingSec = 0
                root._stopMeasuringAndReport()
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
                model: [1000, 5000, 10000]
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
                    root.autoPanRunning = false
                    root.autoPanRemainingSec = 0
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
                        root.autoPanRunning = false
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
                    root._startMeasuring()
                    root.autoPanRunning = true
                    autoPanTimer.startPanX = root.canvas.panX
                    autoPanTimer.startPanY = root.canvas.panY
                    autoPanTimer.startedMs = Date.now()
                    root.autoPanRemainingSec = 20
                    autoPanTimer.running = true
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
                                text: root.telemetry.enabled
                                     ? (root.autoPanRunning
                                         ? "Sampling (auto-pan " + root.autoPanRemainingSec.toFixed(1) + "s left)..."
                                         : "Sampling...")
                                            : (root.frameSampleValid
                                                 ? "Sample valid"
                                                 : "Invalid sample size (need >= " + root.minValidFrameSamples
                                                     + ", got " + root.frameTelemetry.frameSwapSampleCount + ")")
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
    }
}
