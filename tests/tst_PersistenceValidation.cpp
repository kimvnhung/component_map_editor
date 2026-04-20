#include <QtTest/QtTest>

#include <QJsonDocument>

#include "extensions/sample_pack/SampleValidationProvider.h"
#include "ExportService.h"
#include "ValidationService.h"

namespace {

bool containsFragment(const QStringList &errors, const QString &fragment)
{
    for (const QString &error : errors) {
        if (error.contains(fragment))
            return true;
    }
    return false;
}

} // namespace

class PersistenceValidationTests : public QObject
{
    Q_OBJECT

private slots:
    void roundTripIsStable();
    void roundTripPreservesDynamicProperties();
    void importsLegacyTopLeftV2();
    void importsLegacyYUpWithoutCoordinateSystem();
    void componentModelAcceptsDynamicProperties();
    void validationCatchesExpectedSchemaErrors();
    void validationRequiresExactlyOneStartAndStop();
    void validationRejectsDisconnectedWorkflow();
};

void PersistenceValidationTests::roundTripIsStable()
{
    GraphModel source;

    auto *componentA = new ComponentModel(QStringLiteral("A"),
                                          QStringLiteral("Input"),
                                          120.0,
                                          -45.0,
                                          QStringLiteral("#ff7043"),
                                          QStringLiteral("io"));
    componentA->setContent(QStringLiteral("Source"));
    componentA->setIcon(QStringLiteral("arrow-right"));
    componentA->setWidth(140.0);
    componentA->setHeight(80.0);
    componentA->setShape(QStringLiteral("rectangle"));
    source.addComponent(componentA);

    auto *componentB = new ComponentModel(QStringLiteral("B"),
                                          QStringLiteral("Output"),
                                          420.0,
                                          95.0,
                                          QStringLiteral("#4fc3f7"),
                                          QStringLiteral("default"));
    componentB->setContent(QStringLiteral("Sink"));
    componentB->setIcon(QStringLiteral("cube"));
    source.addComponent(componentB);

    auto *connection = new ConnectionModel(QStringLiteral("E1"),
                                           QStringLiteral("A"),
                                           QStringLiteral("B"),
                                           QStringLiteral("main"));
    connection->setTokenKey(QStringLiteral("tok.A.B"));
    connection->setSourceSide(ConnectionModel::SideRight);
    connection->setTargetSide(ConnectionModel::SideLeft);
    source.addConnection(connection);

    ExportService persistence;
    const QString jsonBefore = persistence.exportToJson(&source);

    GraphModel imported;
    QVERIFY(persistence.importFromJson(&imported, jsonBefore));

    ConnectionModel *importedConnection = imported.connectionById(QStringLiteral("E1"));
    QVERIFY(importedConnection != nullptr);
    QCOMPARE(importedConnection->tokenKey(), QStringLiteral("tok.A.B"));

    const QString jsonAfter = persistence.exportToJson(&imported);
    const QByteArray canonicalBefore = QJsonDocument::fromJson(jsonBefore.toUtf8())
                                           .toJson(QJsonDocument::Compact);
    const QByteArray canonicalAfter = QJsonDocument::fromJson(jsonAfter.toUtf8())
                                          .toJson(QJsonDocument::Compact);
    QCOMPARE(canonicalBefore, canonicalAfter);
}

void PersistenceValidationTests::importsLegacyTopLeftV2()
{
    const QString json = QStringLiteral(R"json({
        "coordinateSystem": "world-top-left-y-down-v2",
        "components": [
            {
                "id": "A",
                "title": "Legacy",
                "x": 10,
                "y": 20,
                "width": 100,
                "height": 50,
                "color": "#4fc3f7",
                "type": "default"
            }
        ],
        "connections": []
    })json");

    GraphModel graph;
    ExportService persistence;
    QVERIFY(persistence.importFromJson(&graph, json));

    ComponentModel *component = graph.componentById(QStringLiteral("A"));
    QVERIFY(component != nullptr);
    QCOMPARE(component->x(), 60.0);
    QCOMPARE(component->y(), 45.0);
}

void PersistenceValidationTests::importsLegacyYUpWithoutCoordinateSystem()
{
    const QString json = QStringLiteral(R"json({
        "components": [
            {
                "id": "A",
                "title": "LegacyYUp",
                "x": 12,
                "y": 30,
                "width": 96,
                "height": 96,
                "color": "#4fc3f7",
                "type": "default"
            }
        ],
        "connections": []
    })json");

    GraphModel graph;
    ExportService persistence;
    QVERIFY(persistence.importFromJson(&graph, json));

    ComponentModel *component = graph.componentById(QStringLiteral("A"));
    QVERIFY(component != nullptr);
    QCOMPARE(component->x(), 12.0);
    QCOMPARE(component->y(), -30.0);
}

