import QtQuick
import QtQuick.Shapes

Shape {
    id: arrow
    
    property real arrowWidth: 2
    property color arrowColor: "#000000"
    property real arrowSize: 20
    property int direction: 0  // 0: up, 1: right, 2: down, 3: left
    
    readonly property real rotation: direction * 90
    
    width: arrowSize
    height: arrowSize
    
    ShapePath {
        strokeColor: arrow.arrowColor
        fillColor: arrow.arrowColor
        strokeWidth: arrow.arrowWidth
        capStyle: ShapePath.RoundCap
        joinStyle: ShapePath.RoundJoin
        
        startX: arrow.width / 2
        startY: 0
        
        PathLine { x: arrow.width; y: arrow.height }
        PathLine { x: arrow.width / 2; y: arrow.height * 0.6 }
        PathLine { x: 0; y: arrow.height }
        PathLine { x: arrow.width / 2; y: 0 }
    }
    
    transform: Rotation {
        origin.x: arrow.width / 2
        origin.y: arrow.height / 2
        angle: arrow.rotation
    }
}