#include <QtTest>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QMetaObject>

#include "componentmapeditormanager.h"

class EditorQmlModuleContractTest : public QObject
{
    Q_OBJECT

private slots:
    void moduleDeclaresReusableEditorTypes();
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

QTEST_MAIN(EditorQmlModuleContractTest)
#include "tst_EditorQmlModuleContract.moc"