void PersistenceValidationTests::componentModelAcceptsDynamicProperties()
{
    ComponentModel component;
    component.setDynamicProperty(QStringLiteral("inputNumber"), 12);
    component.setDynamicProperty(QStringLiteral("addValue"), 9);

    QCOMPARE(component.dynamicPropertyValue(QStringLiteral("inputNumber")).toInt(), 12);
    QCOMPARE(component.dynamicPropertyValue(QStringLiteral("addValue")).toInt(), 9);
}

void PersistenceValidationTests::roundTripPreservesDynamicProperties()
{
    // Verifies that schema-driven properties set via PropertyPanel (stored as Qt
    // dynamic properties) survive a full export/import round-trip.
    GraphModel source;

    auto *start = new ComponentModel(QStringLiteral("S1"),
                                     QStringLiteral("Start"),
                                     0.0,
                                     0.0,
                                     QStringLiteral("#66bb6a"),
                                     QStringLiteral("start"));
    start->setProperty("inputNumber", 42);
    source.addComponent(start);

    auto *process = new ComponentModel(QStringLiteral("P1"),
                                       QStringLiteral("Process"),
                                       200.0,
                                       0.0,
                                       QStringLiteral("#4fc3f7"),
                                       QStringLiteral("process"));
    process->setProperty("addValue", 7);
    process->setProperty("description", QStringLiteral("applies +7"));
    source.addComponent(process);

    ExportService persistence;
    const QString json = persistence.exportToJson(&source);

    // Sanity: the exported JSON must contain the dynamic property values.
    QVERIFY(json.contains(QStringLiteral("\"properties\"")));
    QVERIFY(json.contains(QStringLiteral("42")));
    QVERIFY(json.contains(QStringLiteral("7")));
    QVERIFY(json.contains(QStringLiteral("applies +7")));

    GraphModel imported;
    QVERIFY(persistence.importFromJson(&imported, json));

    const ComponentModel *importedStart = imported.componentById(QStringLiteral("S1"));
    QVERIFY(importedStart != nullptr);
    QCOMPARE(importedStart->property("inputNumber").toInt(), 42);

    const ComponentModel *importedProcess = imported.componentById(QStringLiteral("P1"));
    QVERIFY(importedProcess != nullptr);
    QCOMPARE(importedProcess->property("addValue").toInt(), 7);
    QCOMPARE(importedProcess->property("description").toString(), QStringLiteral("applies +7"));

    // Round-trip must be idempotent.
    const QString json2 = persistence.exportToJson(&imported);
    const QByteArray canonical1 = QJsonDocument::fromJson(json.toUtf8()).toJson(QJsonDocument::Compact);
    const QByteArray canonical2 = QJsonDocument::fromJson(json2.toUtf8()).toJson(QJsonDocument::Compact);
    QCOMPARE(canonical1, canonical2);
}

void PersistenceValidationTests::validationCatchesExpectedSchemaErrors()
{
    GraphModel graph;
    graph.beginBatchUpdate();

    auto *componentA = new ComponentModel(QStringLiteral("A"),
                                          QStringLiteral("ComponentA"),
                                          0.0,
                                          0.0);
    graph.addComponent(componentA);

    auto *duplicateComponentA = new ComponentModel(QStringLiteral("A"),
                                                   QStringLiteral("Duplicate"),
                                                   10.0,
                                                   10.0);
    graph.addComponent(duplicateComponentA);

    auto *connection = new ConnectionModel(QStringLiteral("E1"),
                                           QStringLiteral("A"),
                                           QStringLiteral("MissingTarget"),
                                           QStringLiteral("broken"));
    connection->setSourceSide(static_cast<ConnectionModel::Side>(99));
    connection->setTargetSide(static_cast<ConnectionModel::Side>(99));
    graph.addConnection(connection);
    graph.endBatchUpdate();

    SampleValidationProvider provider;
    ValidationService validation;
    validation.setValidationProviders({ &provider });
    const QStringList errors = validation.validationErrors(&graph);

    QVERIFY(containsFragment(errors, QStringLiteral("Duplicate component id: A")));
    QVERIFY(containsFragment(errors, QStringLiteral("references unknown target component 'MissingTarget'")));
    QVERIFY(containsFragment(errors, QStringLiteral("invalid sourceSide value 99")));
    QVERIFY(containsFragment(errors, QStringLiteral("invalid targetSide value 99")));
}

