# component_map_editor

A reusable Qt/QML module (ComponentMapEditor) for building interactive graph or component-map editors.
The module exposes C++ models, services, and commands together with ready-made QML components.

## Project structure

component_map_editor/
- models/
  - ComponentModel: QML element for a graph component (id, label, x, y, color, type)
  - ConnectionModel: QML element for a directed connection (id, sourceId, targetId, label)
  - GraphModel: QML element holding component + connection lists and CRUD methods
- services/
  - ValidationService: validates graph integrity
  - ExportService: JSON serialization/deserialization
- ui/
  - GraphCanvas.qml: interactive canvas (grid, draggable components, connection rendering)
  - ComponentItem.qml: draggable component card delegate
  - Connection.qml: directed-connection Shape component
  - Palette.qml: sidebar for adding new component types
  - PropertyPanel.qml: inspector panel for selected component/connection
- commands/
  - UndoStack: custom undo/redo stack (no Qt Widgets dependency)
  - GraphCommands: GraphCommand subclasses (Add/Remove/Move component & connection)

example/
- standalone demo application

## Requirements

- Qt: 6.5+
- CMake: 3.21+
- C++: 17

## Build

cmake -B build -DCMAKE_PREFIX_PATH=/path/to/Qt6
cmake --build build

The example application is built as build/example/example_app.

## QML API summary

- ComponentModel: id, label, x, y, color, type
- ConnectionModel: id, sourceId, targetId, label
- GraphModel: components, connections, addComponent(), removeComponent(), addConnection(), removeConnection(), componentById(), connectionById(), clear()
- ValidationService: validate(graph), validationErrors(graph)
- ExportService: exportToJson(graph), importFromJson(graph, json)
- UndoStack: canUndo, canRedo, undoText, redoText, undo(), redo(), clear()
- GraphCanvas: graph, undoStack, selectedComponent, selectedConnection
- ComponentItem: component, selected, undoStack
- Connection: connection, sourceX/Y, targetX/Y, selected
- Palette: graph, undoStack
- PropertyPanel: component, connection

## Naming note

- The project intentionally avoids creating a QML type named Component because QtQuick already provides a built-in Component type.
- The visual delegate is named ComponentItem to avoid ambiguity, while model naming uses ComponentModel.

## License

MIT
