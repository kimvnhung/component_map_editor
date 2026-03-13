// PerformanceTelemetry.qml
// Lightweight wall-clock telemetry for GraphCanvas.
//
// Attach to GraphCanvas.telemetry to start collecting samples.
// Call report() or computeStats() to read results.
//
// What is measured:
//   Camera update interval — wall-clock time between consecutive pan/zoom
//     signal deliveries. Approximates event-loop throughput for camera ops.
//   Drag event interval    — wall-clock time between consecutive pointer-move
//     deliveries during pan or node-drag. Approximates drag responsiveness.
//
// NOTE: These are signal-delivery intervals, not GPU swap times.
//   For GPU frame time use QSG_RENDERER_DEBUG=render in the environment.
import QtQuick

QtObject {
    id: root

    // Enable or disable sample collection. Toggling off does not clear samples.
    property bool enabled: false

    // Maximum samples kept per category (oldest dropped when exceeded).
    property int maxSamples: 2000

    // ── Computed statistics (refreshed by computeStats / report) ────────────
    property real frameTimeP50: 0          // ms — camera-event interval p50
    property real frameTimeP95: 0          // ms — camera-event interval p95
    property int  frameTimeSampleCount: 0

    property real dragLatencyP50: 0        // ms — drag-event interval p50
    property real dragLatencyP95: 0        // ms — drag-event interval p95
    property int  dragLatencySampleCount: 0

    // ── Internal state ───────────────────────────────────────────────────────
    property var  _frameSamples: []
    property var  _dragSamples:  []
    property real _lastCameraMs: -1        // -1 = no prior sample
    property real _lastDragMs:   -1

    // ── Public API ───────────────────────────────────────────────────────────

    // Call from GraphCanvas on every camera property change (panX or zoom).
    // Calling only for panX (not panY) avoids double-counting per drag step.
    function notifyCameraChanged() {
        if (!enabled)
            return
        var now = Date.now()
        if (_lastCameraMs >= 0) {
            var dt = now - _lastCameraMs
            if (dt > 0 && dt < 500) {
                _frameSamples.push(dt)
                if (_frameSamples.length > maxSamples)
                    _frameSamples.shift()
            }
        }
        _lastCameraMs = now
    }

    // Call when a drag gesture (background pan or node move) starts.
    function notifyDragStarted() {
        if (!enabled)
            return
        _lastDragMs = Date.now()
    }

    // Call on each pointer-move event within an active drag.
    function notifyDragMoved() {
        if (!enabled || _lastDragMs < 0)
            return
        var now = Date.now()
        var dt  = now - _lastDragMs
        if (dt > 0 && dt < 500) {
            _dragSamples.push(dt)
            if (_dragSamples.length > maxSamples)
                _dragSamples.shift()
        }
        _lastDragMs = now
    }

    // Call when the active drag ends.
    function notifyDragEnded() {
        _lastDragMs = -1
    }

    // Recomputes all p50/p95 values from current sample buffers.
    function computeStats() {
        frameTimeP50           = _pct(_frameSamples, 0.50)
        frameTimeP95           = _pct(_frameSamples, 0.95)
        frameTimeSampleCount   = _frameSamples.length
        dragLatencyP50         = _pct(_dragSamples,  0.50)
        dragLatencyP95         = _pct(_dragSamples,  0.95)
        dragLatencySampleCount = _dragSamples.length
    }

    // Calls computeStats() then prints a formatted report to the console.
    function report(tag) {
        computeStats()
        var sep = "======================================================="
        console.log(sep)
        console.log("[PerfTelemetry] " + (tag || "Baseline Report"))
        console.log("  Camera update interval  p50=" + frameTimeP50.toFixed(2)
                    + " ms   p95=" + frameTimeP95.toFixed(2)
                    + " ms   n="  + frameTimeSampleCount)
        console.log("  Drag event interval     p50=" + dragLatencyP50.toFixed(2)
                    + " ms   p95=" + dragLatencyP95.toFixed(2)
                    + " ms   n="  + dragLatencySampleCount)
        console.log(sep)
    }

    // Clears all samples and resets computed statistics. Does not affect enabled.
    function reset() {
        _frameSamples          = []
        _dragSamples           = []
        _lastCameraMs          = -1
        _lastDragMs            = -1
        frameTimeP50           = 0
        frameTimeP95           = 0
        frameTimeSampleCount   = 0
        dragLatencyP50         = 0
        dragLatencyP95         = 0
        dragLatencySampleCount = 0
    }

    // ── Private helper ───────────────────────────────────────────────────────

    function _pct(arr, p) {
        if (!arr || arr.length === 0)
            return 0
        var s = arr.slice().sort(function (a, b) { return a - b })
        return s[Math.floor(p * (s.length - 1))]
    }
}
