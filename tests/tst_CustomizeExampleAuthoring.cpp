#include <QtTest>

#include "EditorFacade.h"
#include "services/ExportService.h"
#include "extensions/providers/customizecomponenttypeprovider.h"
#include "extensions/providers/customizeconnectionpolicyprovider.h"
#include "extensions/providers/customizevalidationprovider.h"

class tst_CustomizeExampleAuthoring : public QObject
{
    Q_OBJECT

private slots:
    void domainWorkflowUsesEditorFacade()
    {
        ComponentMapEditorFacade facade;
        CustomizeComponentTypeProvider types;
        CustomizeConnectionPolicyProvider policies;
        CustomizeValidationProvider validation;
        QString error;

        QVERIFY2(facade.registerComponentType(&types, &error), qPrintable(error));
        QVERIFY2(facade.registerConnectionPolicy(&policies, &error), qPrintable(error));
        QVERIFY2(facade.registerValidationProvider(&validation, &error), qPrintable(error));

        const QString start = facade.createComponent(QString::fromLatin1(CustomizeComponentTypeProvider::TypeStart), 0, 0);
        const QString process = facade.createComponent(QStringLiteral("process"), 100, 0);
        const QString stop = facade.createComponent(QString::fromLatin1(CustomizeComponentTypeProvider::TypeStop), 200, 0);
        QVERIFY(!start.isEmpty());
        QVERIFY(!process.isEmpty());
        QVERIFY(!stop.isEmpty());

        QVERIFY(facade.setComponentProperty(process, QStringLiteral("inputNumber"), 7));
        const QString clone = facade.cloneComponent(process, 0, 80);
        QVERIFY(!clone.isEmpty());

        QVERIFY(!facade.connectPorts(start, process).isEmpty());
        QVERIFY(!facade.connectPorts(process, stop).isEmpty());
        QVERIFY(facade.validateGraph());

        ExportService exporter;
        const QString artifact = exporter.exportToJson(facade.graph());
        QVERIFY(artifact.contains(QStringLiteral("components")));
        QVERIFY(artifact.contains(QStringLiteral("connections")));
    }
};

QTEST_MAIN(tst_CustomizeExampleAuthoring)
#include "tst_CustomizeExampleAuthoring.moc"
