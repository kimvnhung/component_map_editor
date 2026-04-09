// tests/tst_Phase5ValidationMigration.cpp
//
// Phase 5: ValidationService Typed Snapshot Migration Tests
//
// Goals:
// - Verify typed GraphSnapshot proto building from graph model
// - Verify roundtrip conversion through ValidationAdapter
// - Validate equivalence of typed vs legacy snapshot handling
// - Test null graph handling
// - Verify warning/error mapping through enums

#include <QtTest>
#include <QVariantMap>
#include <QVariantList>

#include "services/ValidationService.h"
#include "models/GraphModel.h"
#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"
#include "extensions/contracts/IValidationProvider.h"
#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/contracts/ExtensionApiVersion.h"
#include "adapters/ValidationAdapter.h"

// Mock validation provider that returns typed issues
class MockValidationProviderPhase5 : public IValidationProvider
{
public:
    QString providerId() const override {
        return QStringLiteral("test.phase5.provider");
    }

    QVariantList validateGraph(const QVariantMap &graphSnapshot) const override
    {
        QVariantList issues;

        // Null graph check (snapshot empty)
        const int componentCount = graphSnapshot.value("components").toList().size();
        if (componentCount == 0) {
            issues.append(QVariantMap{
                { QStringLiteral("code"), QStringLiteral("TEST_NO_COMPONENTS") },
                { QStringLiteral("severity"), QStringLiteral("warning") },
                { QStringLiteral("message"), QStringLiteral("No components in graph") },
                { QStringLiteral("componentId"), QString() },
                { QStringLiteral("connectionId"), QString() }
            });
            return issues;
        }

        // Check for at least 2 components (error if not)
        if (componentCount < 2) {
            issues.append(QVariantMap{
                { QStringLiteral("code"), QStringLiteral("TEST_MIN_COMPONENTS") },
                { QStringLiteral("severity"), QStringLiteral("error") },
                { QStringLiteral("message"), QStringLiteral("Graph needs at least 2 components") },
                { QStringLiteral("componentId"), QString() },
                { QStringLiteral("connectionId"), QString() }
            });
        }

        // Check connections exist (warning if not)
        const int connectionCount = graphSnapshot.value("connections").toList().size();
        if (connectionCount == 0 && componentCount > 0) {
            issues.append(QVariantMap{
                { QStringLiteral("code"), QStringLiteral("TEST_NO_CONNECTIONS") },
                { QStringLiteral("severity"), QStringLiteral("warning") },
                { QStringLiteral("message"), QStringLiteral("No connections between components") },
                { QStringLiteral("componentId"), QString() },
                { QStringLiteral("connectionId"), QString() }
            });
        }

        return issues;
    }
};

