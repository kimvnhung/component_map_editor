#include <QtTest>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QMetaObject>

#include "componentmapeditormanager.h"

class EditorQmlModuleContractTest : public QObject
{
    Q_OBJECT

private slots:
    void moduleDeclaresReusableEditorTypes();
    void catalogDescriptorDrivesQmlComponentModel();
};

void EditorQmlModuleContractTest::moduleDeclaresReusableEditorTypes()
{
    ComponentMapEditorManager manager;
    QQmlEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("editorManager"), &manager);

    QQmlComponent component(&engine);
    component.setData(R"QML(
        import QtQuick
        import ComponentMapEditor

        Item {
            property alias graphModel: graph
            property alias editorController: controller

            GraphModel { id: graph }
            TypeRegistry { id: registry }
            UndoStack { id: undoStack }
            GraphEditorController {
                id: controller
                graph: graph
                undoStack: undoStack
                typeRegistry: registry
            }
            GraphCanvas {
                width: 640
                height: 480
                graph: graph
                componentTypeRegistry: registry
            }
            Palette {
                width: 220
                height: 480
                graph: graph
                componentTypeRegistry: registry
            }
            PropertyPanel {
                width: 320
                height: 480
                graph: graph
                propertySchemaRegistry: editorManager.propertySchemaRegistry
                dynamicOptionsRegistry: editorManager.dynamicOptionsRegistry
            }
            ExecutionPanel {
                width: 320
                height: 480
            }
        }
    )QML", QUrl(QStringLiteral("qrc:/editor-module-contract.qml")));

    QObject *root = component.create();
    QVERIFY2(root, qPrintable(component.errorString()));

    auto *controller = root->property("editorController").value<QObject *>();
    QVERIFY(controller);
    QString createdId;
    QVERIFY(QMetaObject::invokeMethod(controller,
                                      "createComponentWithId",
                                      Qt::DirectConnection,
                                      Q_RETURN_ARG(QString, createdId),
                                      Q_ARG(QString, QStringLiteral("contract_instance")),
                                      Q_ARG(QString, QStringLiteral("contract_type")),
                                      Q_ARG(qreal, 32.0),
                                      Q_ARG(qreal, 48.0)));
    QCOMPARE(createdId, QStringLiteral("contract_instance"));

    auto *graph = root->property("graphModel").value<QObject *>();
    QVERIFY(graph);
    QCOMPARE(graph->property("componentCount").toInt(), 1);
    delete root;
}

void EditorQmlModuleContractTest::catalogDescriptorDrivesQmlComponentModel()
{
    ComponentMapEditorManager manager;
    QQmlEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("editorManager"), &manager);

    QVariantMap descriptor{
        {QStringLiteral("identity"), QStringLiteral("runtime.qml.transform")},
        {QStringLiteral("version"), QStringLiteral("3.0.0")},
        {QStringLiteral("properties"), QVariantList{
            QVariantMap{{QStringLiteral("id"), QStringLiteral("enabled")},
                        {QStringLiteral("type"), QStringLiteral("boolean")},
                        {QStringLiteral("required"), true}}
        }},
        {QStringLiteral("ports"), QVariantList{
            QVariantMap{{QStringLiteral("id"), QStringLiteral("input")},
                        {QStringLiteral("direction"), QStringLiteral("input")},
                        {QStringLiteral("payloadType"), QStringLiteral("event")}}
        }},
        {QStringLiteral("execution"), QVariantMap{{QStringLiteral("mode"), 2}}}
    };
    manager.componentTypeRegistry()->replaceFromRuntimeCatalog({descriptor});
    manager.propertySchemaRegistry()->replaceFromRuntimeCatalog({descriptor});

    QQmlComponent component(&engine);
    component.setData(R"QML(
        import QtQuick
        import ComponentMapEditor
        Item {
            ComponentModel {
                id: node
                objectName: "catalogNode"
            }
            Component.onCompleted: node.applyRuntimeDescriptor({
                identity: "runtime.qml.transform",
                version: "3.0.0",
                properties: [{id: "enabled", type: "boolean", required: true}],
                ports: [{id: "input", direction: "input", payloadType: "event"}],
                execution: {mode: 2}
            })
        }
    )QML", QUrl(QStringLiteral("qrc:/runtime-catalog-model.qml")));

    QObject *root = component.create();
    QVERIFY2(root, qPrintable(component.errorString()));
    auto *node = root->findChild<QObject *>(QStringLiteral("catalogNode"));
    QVERIFY(node);
    QCOMPARE(node->property("componentIdentity").toString(), QStringLiteral("runtime.qml.transform"));
    QCOMPARE(node->property("componentVersion").toString(), QStringLiteral("3.0.0"));
    QCOMPARE(node->property("properties").toList().size(), 1);
    QCOMPARE(node->property("ports").toList().size(), 1);
    delete root;
}

QTEST_MAIN(EditorQmlModuleContractTest)
#include "tst_EditorQmlModuleContract.moc"
