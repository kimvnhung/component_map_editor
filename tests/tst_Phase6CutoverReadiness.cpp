#include <QFile>
#include <QtTest>

#include "services/ExecutionMigrationFlags.h"

namespace {

QString readTextFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(file.readAll());
}

} // namespace

class tst_Phase6CutoverReadiness : public QObject
{
    Q_OBJECT

private slots:
    void tokenTransport_defaultOn_andCompatibilityWindowOpen();
    void v1AdapterRetained_forReleaseWindow();
    void canaryEntrypoints_enableDesignBExplicitly();
};

void tst_Phase6CutoverReadiness::tokenTransport_defaultOn_andCompatibilityWindowOpen()
{
    cme::execution::MigrationFlags::resetDefaults();
    QCOMPARE(cme::execution::MigrationFlags::tokenTransportEnabled(), true);
    QCOMPARE(cme::execution::MigrationFlags::compatibilityWindowOpen(), true);
}

void tst_Phase6CutoverReadiness::v1AdapterRetained_forReleaseWindow()
{
    const QString contractPath =
        QStringLiteral("/home/hungkv/projects/component_map_editor/component_map_editor/extensions/contracts/IExecutionSemanticsProvider.h");
    const QString sandboxPath =
        QStringLiteral("/home/hungkv/projects/component_map_editor/component_map_editor/services/GraphExecutionSandbox.cpp");

    const QString contractSource = readTextFile(contractPath);
    const QString sandboxSource = readTextFile(sandboxPath);

    QVERIFY2(!contractSource.isEmpty(), "Could not read IExecutionSemanticsProvider.h");
    QVERIFY2(!sandboxSource.isEmpty(), "Could not read GraphExecutionSandbox.cpp");

    QVERIFY2(contractSource.contains(QStringLiteral("virtual bool executeComponent(")),
             "v1 executeComponent contract must remain for compatibility window");
    QVERIFY2(contractSource.contains(QStringLiteral("virtual bool executeComponentV2(")),
             "v2 executeComponentV2 contract must be present after cutover");
    QVERIFY2(sandboxSource.contains(QStringLiteral("__legacy_global_state__")),
             "Legacy global-state fallback path must remain until compatibility window closes");
}

void tst_Phase6CutoverReadiness::canaryEntrypoints_enableDesignBExplicitly()
{
    const QString exampleMainPath =
        QStringLiteral("/home/hungkv/projects/component_map_editor/example/main.cpp");
    const QString customizeMainPath =
        QStringLiteral("/home/hungkv/projects/component_map_editor/customize_example/main.cpp");

    const QString exampleMain = readTextFile(exampleMainPath);
    const QString customizeMain = readTextFile(customizeMainPath);

    QVERIFY2(!exampleMain.isEmpty(), "Could not read example/main.cpp");
    QVERIFY2(!customizeMain.isEmpty(), "Could not read customize_example/main.cpp");

    QVERIFY2(exampleMain.contains(QStringLiteral("MigrationFlags::setTokenTransportEnabled(true)")),
             "Example app should explicitly enable token transport for canary rollout");
    QVERIFY2(customizeMain.contains(QStringLiteral("MigrationFlags::setTokenTransportEnabled(true)")),
             "Customize app should explicitly enable token transport for canary rollout");
}

QTEST_MAIN(tst_Phase6CutoverReadiness)
#include "tst_Phase6CutoverReadiness.moc"
