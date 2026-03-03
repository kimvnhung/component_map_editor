find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2)

qt_add_executable(example_app
    main.cpp
)

qt_add_qml_module(example_app
    URI ExampleApp
    VERSION 1.0
    QML_FILES
        main.qml
)

# Link against the JobDesigner backing library and its generated QML plugin.
target_link_libraries(example_app
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
        jobdesigner
        jobdesignerplugin
)