void PersistenceValidationTests::validationRequiresExactlyOneStartAndStop()
{
    GraphModel graph;

    auto *processOnly = new ComponentModel(QStringLiteral("P1"),
                                           QStringLiteral("Process"),
                                           0.0,
                                           0.0,
                                           QStringLiteral("#4fc3f7"),
                                           QStringLiteral("process"));
    graph.addComponent(processOnly);

    SampleValidationProvider provider;
    ValidationService validation;
    validation.setValidationProviders({ &provider });
    const QStringList missingErrors = validation.validationErrors(&graph);
    QVERIFY(containsFragment(missingErrors,
                             QStringLiteral("exactly one start component (found 0)")));
    QVERIFY(containsFragment(missingErrors,
                             QStringLiteral("exactly one stop component (found 0)")));

    auto *startA = new ComponentModel(QStringLiteral("S1"),
                                      QStringLiteral("Start A"),
                                      10.0,
                                      10.0,
                                      QStringLiteral("#66bb6a"),
                                      QStringLiteral("start"));
    auto *startB = new ComponentModel(QStringLiteral("S2"),
                                      QStringLiteral("Start B"),
                                      20.0,
                                      20.0,
                                      QStringLiteral("#66bb6a"),
                                      QStringLiteral("start"));
    auto *stop = new ComponentModel(QStringLiteral("T1"),
                                    QStringLiteral("Stop"),
                                    30.0,
                                    30.0,
                                    QStringLiteral("#ef5350"),
                                    QStringLiteral("stop"));
    graph.addComponent(startA);
    graph.addComponent(startB);
    graph.addComponent(stop);

    const QStringList duplicateStartErrors = validation.validationErrors(&graph);
    QVERIFY(containsFragment(duplicateStartErrors,
                             QStringLiteral("exactly one start component (found 2)")));
}

void PersistenceValidationTests::validationRejectsDisconnectedWorkflow()
{
    GraphModel graph;

    auto *start = new ComponentModel(QStringLiteral("S1"),
                                     QStringLiteral("Start"),
                                     0.0,
                                     0.0,
                                     QStringLiteral("#66bb6a"),
                                     QStringLiteral("start"));
    auto *stop = new ComponentModel(QStringLiteral("T1"),
                                    QStringLiteral("Stop"),
                                    100.0,
                                    0.0,
                                    QStringLiteral("#ef5350"),
                                    QStringLiteral("stop"));
    graph.addComponent(start);
    graph.addComponent(stop);

    SampleValidationProvider provider;
    ValidationService validation;
    validation.setValidationProviders({ &provider });
    const QStringList disconnectedErrors = validation.validationErrors(&graph);
    QVERIFY(containsFragment(disconnectedErrors,
                             QStringLiteral("Start component 'S1' must have at least one outgoing connection")));
    QVERIFY(containsFragment(disconnectedErrors,
                             QStringLiteral("Stop component 'T1' must have at least one incoming connection")));
    QVERIFY(containsFragment(disconnectedErrors,
                             QStringLiteral("Component 'T1' is not reachable from start component 'S1'")));

    auto *process = new ComponentModel(QStringLiteral("P1"),
                                       QStringLiteral("Process"),
                                       50.0,
                                       50.0,
                                       QStringLiteral("#4fc3f7"),
                                       QStringLiteral("process"));
    graph.addComponent(process);

    auto *startToStop = new ConnectionModel(QStringLiteral("C1"),
                                            QStringLiteral("S1"),
                                            QStringLiteral("T1"),
                                            QStringLiteral("flow"));
    graph.addConnection(startToStop);

    const QStringList unreachableErrors = validation.validationErrors(&graph);
    QVERIFY(containsFragment(unreachableErrors,
                             QStringLiteral("Component 'P1' must have at least one incoming connection")));
    QVERIFY(containsFragment(unreachableErrors,
                             QStringLiteral("Component 'P1' must have at least one outgoing connection")));
    QVERIFY(containsFragment(unreachableErrors,
                             QStringLiteral("Component 'P1' is not reachable from start component 'S1'")));
}

QTEST_MAIN(PersistenceValidationTests)
#include "tst_PersistenceValidation.moc"
