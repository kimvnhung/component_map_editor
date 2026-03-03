# component_map_editor

A reusable **Qt / QML module** (`ComponentMapEditor`) for building interactive
graph / component-map editors.  The module exposes C++ models, services and
commands together with a set of ready-made QML components that can be dropped
into any Qt 6 CMake project.

---

## Project structure

```
componentmapeditor/                   ← reusable QML module (URI: ComponentMapEditor)
├── models/
│   ├── NodeModel              QML element – a graph node (id, label, x, y, color, type)
│   ├── EdgeModel              QML element – a directed edge (id, sourceId, targetId, label)
│   └── GraphModel             QML element – holds node + edge lists; CRUD methods
├── services/
│   ├── ValidationService      QML element – validates graph integrity
│   └── ExportService          QML element – JSON serialisation / deserialisation
├── ui/
│   ├── GraphCanvas.qml        Interactive canvas: grid, draggable nodes, edge rendering
│   ├── Node.qml               Draggable node card
│   ├── Edge.qml               Directed-edge Shape component
│   ├── Palette.qml            Sidebar for adding new node types
│   └── PropertyPanel.qml      Inspector panel for the selected node / edge
└── commands/
    ├── UndoStack               QML element — custom undo/redo stack (no Qt Widgets dependency)
    └── GraphCommands           C++ GraphCommand subclasses (Add/Remove/Move node & edge)

example/                       ← standalone demo application
```

---

## Requirements

| Dependency | Minimum version |
|------------|----------------|
| Qt         | 6.5            |
| CMake      | 3.21           |
| C++        | 17             |

---

## Building

```bash
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/Qt6
cmake --build build
```

The example application is built automatically as `build/example/example_app`.

---

## Using the module in your own project

### Option A – add_subdirectory

```cmake
# In your CMakeLists.txt
add_subdirectory(path/to/component_map_editor/componentmapeditor)

target_link_libraries(my_app PRIVATE componentmapeditor componentmapeditorplugin)
```

In `main.cpp` add:
```cpp
#include <QtPlugin>
Q_IMPORT_QML_PLUGIN(ComponentMapEditorPlugin)
```

Then in QML:
```qml
import ComponentMapEditor

GraphModel  { id: graph }
UndoStack   { id: undoStack }
GraphCanvas { graph: graph; undoStack: undoStack; anchors.fill: parent }
```

### Option B – install & find_package *(coming soon)*

---

## QML API summary

| Type               | Key properties / methods |
|--------------------|--------------------------|
| `NodeModel`        | `id`, `label`, `x`, `y`, `color`, `type` |
| `EdgeModel`        | `id`, `sourceId`, `targetId`, `label` |
| `GraphModel`       | `nodes`, `edges`, `addNode()`, `removeNode()`, `addEdge()`, `removeEdge()`, `nodeById()`, `edgeById()`, `clear()` |
| `ValidationService`| `validate(graph)`, `validationErrors(graph)` |
| `ExportService`    | `exportToJson(graph)`, `importFromJson(graph, json)` |
| `UndoStack`        | `canUndo`, `canRedo`, `undoText`, `redoText`, `undo()`, `redo()`, `clear()` |
| `GraphCanvas`      | `graph`, `undoStack`, `selectedNode`, `selectedEdge` |
| `Node`             | `node`, `selected`, `undoStack` |
| `Edge`             | `edge`, `sourceX/Y`, `targetX/Y`, `selected` |
| `Palette`          | `graph`, `undoStack` |
| `PropertyPanel`    | `node`, `edge` |

---

## License

MIT