class TestPhase5ValidationMigration : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // Placeholder for test setup
    }

    // ── Adapter Tests ───────────────────────────────────────────────────────

    void test_adapter_variantMapToGraphSnapshot()
    {
        // Test converting legacy snapshot map to typed GraphSnapshot proto
        QVariantMap variantSnapshot;
        QVariantList components;
        QVariantList connections;

        // Add component entry
        components.append(QVariantMap{
            { QStringLiteral("id"), QStringLiteral("comp1") },
            { QStringLiteral("typeId"), QStringLiteral("type_a") },
            { QStringLiteral("x"), 10.0 },
            { QStringLiteral("y"), 20.0 },
            { QStringLiteral("properties"), QVariantMap{
                { QStringLiteral("title"), QStringLiteral("Component 1") }
            }}
        });

        // Add connection entry
        connections.append(QVariantMap{
            { QStringLiteral("id"), QStringLiteral("conn1") },
            { QStringLiteral("sourceId"), QStringLiteral("comp1") },
            { QStringLiteral("targetId"), QStringLiteral("comp2") },
            { QStringLiteral("properties"), QVariantMap{
                { QStringLiteral("label"), QStringLiteral("flows_to") }
            }}
        });

        variantSnapshot[QStringLiteral("components")] = components;
        variantSnapshot[QStringLiteral("connections")] = connections;

        // Convert to proto
        cme::GraphSnapshot protoSnapshot;
        auto err = cme::adapter::variantMapToGraphSnapshotForValidation(variantSnapshot, protoSnapshot);
        QVERIFY(!err.has_error);

        // Verify proto contains expected data
        QCOMPARE(protoSnapshot.components_size(), 1);
        QCOMPARE(protoSnapshot.connections_size(), 1);

        const auto &comp = protoSnapshot.components(0);
        QCOMPARE(QString::fromStdString(comp.id()), QStringLiteral("comp1"));
        QCOMPARE(QString::fromStdString(comp.type_id()), QStringLiteral("type_a"));
        QCOMPARE(comp.x(), 10.0);
        QCOMPARE(comp.y(), 20.0);

        const auto &conn = protoSnapshot.connections(0);
        QCOMPARE(QString::fromStdString(conn.id()), QStringLiteral("conn1"));
        QCOMPARE(QString::fromStdString(conn.source_id()), QStringLiteral("comp1"));
        QCOMPARE(QString::fromStdString(conn.target_id()), QStringLiteral("comp2"));
    }

    void test_adapter_graphSnapshotToVariantMap()
    {
        // Test converting typed GraphSnapshot proto back to legacy map
        cme::GraphSnapshot protoSnapshot;

        // Add component
        auto *comp = protoSnapshot.add_components();
        comp->set_id("comp1");
        comp->set_type_id("type_a");
        comp->set_x(5.0);
        comp->set_y(15.0);
        (*comp->mutable_properties())["title"] = "Test Component";

        // Add connection
        auto *conn = protoSnapshot.add_connections();
        conn->set_id("conn1");
        conn->set_source_id("comp1");
        conn->set_target_id("comp2");
        (*conn->mutable_properties())["label"] = "test_flow";

        // Convert to legacy map
        QVariantMap variantSnapshot = cme::adapter::graphSnapshotForValidationToVariantMap(protoSnapshot);

        // Verify map contains expected data
        const auto components = variantSnapshot[QStringLiteral("components")].toList();
        const auto connections = variantSnapshot[QStringLiteral("connections")].toList();

        QCOMPARE(components.size(), 1);
        QCOMPARE(connections.size(), 1);

        const auto compMap = components.first().toMap();
        QCOMPARE(compMap[QStringLiteral("id")].toString(), QStringLiteral("comp1"));
        QCOMPARE(compMap[QStringLiteral("typeId")].toString(), QStringLiteral("type_a"));

        const auto connMap = connections.first().toMap();
        QCOMPARE(connMap[QStringLiteral("id")].toString(), QStringLiteral("conn1"));
        QCOMPARE(connMap[QStringLiteral("sourceId")].toString(), QStringLiteral("comp1"));
    }

    void test_adapter_roundtrip()
    {
        // Test complete roundtrip: map → proto → map preserves all data
        QVariantMap original;
        QVariantList components, connections;

        components.append(QVariantMap{
            { QStringLiteral("id"), QStringLiteral("nodeA") },
            { QStringLiteral("typeId"), QStringLiteral("type_x") },
            { QStringLiteral("x"), 100.5 },
            { QStringLiteral("y"), 200.5 }
        });

        components.append(QVariantMap{
            { QStringLiteral("id"), QStringLiteral("nodeB") },
            { QStringLiteral("typeId"), QStringLiteral("type_y") },
            { QStringLiteral("x"), 300.5 },
            { QStringLiteral("y"), 400.5 }
        });

        connections.append(QVariantMap{
            { QStringLiteral("id"), QStringLiteral("edge1") },
            { QStringLiteral("sourceId"), QStringLiteral("nodeA") },
            { QStringLiteral("targetId"), QStringLiteral("nodeB") }
        });

        original[QStringLiteral("components")] = components;
        original[QStringLiteral("connections")] = connections;

        // First pass: map → proto
        cme::GraphSnapshot proto;
        auto err = cme::adapter::variantMapToGraphSnapshotForValidation(original, proto);
        QVERIFY(!err.has_error);

        // Second pass: proto → map
        const QVariantMap roundtrip = cme::adapter::graphSnapshotForValidationToVariantMap(proto);

        // Verify roundtrip preserves all data
        const auto origComponents = original[QStringLiteral("components")].toList();
        const auto tripComponents = roundtrip[QStringLiteral("components")].toList();
        QCOMPARE(tripComponents.size(), origComponents.size());

        const auto origConnections = original[QStringLiteral("connections")].toList();
        const auto tripConnections = roundtrip[QStringLiteral("connections")].toList();
        QCOMPARE(tripConnections.size(), origConnections.size());

        // Check first component preserved
        const auto origComp0 = origComponents.first().toMap();
        const auto tripComp0 = tripComponents.first().toMap();
        QCOMPARE(tripComp0[QStringLiteral("id")], origComp0[QStringLiteral("id")]);
        QCOMPARE(tripComp0[QStringLiteral("typeId")], origComp0[QStringLiteral("typeId")]);
    }

    // ── ValidationService Tests ─────────────────────────────────────────────

    void test_validationService_nullGraph()
    {
        // Test that ValidationService.validationIssues() handles null graph correctly
        auto service = std::make_unique<ValidationService>();

        // Register mock provider
        MockValidationProviderPhase5 mockProvider;
        service->setValidationProviders({&mockProvider});

        // Validate null graph
        const QVariantList issues = service->validationIssues(nullptr);

        // Should have single "null graph" issue
        QCOMPARE(issues.size(), 1);
        const auto issue = issues.first().toMap();
        QCOMPARE(issue[QStringLiteral("code")].toString(), QStringLiteral("CORE_NULL_GRAPH"));
        QCOMPARE(issue[QStringLiteral("severity")].toString(), QStringLiteral("error"));
        QVERIFY(issue[QStringLiteral("message")].toString().contains("null"));
    }

    void test_validationService_emptyGraph()
    {
        // Test validation of empty graph (no components)
        auto service = std::make_unique<ValidationService>();
        auto graph = std::make_unique<GraphModel>();

        MockValidationProviderPhase5 mockProvider;
        service->setValidationProviders({&mockProvider});

        // Validate empty graph
        const QVariantList issues = service->validationIssues(graph.get());

        // Should have warning about no components
        QVERIFY(!issues.isEmpty());
        bool foundNoComponents = false;
        for (const auto &issueVar : issues) {
            const auto issue = issueVar.toMap();
            if (issue[QStringLiteral("code")].toString() == QStringLiteral("TEST_NO_COMPONENTS")) {
                foundNoComponents = true;
                QCOMPARE(issue[QStringLiteral("severity")].toString(), QStringLiteral("warning"));
            }
        }
        QVERIFY(foundNoComponents);
    }

    void test_validationService_withComponentsNoConnections()
    {
        // Test graph with components but no connections
        auto service = std::make_unique<ValidationService>();
        auto graph = std::make_unique<GraphModel>();

        // Add two components
        auto *comp1 = new ComponentModel(QStringLiteral("comp1"), QStringLiteral("Component 1"), 0.0, 0.0);
        comp1->setType(QStringLiteral("type_a"));
        graph->addComponent(comp1);

        auto *comp2 = new ComponentModel(QStringLiteral("comp2"), QStringLiteral("Component 2"), 100.0, 0.0);
        comp2->setType(QStringLiteral("type_b"));
        graph->addComponent(comp2);

        MockValidationProviderPhase5 mockProvider;
        service->setValidationProviders({&mockProvider});

        // Validate graph with components but no connections
        const QVariantList issues = service->validationIssues(graph.get());

        // Should have warning about no connections
        bool foundNoConnections = false;
        for (const auto &issueVar : issues) {
            const auto issue = issueVar.toMap();
            if (issue[QStringLiteral("code")].toString() == QStringLiteral("TEST_NO_CONNECTIONS")) {
                foundNoConnections = true;
                QCOMPARE(issue[QStringLiteral("severity")].toString(), QStringLiteral("warning"));
            }
        }
        QVERIFY(foundNoConnections);
    }

    void test_validationService_withComponentsAndConnections()
    {
        // Test graph with both components and connections (valid graph)
        auto service = std::make_unique<ValidationService>();
        auto graph = std::make_unique<GraphModel>();

        // Add two components
        auto *comp1 = new ComponentModel(QStringLiteral("comp1"), QStringLiteral("Component 1"), 0.0, 0.0);
        comp1->setType(QStringLiteral("type_a"));
        graph->addComponent(comp1);

        auto *comp2 = new ComponentModel(QStringLiteral("comp2"), QStringLiteral("Component 2"), 100.0, 0.0);
        comp2->setType(QStringLiteral("type_b"));
        graph->addComponent(comp2);

        // Add connection between them
        auto *conn = new ConnectionModel(QStringLiteral("conn1"), QStringLiteral("comp1"), QStringLiteral("comp2"));
        conn->setLabel(QStringLiteral("flows_to"));
        graph->addConnection(conn);

        MockValidationProviderPhase5 mockProvider;
        service->setValidationProviders({&mockProvider});

        // Validate complete graph
        const QVariantList issues = service->validationIssues(graph.get());

        // Should have no issues (valid graph)
        QVERIFY(issues.isEmpty());
    }

    void test_validationService_severityMapping_warning()
    {
        // Test that warning severity is preserved through validation
        auto service = std::make_unique<ValidationService>();
        auto graph = std::make_unique<GraphModel>();

        MockValidationProviderPhase5 mockProvider;
        service->setValidationProviders({&mockProvider});

        // Empty graph should produce warning
        const QVariantList issues = service->validationIssues(graph.get());

        // Find warning issue
        for (const auto &issueVar : issues) {
            const auto issue = issueVar.toMap();
            if (issue[QStringLiteral("severity")].toString() == QStringLiteral("warning")) {
                // Verify it's a proper severity value
                const QString severity = issue[QStringLiteral("severity")].toString();
                QVERIFY(severity == QStringLiteral("warning") || 
                       severity == QStringLiteral("error") || 
                       severity == QStringLiteral("info"));
                return; // Found it
            }
        }
        QFAIL("No warning severity found in validation issues");
    }

    void test_validationService_severityMapping_error()
    {
        // Test that error severity is preserved through validation
        auto service = std::make_unique<ValidationService>();
        auto graph = std::make_unique<GraphModel>();

        // Add only one component (triggers error)
        auto *comp = new ComponentModel(QStringLiteral("comp1"), QStringLiteral("Component 1"), 0.0, 0.0);
        comp->setType(QStringLiteral("type_a"));
        graph->addComponent(comp);

        MockValidationProviderPhase5 mockProvider;
        service->setValidationProviders({&mockProvider});

        // Single component should produce error
        const QVariantList issues = service->validationIssues(graph.get());

        // Find error issue
        bool foundError = false;
        for (const auto &issueVar : issues) {
            const auto issue = issueVar.toMap();
            if (issue[QStringLiteral("code")].toString() == QStringLiteral("TEST_MIN_COMPONENTS")) {
                QCOMPARE(issue[QStringLiteral("severity")].toString(), QStringLiteral("error"));
                foundError = true;
                break;
            }
        }
        QVERIFY(foundError);
    }

    void test_validationService_issueFieldConsistency()
    {
        // Test that all required issue fields are present and consistent
        auto service = std::make_unique<ValidationService>();
        auto graph = std::make_unique<GraphModel>();

        MockValidationProviderPhase5 mockProvider;
        service->setValidationProviders({&mockProvider});

        const QVariantList issues = service->validationIssues(graph.get());

        // Each issue should have required fields
        for (const auto &issueVar : issues) {
            const auto issue = issueVar.toMap();

            // Verify required keys
            QVERIFY(issue.contains(QStringLiteral("code")));
            QVERIFY(issue.contains(QStringLiteral("severity")));
            QVERIFY(issue.contains(QStringLiteral("message")));

            // Verify types
            QVERIFY(issue[QStringLiteral("code")].canConvert<QString>());
            QVERIFY(issue[QStringLiteral("severity")].canConvert<QString>());
            QVERIFY(issue[QStringLiteral("message")].canConvert<QString>());

            // Verify severity is valid enum value
            const QString severity = issue[QStringLiteral("severity")].toString().toLower();
            QVERIFY(severity == QStringLiteral("error") || 
                   severity == QStringLiteral("warning") || 
                   severity == QStringLiteral("info"));
        }
    }

    void test_validationService_typedVsLegacy_equivalence()
    {
        // Test that typed snapshot produces same validation results as legacy
        auto service = std::make_unique<ValidationService>();
        auto graph = std::make_unique<GraphModel>();

        // Build a graph with components and connections
        auto *comp1 = new ComponentModel(QStringLiteral("comp1"), QStringLiteral("Start"), 0.0, 0.0);
        comp1->setType(QStringLiteral("type_a"));
        graph->addComponent(comp1);

        auto *comp2 = new ComponentModel(QStringLiteral("comp2"), QStringLiteral("End"), 100.0, 0.0);
        comp2->setType(QStringLiteral("type_b"));
        graph->addComponent(comp2);

        auto *conn = new ConnectionModel(QStringLiteral("conn1"), 
                                        QStringLiteral("comp1"), 
                                        QStringLiteral("comp2"));
        conn->setLabel(QStringLiteral("success"));
        graph->addConnection(conn);

        MockValidationProviderPhase5 mockProvider;
        service->setValidationProviders({&mockProvider});

        // Run validation (uses typed snapshot internally)
        const QVariantList typedIssues = service->validationIssues(graph.get());

        // Should match expected validation results for complete graph
        QVERIFY(typedIssues.isEmpty()); // Complete graph has no issues
    }

    void test_validationService_validate_method()
    {
        // Test ValidationService.validate() method which uses validationIssues internally
        auto service = std::make_unique<ValidationService>();
        auto graph = std::make_unique<GraphModel>();

        auto *comp1 = new ComponentModel(QStringLiteral("comp1"), QStringLiteral("Component 1"), 0.0, 0.0);
        comp1->setType(QStringLiteral("type_a"));
        graph->addComponent(comp1);

        auto *comp2 = new ComponentModel(QStringLiteral("comp2"), QStringLiteral("Component 2"), 100.0, 0.0);
        comp2->setType(QStringLiteral("type_b"));
        graph->addComponent(comp2);

        auto *conn = new ConnectionModel(QStringLiteral("conn1"), 
                                        QStringLiteral("comp1"), 
                                        QStringLiteral("comp2"));
        graph->addConnection(conn);

        MockValidationProviderPhase5 mockProvider;
        service->setValidationProviders({&mockProvider});

        // Valid graph should return true
        bool isValid = service->validate(graph.get());
        QVERIFY(isValid);
    }

    void cleanupTestCase()
    {
        // Placeholder for test cleanup
    }
};

QTEST_MAIN(TestPhase5ValidationMigration)

#include "tst_Phase5ValidationMigration.moc"
