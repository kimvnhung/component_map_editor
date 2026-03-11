.pragma library

// Shared visual style for connection drawing. Keeping this centralized avoids
// repeating styling literals at each call site.
var EDGE_STYLE = {
    "strokeWidth": 2,
    "strokeWidthSelected": 3,
    "arrowLength": 12,
    "arrowAngleOffset": 0.4,
    "labelYOffset": 3,
    "normalColor": "#607d8b",
    "selectedColor": "#ff5722",
    "labelFont": "11px sans-serif"
}

var CONNECTION_TYPE = {
    "real": "real",
    "temp": "temp"
}

var CONNECTION_TYPE_STYLE = {
    "real": {
        "normalColor": EDGE_STYLE.normalColor,
        "selectedColor": EDGE_STYLE.selectedColor,
        "strokeWidth": EDGE_STYLE.strokeWidth,
        "strokeWidthSelected": EDGE_STYLE.strokeWidthSelected,
        "arrowLength": EDGE_STYLE.arrowLength,
        "arrowAngleOffset": EDGE_STYLE.arrowAngleOffset,
        "labelYOffset": EDGE_STYLE.labelYOffset,
        "labelFont": EDGE_STYLE.labelFont
    },
    "temp": {
        "normalColor": "#90caf9",
        "selectedColor": "#90caf9",
        "strokeWidth": EDGE_STYLE.strokeWidth,
        "strokeWidthSelected": EDGE_STYLE.strokeWidth,
        "arrowLength": EDGE_STYLE.arrowLength,
        "arrowAngleOffset": EDGE_STYLE.arrowAngleOffset,
        "labelYOffset": EDGE_STYLE.labelYOffset,
        "labelFont": EDGE_STYLE.labelFont
    }
}

// Clamps a numeric value inside [minValue, maxValue].
function clamp(value, minValue, maxValue) {
    return Math.max(minValue, Math.min(maxValue, value))
}

// Converts from scene space (pixels in the viewport) to world space
// (persistent graph coordinates used by ComponentModel.x/y).
function sceneToWorld(sceneX, sceneY, panX, panY, zoom) {
    return Qt.point((sceneX - panX) / zoom, -(sceneY - panY) / zoom)
}

// Converts from world space (graph coordinates) to scene space (viewport).
function worldToScene(worldX, worldY, panX, panY, zoom) {
    return Qt.point(worldX * zoom + panX, -worldY * zoom + panY)
}

// Returns a positive modulo in [0, modulus), useful for stable grid offsets
// even when pan is negative.
function positiveModulo(value, modulus) {
    return ((value % modulus) + modulus) % modulus
}

// Adapts the grid pixel spacing to remain readable across zoom levels.
// The power-of-two scaling keeps visual transitions smooth.
function normalizedGridStep(baseStep, zoom, minPixelStep, maxPixelStep) {
    var step = baseStep * zoom
    while (step < minPixelStep)
        step *= 2
    while (step > maxPixelStep)
        step /= 2
    return step
}

// Computes the camera state after zooming at a cursor position so that the
// world point under the cursor remains fixed on screen.
function zoomAtCursor(anchorWorldX, anchorWorldY, panX, panY, zoom,
                      zoomFactor, minZoom, maxZoom, epsilon) {
    var anchorScene = worldToScene(anchorWorldX, anchorWorldY, panX, panY,
                                   zoom)
    var nextZoom = clamp(zoom * zoomFactor, minZoom, maxZoom)
    if (Math.abs(nextZoom - zoom) < epsilon) {
        return {
            "changed": false,
            "panX": panX,
            "panY": panY,
            "zoom": zoom
        }
    }

    // Recompute pan from scene = world * zoom + pan so the same world point
    // remains under the original scene anchor after zoom changes.
    return {
        "changed": true,
        "zoom": nextZoom,
        "panX": anchorScene.x - anchorWorldX * nextZoom,
        "panY": anchorScene.y + anchorWorldY * nextZoom
    }
}

// Maps a point from an item's local coordinates to scene coordinates. If item is
function scenePoint(item, localX, localY) {
    if (item === null) {
        console.warn("sceneCoordinate called with null item, returning the input point as-is")
        return Qt.point(localX, localY)
    }

    return item.mapToItem(null, localX, localY)
}

// Computes component center in world coordinates for connection endpoint placement.
function componentCenter(component) {
    return Qt.point(component.x, component.y)
}

// Returns the four canonical connection points on a component bounding box.
// Component x/y are interpreted as the center of the box.
function componentConnectionPoints(component) {
    var center = componentCenter(component)
    var halfW = component.width / 2
    var halfH = component.height / 2

    return {
        "left": Qt.point(center.x - halfW, center.y),
        "right": Qt.point(center.x + halfW, center.y),
        "top": Qt.point(center.x, center.y - halfH),
        "bottom": Qt.point(center.x, center.y + halfH)
    }
}

// Returns the four canonical connection points for a rectangle expressed in
// the current drawing coordinate space. centerPoint uses that same space.
function rectangleConnectionPoints(centerPoint, width, height) {
    var halfW = width / 2
    var halfH = height / 2

    return {
        "left": Qt.point(centerPoint.x - halfW, centerPoint.y),
        "right": Qt.point(centerPoint.x + halfW, centerPoint.y),
        "top": Qt.point(centerPoint.x, centerPoint.y - halfH),
        "bottom": Qt.point(centerPoint.x, centerPoint.y + halfH)
    }
}

