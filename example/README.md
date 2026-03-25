# Component Map Editor — Example & Integration Guide

This directory contains a fully-featured reference application that demonstrates
every capability of the **ComponentMapEditor** library.  Read this guide from top
to bottom to understand how to embed the library in your own Qt/QML project.

---

## Table of Contents

1. [Getting Started](#1-getting-started)
   - [Prerequisites](#prerequisites)
   - [CMake Integration](#cmake-integration)
   - [C++ Bootstrap](#c-bootstrap)
   - [Minimal QML Window](#minimal-qml-window)
2. [Feature: Graph Model](#2-feature-graph-model)
3. [Feature: Graph Canvas](#3-feature-graph-canvas)
4. [Feature: Palette](#4-feature-palette)
5. [Feature: Property Panel & Schema Forms](#5-feature-property-panel--schema-forms)
6. [Feature: Undo / Redo](#6-feature-undo--redo)
7. [Feature: Validation](#7-feature-validation)
8. [Feature: Export & Import](#8-feature-export--import)
9. [Feature: Extension SDK](#9-feature-extension-sdk)
   - [Manifest Format](#manifest-format)
   - [Component Type Provider](#component-type-provider)
   - [Connection Policy Provider](#connection-policy-provider)
   - [Validation Provider](#validation-provider)
   - [Property Schema Provider](#property-schema-provider)
   - [Execution Semantics Provider](#execution-semantics-provider)
   - [Assembling Your Pack](#assembling-your-pack)
10. [Feature: Rule Engine & Hot-Reload](#10-feature-rule-engine--hot-reload)
11. [Feature: Execution Semantics](#11-feature-execution-semantics)
12. [Feature: Compatibility Checker CLI](#12-feature-compatibility-checker-cli)
13. [Feature: Performance Telemetry & Benchmarking](#13-feature-performance-telemetry--benchmarking)
14. [Architecture Overview](#14-architecture-overview)

---

## 1. Getting Started

### Prerequisites

| Requirement | Version |
|-------------|---------|
| Qt          | 6.6 or later (6.9 recommended) |
| CMake       | 3.22 or later |
| C++         | 17 |
| QML modules | `QtQuick`, `QtQuick.Controls`, `QtQuick.Layouts` |

---

### CMake Integration

The library is compiled as `libComponentMapEditor` and exposes the QML module
`ComponentMapEditor`.  In your `CMakeLists.txt`, link it like any other Qt
library:

```cmake
cmake_minimum_required(VERSION 3.22)
project(MyApp LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2)

# --- The library itself (add_subdirectory or find_package) ----------------
add_subdirectory(path/to/component_map_editor)

# --- Your application executable ------------------------------------------
qt_add_executable(my_app main.cpp)

qt_add_qml_module(my_app
    URI MyApp
    VERSION 1.0
    QML_FILES Main.qml
)

target_link_libraries(my_app PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    libComponentMapEditor        # ← the library target
)

target_include_directories(my_app PRIVATE
    "${CMAKE_SOURCE_DIR}/path/to/component_map_editor"
)
```

If your application uses extension packs or rule files you need to copy them
alongside the binary at configure time:

```cmake
# Copy your extension manifest directory next to the binary
set(MANIFEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/manifests")
file(COPY manifests/ DESTINATION "${MANIFEST_DIR}")
target_compile_definitions(my_app PRIVATE
    MY_EXTENSION_MANIFEST_DIR="${MANIFEST_DIR}"
)

# Copy your rule file
configure_file(rules/my_rules.json
    "${CMAKE_CURRENT_BINARY_DIR}/my_rules.json" COPYONLY)
target_compile_definitions(my_app PRIVATE
    MY_RULE_FILE="${CMAKE_CURRENT_BINARY_DIR}/my_rules.json"
)
```

---

### C++ Bootstrap

`main.cpp` in this directory is the canonical bootstrap.  The minimum viable
version that enables all library features:

```cpp
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

// Extension system
#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/runtime/ExtensionStartupLoader.h"
#include "extensions/runtime/PropertySchemaRegistry.h"
#include "extensions/runtime/rules/RuleRuntimeRegistry.h"
#include "extensions/runtime/rules/RuleBackedProviders.h"
#include "extensions/runtime/rules/RuleHotReloadService.h"

// Your custom pack (optional)
#include "extensions/sample_pack/SampleExtensionPack.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    // 1. Create the contract registry with the current core API version.
    ExtensionContractRegistry extensionContracts({1, 0, 0});

    // 2. Set up the rule-backed providers (connection policy + validation)
    //    that read their logic from a JSON rule file at runtime.
    RuleRuntimeRegistry ruleRegistry;
    RuleBackedConnectionPolicyProvider compiledConnectionPolicy(&ruleRegistry);
    RuleBackedValidationProvider       compiledValidation(&ruleRegistry);

    QString providerError;
    extensionContracts.registerConnectionPolicyProvider(&compiledConnectionPolicy, &providerError);
    extensionContracts.registerValidationProvider(&compiledValidation, &providerError);

    // 3. Start watching the rule file – changes are picked up without restart.
    const QString ruleFilePath = QString::fromUtf8(MY_RULE_FILE);
    RuleHotReloadService ruleHotReload(&ruleRegistry);
    ruleHotReload.startWatchingFile(ruleFilePath);

    // 4. Discover and load extension packs from the manifest directory.
    const QString manifestDir = QString::fromUtf8(MY_EXTENSION_MANIFEST_DIR);
    ExtensionStartupLoader startupLoader;
    startupLoader.registerFactory("sample.workflow", []() {
        return std::make_unique<SampleExtensionPack>();
    });
    startupLoader.loadFromDirectory(manifestDir, extensionContracts);

    // 5. Build the property-schema cache and expose it to QML.
    PropertySchemaRegistry propertySchemas;
    propertySchemas.rebuildFromRegistry(extensionContracts);
    engine.rootContext()->setContextProperty(
        "startupPropertySchemaRegistry", &propertySchemas);

    // 6. Load the root QML document.
    engine.loadFromModule("MyApp", "Main");
    return app.exec();
}
```

> **Why expose `PropertySchemaRegistry` to QML?**  
> `PropertyPanel` uses it to discover which fields should appear for a selected
> component type.  Without it, the panel falls back to a generic title/content
> form.

---

### Minimal QML Window

Once the C++ side is done, the smallest working editor is a single QML file:

```qml
import QtQuick
import QtQuick.Window
import ComponentMapEditor           // ← the library's QML module

ApplicationWindow {
    width: 800; height: 600
    visible: true
    title: "My Graph Editor"

    GraphModel { id: graph }

    GraphCanvas {
        anchors.fill: parent
        graph: graph
    }
}
```

This gives you a fully interactive pan/zoom canvas where components can be
dragged from the built-in palette and connected by dragging between ports.

---

## 2. Feature: Graph Model

**Why useful:** `GraphModel` is the central data store.  Every other part of the
library (canvas, property panel, export service, validation service) reads from
and writes to this single object.  It emits fine-grained signals so the QML
canvas can respond incrementally instead of re-rendering the entire scene.

### Core classes

| Class | Purpose |
|-------|---------|
| `GraphModel` | Root model; owns lists of `ComponentModel` and `ConnectionModel` |
| `ComponentModel` | A single node (position, size, title, colour, type …) |
| `ConnectionModel` | An edge between two component IDs, with optional label and port hints |

### Adding components programmatically (QML)

```qml
GraphModel {
    id: graph

    Component.onCompleted: {
        // Create a component object inside graph's ownership scope
        var node = Qt.createQmlObject(
            'import ComponentMapEditor; ComponentModel {}', graph)

        node.id    = "node-1"
        node.title = "Start"
        node.x     = 200
        node.y     = 150
        node.color = "#26a69a"   // any CSS colour string
        node.type  = "start"     // matched against your extension pack types

        // Schema-defined runtime fields are dynamic properties.
        // Use the explicit QML-facing API instead of direct assignment.
        node.setDynamicProperty("inputNumber", 12)

        graph.addComponent(node)
    }
}
```

For schema-defined runtime fields such as `inputNumber` or `addValue`, prefer
`setDynamicProperty(...)` in QML:

```qml
var processNode = Qt.createQmlObject(
    'import ComponentMapEditor; ComponentModel {}', graph)

processNode.id = "node-2"
processNode.title = "Process"
processNode.type = "process"
processNode.setDynamicProperty("addValue", 9)

graph.addComponent(processNode)
```

### Adding components from C++

```cpp
// ComponentModel constructor: (id, title, x, y, color, type, parent)
auto *node = new ComponentModel("n1", "Start", 200, 150,
                                "#26a69a", "start", &graphModel);
graphModel.addComponent(node);
```

### Adding connections (QML)

```qml
var edge = Qt.createQmlObject(
    'import ComponentMapEditor; ConnectionModel {}', graph)

edge.id       = "e1"
edge.sourceId = "node-1"    // must match an existing component id
edge.targetId = "node-2"
edge.label    = "on success"

// Optional: fix which port the arrow exits/enters from
edge.sourceSide = ConnectionModel.SideRight   // SideAuto | SideTop | SideRight | SideBottom | SideLeft
edge.targetSide = ConnectionModel.SideLeft

graph.addConnection(edge)
```

### Querying the model

```cpp
// From C++
int total = graphModel.componentCount();          // fast – O(1)
auto *node = graphModel.componentById("n1");      // O(1) hash lookup
const auto &list = graphModel.componentList();    // QList<ComponentModel*>
bool ok = graphModel.removeComponent("n1");       // also fires componentRemoved signal
graphModel.clear();                               // removes everything
```

### Batch updates for large graphs

When loading or generating hundreds of nodes at once, wrap the operations in a
batch to suppress intermediate signals:

```cpp
graphModel.beginBatchUpdate();
for (int i = 0; i < 500; ++i) {
    auto *n = new ComponentModel(QString("n%1").arg(i), "Node",
                                 i * 120, 0, "#4fc3f7", "default", &graphModel);
    graphModel.addComponent(n);
}
graphModel.endBatchUpdate();   // emits a single componentsChanged signal
```

---

## 3. Feature: Graph Canvas

**Why useful:** `GraphCanvas` is the interactive editing surface.  It handles
pan, zoom, single- and multi-select, drag-to-move, connection drawing,
context menus, and keyboard shortcuts — so you don't have to implement any of
that yourself.

### Basic wiring

```qml
import ComponentMapEditor

GraphCanvas {
    id: canvas
    anchors.fill: parent

    graph: graph             // required – a GraphModel instance
    undoStack: undoStack     // optional – enables undo/redo for user edits
    telemetry: perfTelemetry // optional – attaches performance sampling
}
```

### Responding to selection

```qml
GraphCanvas {
    // ...

    onComponentSelected: function(component) {
        console.log("Selected:", component.id, component.title)
        propertyPanel.component = component
        propertyPanel.connection = null
    }

    onConnectionSelected: function(connection) {
        console.log("Edge:", connection.sourceId, "→", connection.targetId)
        propertyPanel.connection = connection
        propertyPanel.component = null
    }

    onBackgroundClicked: {
        propertyPanel.component = null
        propertyPanel.connection = null
    }
}
```

### Camera control properties

```qml
GraphCanvas {
    zoom: 1.5          // current zoom level (default: 1.0)
    minZoom: 0.35      // minimum zoom (default: 0.35)
    maxZoom: 3.0       // maximum zoom (default: 3.0)
    panX: 0            // horizontal pan offset in screen pixels
    panY: 0            // vertical pan offset in screen pixels
}
```

### Listening to camera changes

```qml
GraphCanvas {
    onViewTransformChanged: function(panX, panY, zoom) {
        console.log("Camera moved – zoom:", zoom, "pan:", panX, panY)
    }
}
```

### Resetting the canvas

```qml
Button {
    text: "Clear All"
    onClicked: {
        canvas.resetAllState()   // clears selection, temp connection, etc.
        graph.clear()
        undoStack.clear()
    }
}
```

### Multi-select

Hold **Ctrl** while clicking components to add them to the selection.
`canvas.selectedComponentIds` is a list of the currently selected IDs.

---

## 4. Feature: Palette

**Why useful:** The `Palette` component provides a drag-and-drop sidebar listing
all component types registered by the loaded extension pack.  Users drag a tile
onto the canvas to create a new node — no code required on your side.

```qml
import ComponentMapEditor

RowLayout {
    Palette {
        Layout.preferredWidth: 150
        Layout.fillHeight: true

        graph: graph         // writes new components here
        undoStack: undoStack // so creation is undoable
        canvas: canvas       // needed to convert drop position to world coords
    }

    GraphCanvas {
        id: canvas
        Layout.fillWidth: true
        Layout.fillHeight: true
        graph: graph
        undoStack: undoStack
    }
}
```

The tiles shown in the palette come from the `IComponentTypeProvider`
registered by your extension pack (see [Section 9](#9-feature-extension-sdk)).
Without a pack the palette is empty.

---

## 5. Feature: Property Panel & Schema Forms

**Why useful:** When the user selects a node or edge, `PropertyPanel` displays
an editable form.  The fields are **generated automatically** from the property
schema registered by your extension pack, so you never write form code for each
type.

### Basic usage

```qml
import ComponentMapEditor

PropertyPanel {
    id: propertyPanel
    Layout.preferredWidth: 210
    Layout.fillHeight: true

    undoStack: undoStack
    // The registry built in main.cpp and exposed via setContextProperty():
    propertySchemaRegistry: startupPropertySchemaRegistry

    // Set these from canvas selection signals:
    // propertyPanel.component = selectedNode
    // propertyPanel.connection = selectedEdge
}
```

### Querying schemas from C++

```cpp
PropertySchemaRegistry reg;
reg.rebuildFromRegistry(extensionContracts);

// All fields for a component type
QVariantList fields = reg.componentSchema("process");

// Grouped by section for rendering a collapsible form
QVariantList sections = reg.sectionedSchemaForTarget("component/process");
```

### Querying schemas from QML

```qml
// startupPropertySchemaRegistry is the C++ context property
var fields = startupPropertySchemaRegistry.componentSchema("process")

// Check whether any schema exists for this type
if (startupPropertySchemaRegistry.hasTarget("component/process")) {
    // render fields...
}
```

`SchemaFormRenderer.qml` (used internally by `PropertyPanel`) turns the
`QVariantList` of field descriptors into live input widgets with two-way
binding.  Each field descriptor map supports the following keys:

| Key | Type | Description |
|-----|------|-------------|
| `key` | string | Property name on the model |
| `type` | string | `"string"`, `"int"`, `"bool"`, `"enum"`, `"color"` |
| `title` | string | Label shown in the UI |
| `required` | bool | Whether the field must have a value |
| `defaultValue` | any | Fallback when the property is not set |
| `section` | string | Groups fields under a collapsible header |
| `order` | int | Sort order within a section |
| `hint` | string | Placeholder / tooltip text |
| `options` | list | For `"enum"` type: list of `{ value, label }` maps |

---

## 6. Feature: Undo / Redo

**Why useful:** Every destructive edit — adding, removing, moving, or changing a
property — can be wrapped in an undoable command.  `UndoStack` exposes a clean
QML-friendly API; the canvas and palette wire it automatically when you pass it
in.

### Declaring the stack

```qml
UndoStack { id: undoStack }
```

### Toolbar buttons

```qml
ToolButton {
    text: "⟵ Undo"
    enabled: undoStack.canUndo
    ToolTip.text: undoStack.undoText   // e.g. "Undo Move Component"
    onClicked: undoStack.undo()
}
ToolButton {
    text: "Redo ⟶"
    enabled: undoStack.canRedo
    ToolTip.text: undoStack.redoText
    onClicked: undoStack.redo()
}
```

### Programmatic pushes (QML)

```qml
// Record a property change so it can be undone
undoStack.pushSetComponentProperty(component, "title", newValue)

// Record a move
undoStack.pushMoveComponent(graph, component.id,
                            oldX, oldY, newX, newY)

// Record adding a component
undoStack.pushAddComponent(graph, newComponent)

// Record removing a component
undoStack.pushRemoveComponent(graph, component.id)
```

### Resetting

```qml
undoStack.clear()   // called when loading a new graph
```

---

## 7. Feature: Validation

**Why useful:** `ValidationService` runs all registered `IValidationProvider`
implementations and returns a human-readable list of errors (e.g. "Graph must
contain exactly one start component").

### Using `ValidationService` in QML

```qml
ValidationService { id: validator }

Button {
    text: "Validate"
    onClicked: {
        var errors = validator.validationErrors(graph)
        if (errors.length === 0) {
            statusLabel.text = "✓ Graph is valid"
        } else {
            // errors is a QStringList – join for display
            statusLabel.text = "✗ " + errors.join("  |  ")
        }
    }
}
```

```qml
// Boolean check (no error list)
var isOk = validator.validate(graph)
```

### Writing a custom provider in C++

Implement `IValidationProvider` and register it with the contract registry:

```cpp
#include "extensions/contracts/IValidationProvider.h"

class MyValidationProvider : public IValidationProvider
{
public:
    QString providerId() const override { return "my.validation"; }

    QVariantList validateGraph(const QVariantMap &snapshot) const override {
        QVariantList issues;

        auto components = snapshot["components"].toList();
        if (components.isEmpty()) {
            issues << QVariantMap{
                {"code",       "E001"},
                {"severity",   "error"},
                {"message",    "Graph has no components."},
                {"entityType", "graph"},
                {"entityId",   ""},
            };
        }
        return issues;
    }
};
```

See also [Section 10](#10-feature-rule-engine--hot-reload) for rule-file–driven
validation that avoids recompiling when rules change.

---

## 8. Feature: Export & Import

**Why useful:** `ExportService` serialises the full graph to a JSON string and
deserialises it back, making persistence and copy-paste trivial.

### Exporting

```qml
ExportService { id: exporter }

Button {
    text: "Export JSON"
    onClicked: {
        var json = exporter.exportToJson(graph)
        console.log(json)          // write to file, clipboard, etc.
    }
}
```

### Importing

```qml
Button {
    text: "Import JSON"
    onClicked: {
        canvas.resetAllState()
        var ok = exporter.importFromJson(graph, jsonString)
        if (!ok) {
            statusLabel.text = "Import failed – invalid JSON"
        }
    }
}
```

The JSON schema is forward-compatible: the `GraphJsonMigration` service
(used internally) upgrades legacy v0 files to the current format
automatically.

---

## 9. Feature: Extension SDK

**Why useful:** The extension SDK lets you ship domain-specific knowledge
(component types, connection rules, property schemas, graph validation, execution
semantics) as isolated "packs" without touching library source code.  Multiple
packs can co-exist in one application.

### Manifest Format

Each pack requires a manifest file (JSON) placed in a directory that the
`ExtensionStartupLoader` scans at startup:

```json
{
  "extensionId":      "my.domain",
  "displayName":      "My Domain Pack",
  "extensionVersion": "1.0.0",
  "minCoreApi":       "1.0.0",
  "maxCoreApi":       "1.99.99",
  "capabilities":     [
    "componentTypes",
    "connectionPolicy",
    "propertySchema",
    "validation",
    "actions"
  ],
  "dependencies": [],
  "metadata": {
    "owner": "my-team",
    "purpose": "domain-specific graph types"
  }
}
```

`minCoreApi` / `maxCoreApi` declare the semver range of the core library API
that this pack is compatible with.  The loader rejects packs outside the range
and logs a diagnostic.

---

### Component Type Provider

Tells the library (and `Palette`) what node types exist.

```cpp
#include "extensions/contracts/IComponentTypeProvider.h"

class MyComponentTypeProvider : public IComponentTypeProvider
{
public:
    QString providerId() const override { return "my.domain.components"; }

    QStringList componentTypeIds() const override {
        return { "start", "process", "stop" };
    }

    QVariantMap componentTypeDescriptor(const QString &id) const override {
        if (id == "process")
            return {{ "id", "process" }, { "title", "Process" }, { "category", "Flow" }};
        // ... other types
        return {};
    }

    QVariantMap defaultComponentProperties(const QString &id) const override {
        if (id == "start")
            return {{ "inputNumber", 0 }};
        if (id == "process")
            return {{ "addValue", 9 }, { "color", "#5c6bc0" }, { "shape", "rounded" }};
        return {};
    }
};
```

---

### Connection Policy Provider

Enforces which source→target pairs are valid connections.

```cpp
#include "extensions/contracts/IConnectionPolicyProvider.h"

class MyConnectionPolicyProvider : public IConnectionPolicyProvider
{
public:
    QString providerId() const override { return "my.domain.connections"; }

    bool canConnect(const QString &srcType, const QString &dstType,
                    const QVariantMap & /*context*/, QString *reason) const override
    {
        // start has output-only behavior; stop has input-only behavior.
        if (dstType == "start") return false;
        if (srcType == "stop")  return false;

        if (srcType == "start"   && dstType == "process") return true;
        if (srcType == "process" && dstType == "process") return true;
        if (srcType == "process" && dstType == "stop")    return true;

        if (reason)
            *reason = QString("Connection from '%1' to '%2' is not allowed.")
                          .arg(srcType, dstType);
        return false;
    }

    QVariantMap normalizeConnectionProperties(const QString &, const QString &,
                                              const QVariantMap &raw) const override {
        return raw;   // pass through; transform if needed
    }
};
```

---

### Validation Provider

Graph-level rules that run when the user presses **Validate**.

```cpp
#include "extensions/contracts/IValidationProvider.h"

class MyValidationProvider : public IValidationProvider
{
public:
    QString providerId() const override { return "my.domain.validation"; }

    QVariantList validateGraph(const QVariantMap &snapshot) const override {
        QVariantList issues;
        auto comps = snapshot["components"].toList();

        auto countOfType = [&](const QString &t) {
            return std::count_if(comps.begin(), comps.end(), [&](const QVariant &c) {
                return c.toMap()["type"].toString() == t;
            });
        };

        if (countOfType("start") != 1)
            issues << QVariantMap{{"code","W001"},{"severity","error"},
                                  {"message","Graph must have exactly one Start node."},
                                  {"entityType","graph"},{"entityId",""}};

        if (countOfType("stop") != 1)
            issues << QVariantMap{{"code","W002"},{"severity","error"},
                                  {"message","Graph must have exactly one Stop node."},
                                  {"entityType","graph"},{"entityId",""}};

        return issues;
    }
};
```

---

### Property Schema Provider

Declares which input fields appear in `PropertyPanel` for each component type.

```cpp
#include "extensions/contracts/IPropertySchemaProvider.h"

class MyPropertySchemaProvider : public IPropertySchemaProvider
{
public:
    QString providerId() const override { return "my.domain.schema"; }

    QStringList schemaTargets() const override {
        return { "component/start", "component/process", "component/stop", "connection/flow" };
    }

    QVariantList propertySchema(const QString &targetId) const override {
        if (targetId == "component/process") {
            return {
                QVariantMap{{ "key","title"   },{ "type","string" },{ "title","Process Name" },{ "order",1 },{ "section","General" }},
                QVariantMap{{ "key","addValue"},{ "type","int"    },{ "title","Add Value"    },{ "order",2 },{ "section","Behavior" }},
                QVariantMap{{ "key","color"   },{ "type","color"  },{ "title","Color"        },{ "order",3 },{ "section","Appearance" }},
            };
        }
        return {};
    }
};
```

---

### Execution Semantics Provider

Runs individual components in a sandbox during graph simulation.

```cpp
#include "extensions/contracts/IExecutionSemanticsProvider.h"

class MyExecutionProvider : public IExecutionSemanticsProvider
{
public:
    QString providerId() const override { return "my.domain.execution"; }

    QStringList supportedComponentTypes() const override {
        return { "start", "process", "stop" };
    }

    bool executeComponent(const QString &type, const QString &id,
                          const QVariantMap &snapshot,
                          const QVariantMap &inputState,
                          QVariantMap *outputState,
                          QVariantMap *trace,
                          QString *error) const override
    {
        if (type == "start") {
            (*outputState)["workingNumber"] = snapshot.value("inputNumber").toInt();
            (*trace)["log"] = QString("Start %1 seeded input.").arg(id);
            return true;
        }
        if (type == "process") {
            const int current = inputState.value("workingNumber", 0).toInt();
            const int addValue = snapshot.value("addValue", 9).toInt();
            const int result = current + addValue;
            (*outputState)["workingNumber"] = result;
            (*trace)["log"] = QString("Process %1: %2 + %3 = %4")
                                .arg(id)
                                .arg(current)
                                .arg(addValue)
                                .arg(result);
            return true;
        }
        if (type == "stop") {
            const int result = inputState.value("workingNumber", 0).toInt();
            (*outputState)["finalResult"] = result;
            (*trace)["log"] = QString("Stop %1 final result: %2").arg(id).arg(result);
            return true;
        }
        if (error) *error = "Unknown type: " + type;
        return false;
    }
};
```

---

### Assembling Your Pack

Combine all providers into one `IExtensionPack` class:

```cpp
#include "extensions/contracts/IExtensionPack.h"
#include "MyComponentTypeProvider.h"
#include "MyConnectionPolicyProvider.h"
#include "MyValidationProvider.h"
#include "MyPropertySchemaProvider.h"
#include "MyExecutionProvider.h"

class MyDomainPack : public IExtensionPack
{
public:
    bool registerProviders(ExtensionContractRegistry &registry,
                           QString *error) override
    {
        bool ok = true;
        ok &= registry.registerComponentTypeProvider(&m_componentTypes, error);
        ok &= registry.registerConnectionPolicyProvider(&m_connectionPolicy, error);
        ok &= registry.registerValidationProvider(&m_validation, error);
        ok &= registry.registerPropertySchemaProvider(&m_propertySchema, error);
        ok &= registry.registerExecutionSemanticsProvider(&m_execution, error);
        return ok;
    }

private:
    MyComponentTypeProvider      m_componentTypes;
    MyConnectionPolicyProvider   m_connectionPolicy;
    MyValidationProvider         m_validation;
    MyPropertySchemaProvider     m_propertySchema;
    MyExecutionProvider          m_execution;
};
```

Register the factory and load from the manifest directory in `main.cpp`:

```cpp
ExtensionStartupLoader loader;
loader.registerFactory("my.domain", []() {
    return std::make_unique<MyDomainPack>();
});
auto result = loader.loadFromDirectory(manifestDir, extensionContracts);

if (result.hasErrors()) {
    for (const auto &d : result.diagnostics) {
        qWarning() << d.message;
    }
}
qInfo() << "Loaded" << result.loadedPackCount << "pack(s)";
```

---

## 10. Feature: Rule Engine & Hot-Reload

**Why useful:** Instead of writing C++ for connection-policy and validation
logic, you can keep the rules in a JSON file.  The `RuleHotReloadService`
watches the file and recompiles rules the moment you save — no application
restart.  This is ideal for rapid iteration on business logic.

### JSON Rule File Format

```json
{
  "defaultConnectionAllow": false,

  "connections": [
    { "source": "start",   "target": "process", "allow": true },
    { "source": "process", "target": "process", "allow": true },
    { "source": "process", "target": "stop",    "allow": true }
  ],

  "validations": [
    {
      "kind":     "exactlyOneType",
      "type":     "start",
      "code":     "W001",
      "severity": "error",
      "message":  "Graph must contain exactly one start component."
    },
        {
            "kind":     "exactlyOneType",
            "type":     "stop",
            "code":     "W002",
            "severity": "error",
            "message":  "Graph must contain exactly one stop component."
        },
    {
      "kind":     "noIsolated",
      "code":     "W004",
      "severity": "warning",
      "message":  "Component has no connections."
    }
  ]
}
```

### Built-in rule kinds

| Kind | Effect |
|------|--------|
| `exactlyOneType` | Graph must contain exactly one component of the given type |
| `endpointExists` | Each connection's source/target must reference a real node |
| `noIsolated` | Warns when a component has no edges at all |

### Setting up the hot-reload pipeline

```cpp
RuleRuntimeRegistry ruleRegistry;

// Rule-backed providers read their logic from the registry
RuleBackedConnectionPolicyProvider connectionPolicy(&ruleRegistry);
RuleBackedValidationProvider       validation(&ruleRegistry);

extensionContracts.registerConnectionPolicyProvider(&connectionPolicy, nullptr);
extensionContracts.registerValidationProvider(&validation, nullptr);

// Watch the file — any save triggers recompile + registry hot-swap
RuleHotReloadService hotReload(&ruleRegistry);
hotReload.startWatchingFile("/path/to/my_rules.json");

// Force an immediate reload (useful on startup)
hotReload.reloadNow();

// Inspect compiler diagnostics if reload fails
for (const auto &diag : hotReload.lastDiagnostics()) {
    qWarning() << diag.message;
}
```

---

## 11. Feature: Execution Semantics

**Why useful:** Simulate or "run" a graph by stepping through each node and
collecting output state.  The semantics are entirely domain-defined: a workflow
engine might transform numbers through the pipeline: **start** seeds an input,
**process** adds a configured value (default $+9$), and **stop** logs the
final result.

The `IExecutionSemanticsProvider` interface (shown in Section 9) is the only
integration point.  The library calls it; your C++ code decides what execution
means.

### Workflow: Start -> Process -> Stop

The sample pack now models a simple numeric pipeline:

1. `start` stores `inputNumber` (for example `12`) via `setDynamicProperty("inputNumber", 12)` in QML.
2. `process` stores `addValue` (default `9`) via `setDynamicProperty("addValue", 9)` in QML.
3. `stop` stores `finalResult` in state and prints a log line.

For example, with `inputNumber = 12` and `addValue = 9`, the final output is
$12 + 9 = 21$.

### Execution Panel In The Example App

The example application now includes an `Execution` tab beside the normal
`Properties` inspector. It is backed by `GraphExecutionSandbox` and lets you
run the current graph with the loaded execution semantics providers.

Use the controls as follows:

1. `Start` resets the sandbox, captures the current graph snapshot, and prepares
    the ready queue without executing any node.
2. `Step` executes exactly one ready component and appends a timeline entry.
3. `Run` keeps executing until the graph completes, blocks, or hits an error.
4. `Reset` clears execution state, timeline entries, and per-component sandbox state.

The panel also shows:

1. Current sandbox `status` and `tick`.
2. A JSON summary of ready and executed nodes.
3. The global execution state map.
4. The selected component's execution state.
5. A readable execution timeline for each step.

With the seeded sample graph (`start -> process -> stop`), click `Start` and
then `Run` to demonstrate the full pipeline and confirm that the final result is `21`.

### Legacy V0 adapter

If you have an existing v0-era execution provider (`IExecutionSemanticsProviderV0`)
that returns a flat `QStringList` trace, wrap it without rewriting:

```cpp
#include "extensions/contracts/ExecutionSemanticsV0Adapter.h"
#include "MyLegacyProvider.h"

auto *legacy = new MyLegacyProvider();
auto *adapter = new ExecutionSemanticsV0Adapter(legacy);
extensionContracts.registerExecutionSemanticsProvider(adapter, nullptr);
```

The adapter converts the v0 string traces into the v1 `QVariantMap` format
automatically.

---

## 12. Feature: Compatibility Checker CLI

**Why useful:** When upgrading the core library, run the CLI tool before
touching a single line of code.  It loads all manifests in a directory, evaluates
their API version ranges against the new core version, and produces a JSON
report showing which packs will load cleanly and which need attention.

### Building & Running

The `compatibility_checker_tool` target is already in `CMakeLists.txt`:

```bash
cmake --build build --target compatibility_checker_tool
./build/compatibility_checker_tool --core-version 1.2.0 --manifest-dir ./manifests
```

### Example output

```json
{
  "coreVersion": "1.2.0",
  "manifestDirectory": "./manifests",
  "results": [
    {
      "extensionId":   "my.domain",
      "manifestPath":  "./manifests/manifest.my.domain.json",
      "status":        "Compatible",
      "message":       "Extension is compatible with core 1.2.0.",
      "minCoreApi":    "1.0.0",
      "maxCoreApi":    "1.99.99"
    }
  ],
  "summary": {
    "total": 1,
    "compatible": 1,
    "incompatible": 0,
    "warnings": 0
  }
}
```

### Status values

| Status | Meaning |
|--------|---------|
| `Compatible` | Pack loads cleanly on this core version |
| `CoreTooOld` | Core must be upgraded before this pack runs |
| `CoreTooNewMajor` | Pack was built for an older major API; update the pack |
| `OutsideSupportedRange` | The core version falls outside `[minCoreApi, maxCoreApi]` |
| `InvalidInput` | The manifest contains a malformed version string |

---

## 13. Feature: Performance Telemetry & Benchmarking

**Why useful:** When dealing with large graphs (thousands of nodes), you need
hard numbers to make informed rendering decisions.  The telemetry system
gives you microsecond-resolution timings for camera updates, drag latency,
and frame intervals — directly in the QML layer, without any external profiler.

### Attaching telemetry

```qml
// Declare both telemetry objects at the window level
PerformanceTelemetry {
    id: perfTelemetry
    // Leave disabled by default; the BenchmarkPanel activates it on demand.
}

FrameSwapTelemetry {
    id: frameTelemetry
    window: window   // QQuickWindow reference for frameSwapped signal
}

// Pass perfTelemetry to the canvas
GraphCanvas {
    telemetry: perfTelemetry
    // ...
}
```

### BenchmarkPanel (optional UI)

Drop `BenchmarkPanel` below the canvas to get an interactive controls strip
with buttons to generate synthetic graphs (10 → 1 000 nodes), toggle telemetry
overlays, and display live frame-time histograms:

```qml
import ExampleApp   // BenchmarkPanel lives in the example module

BenchmarkPanel {
    Layout.fillWidth: true
    graph: graph
    appWindow: window
    canvas: canvas
    telemetry: perfTelemetry
    frameTelemetry: frameTelemetry
}
```

### Reading telemetry from QML

`PerformanceTelemetry` exposes sampled intervals that you can bind to `Text`
labels or custom charts:

```qml
Text {
    text: "Frame: " + perfTelemetry.lastFrameMs.toFixed(2) + " ms"
}
```

### PerformanceTelemetry overlay

`PerformanceTelemetry.qml` also renders an on-canvas overlay (heat-map colour
coded by frame budget) when activated.  Attach it on top of `GraphCanvas`:

```qml
PerformanceTelemetry {
    id: perfTelemetry
    anchors.top: canvas.top
    anchors.right: canvas.right
}
```

---

## 14. Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│  Your Application (QML)                                         │
│                                                                 │
│  ┌──────────┐  ┌─────────────┐  ┌─────────────┐  ┌──────────┐ │
│  │ Palette  │  │ GraphCanvas │  │PropertyPanel│  │  Toolbar │ │
│  └────┬─────┘  └──────┬──────┘  └──────┬──────┘  └──────────┘ │
│       │               │                │                        │
│       └───────────────┴────────────────┘                        │
│                        │                                        │
│               ┌─────────▼─────────┐                             │
│               │    GraphModel     │  ←→  UndoStack              │
│               │  ComponentModel   │                             │
│               │  ConnectionModel  │                             │
│               └─────────┬─────────┘                             │
│                         │                                        │
└─────────────────────────┼────────────────────────────────────────┘
                          │
┌─────────────────────────▼────────────────────────────────────────┐
│  Library Services (C++)                                           │
│                                                                   │
│  ExportService    ValidationService    GraphJsonMigration         │
│                                                                   │
├───────────────────────────────────────────────────────────────────┤
│  Extension SDK                                                    │
│                                                                   │
│  ExtensionContractRegistry                                        │
│    ├── IComponentTypeProvider                                     │
│    ├── IConnectionPolicyProvider  ←── RuleBackedProviders         │
│    ├── IValidationProvider        ←── (JSON rule engine)          │
│    ├── IPropertySchemaProvider                                    │
│    ├── IActionProvider                                            │
│    └── IExecutionSemanticsProvider                                │
│                                                                   │
│  ExtensionStartupLoader  ──── (manifest discovery + pack wiring) │
│  PropertySchemaRegistry  ──── (flattened schema cache for QML)   │
│  RuleHotReloadService    ──── (QFileSystemWatcher + recompile)    │
│  ExtensionCompatibilityChecker ── (semver range evaluation)       │
└───────────────────────────────────────────────────────────────────┘
```

**Data flow at a glance:**

1. `main.cpp` wires the extension registry, rule engine, and hot-reload service,
   then exposes `PropertySchemaRegistry` as a QML context property.
2. QML creates a `GraphModel` as the single source of truth.
3. `GraphCanvas` renders the model and fires `componentSelected` /
   `connectionSelected` signals back up to the application.
4. `PropertyPanel` uses `PropertySchemaRegistry` (from the context property)
   to determine which fields to show for the active element.
5. `ExportService` and `ValidationService` read the model directly; they are
   pure QML-invokable helpers with no side effects on the graph itself.
6. The extension SDK providers are called by the services at validation/export
   time — your C++ domain logic never reaches into the QML layer.
