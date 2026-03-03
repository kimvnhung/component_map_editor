find_package(Qt6 REQUIRED COMPONENTS Core Quick)

qt_add_library(jobdesigner STATIC)

qt_add_qml_module(jobdesigner
    URI JobDesigner
    VERSION 1.0
    QML_FILES
        ui/GraphCanvas.qml
        ui/Node.qml
        ui/Edge.qml
        ui/Palette.qml
        ui/PropertyPanel.qml
    SOURCES
        models/NodeModel.h     models/NodeModel.cpp
        models/EdgeModel.h     models/EdgeModel.cpp
        models/GraphModel.h    models/GraphModel.cpp
        services/ValidationService.h  services/ValidationService.cpp
        services/ExportService.h      services/ExportService.cpp
        commands/GraphCommand.h
        commands/UndoStack.h   commands/UndoStack.cpp
        commands/GraphCommands.h commands/GraphCommands.cpp
)

target_include_directories(jobdesigner
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/jobdesigner>
)

target_link_libraries(jobdesigner
    PUBLIC Qt6::Core Qt6::Quick
)