// Chooses boundary endpoints for a connection between two components.
// The side selection follows dominant axis direction from source to target.
function connectionEndpointsOnBounding(sourceComponent, targetComponent) {
    var srcCenter = componentCenter(sourceComponent)
    var tgtCenter = componentCenter(targetComponent)
    var dx = tgtCenter.x - srcCenter.x
    var dy = tgtCenter.y - srcCenter.y

    var srcPoints = componentConnectionPoints(sourceComponent)
    var tgtPoints = componentConnectionPoints(targetComponent)

    var sourcePoint
    var targetPoint

    if (Math.abs(dx) >= Math.abs(dy)) {
        sourcePoint = dx >= 0 ? srcPoints.right : srcPoints.left
        targetPoint = dx >= 0 ? tgtPoints.left : tgtPoints.right
    } else {
        sourcePoint = dy >= 0 ? srcPoints.bottom : srcPoints.top
        targetPoint = dy >= 0 ? tgtPoints.top : tgtPoints.bottom
    }

    return {
        "source": sourcePoint,
        "target": targetPoint
    }
}

// Chooses boundary endpoints between two rectangles in the active drawing
// coordinate space. This is used for live delegate geometry during dragging.
function connectionEndpointsBetweenRects(sourceCenter, sourceWidth, sourceHeight, targetCenter, targetWidth, targetHeight) {
    var dx = targetCenter.x - sourceCenter.x
    var dy = targetCenter.y - sourceCenter.y

    var srcPoints = rectangleConnectionPoints(sourceCenter, sourceWidth,
                                              sourceHeight)
    var tgtPoints = rectangleConnectionPoints(targetCenter, targetWidth,
                                              targetHeight)

    var sourcePoint
    var targetPoint

    if (Math.abs(dx) >= Math.abs(dy)) {
        sourcePoint = dx >= 0 ? srcPoints.right : srcPoints.left
        targetPoint = dx >= 0 ? tgtPoints.left : tgtPoints.right
    } else {
        sourcePoint = dy >= 0 ? srcPoints.bottom : srcPoints.top
        targetPoint = dy >= 0 ? tgtPoints.top : tgtPoints.bottom
    }

    return {
        "source": sourcePoint,
        "target": targetPoint
    }
}

function connectionStyle(connectionType) {
    return CONNECTION_TYPE_STYLE[connectionType] || CONNECTION_TYPE_STYLE.real
}

// Draws a directed connection (line + arrow + optional label) on a 2D canvas.
// All coordinates are scene coordinates in the GraphCanvas space (Y-down).
function drawConnection(ctx, startPoint, endPoint, connectionType, label, isSelected) {
    var edgeStyle = connectionStyle(connectionType)
    var strokeColor = isSelected ? edgeStyle.selectedColor : edgeStyle.normalColor
    var strokeWidth = isSelected ? edgeStyle.strokeWidthSelected : edgeStyle.strokeWidth
    var arrowLength = edgeStyle.arrowLength
    var arrowAngleOffset = edgeStyle.arrowAngleOffset
    var labelYOffset = edgeStyle.labelYOffset
    var sx = startPoint.x
    var sy = startPoint.y
    var tx = endPoint.x
    var ty = endPoint.y

    ctx.strokeStyle = strokeColor
    ctx.lineWidth = strokeWidth

    ctx.beginPath()
    ctx.moveTo(sx, sy)
    ctx.lineTo(tx, ty)
    ctx.stroke()

    var angle = Math.atan2(ty - sy, tx - sx)
    ctx.fillStyle = strokeColor
    ctx.beginPath()
    ctx.moveTo(tx, ty)
    ctx.lineTo(tx - arrowLength * Math.cos(angle - arrowAngleOffset),
               ty - arrowLength * Math.sin(angle - arrowAngleOffset))
    ctx.lineTo(tx - arrowLength * Math.cos(angle + arrowAngleOffset),
               ty - arrowLength * Math.sin(angle + arrowAngleOffset))
    ctx.closePath()
    ctx.fill()

    if (label) {
        ctx.fillStyle = strokeColor
        ctx.font = edgeStyle.labelFont
        ctx.textAlign = "center"
        ctx.textBaseline = "bottom"
        ctx.fillText(label, (sx + tx) / 2, (sy + ty) / 2 - labelYOffset)
    }
}

// Optional helper for a future fully-canvas component renderer.
// Current implementation uses ComponentItem.qml delegates, so this is not used yet.
function drawComponent(ctx, component) {
    var x = component.x
    var y = component.y
    var width = component.width
    var height = component.height
    var radius = component.radius
    var fillColor = component.fillColor
    var borderColor = component.borderColor
    var borderWidth = component.borderWidth

    ctx.beginPath()
    ctx.moveTo(x + radius, y)
    ctx.lineTo(x + width - radius, y)
    ctx.quadraticCurveTo(x + width, y, x + width, y + radius)
    ctx.lineTo(x + width, y + height - radius)
    ctx.quadraticCurveTo(x + width, y + height, x + width - radius, y + height)
    ctx.lineTo(x + radius, y + height)
    ctx.quadraticCurveTo(x, y + height, x, y + height - radius)
    ctx.lineTo(x, y + radius)
    ctx.quadraticCurveTo(x, y, x + radius, y)
    ctx.closePath()

    ctx.fillStyle = fillColor
    ctx.fill()

    ctx.lineWidth = borderWidth
    ctx.strokeStyle = borderColor
    ctx.stroke()
}
