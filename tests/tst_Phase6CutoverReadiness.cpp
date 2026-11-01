#include <QFile>
#include <QtTest>

#ifndef CME_SOURCE_DIR
#define CME_SOURCE_DIR "."
#endif

namespace {

QString readTextFile(const QString &relativePath)
{
    QFile file(QStringLiteral(CME_SOURCE_DIR) + QStringLiteral("/") + relativePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(file.readAll());
}

} // namespace

class tst_Phase6CutoverReadiness : public QObject
{
    Q_OBJECT

private slots:
    void managerDoesNotExposeSandbox();
    void productionExampleUsesRuntimePanel();
    void editorBuildSourcesExcludeExecutionStack();
};

void tst_Phase6CutoverReadiness::managerDoesNotExposeSandbox()
{
    const QString header = readTextFile(QStringLiteral("component_map_editor/componentmapeditormanager.h"));
    QVERIFY2(!header.isEmpty(), "Could not read component map editor manager header");
    QVERIFY2(!header.contains(QStringLiteral("GraphExecutionSandbox")),
             "Production manager must not expose the local execution sandbox");
    QVERIFY2(!header.contains(QStringLiteral("executionSandbox")),
             "Production manager must not expose an executionSandbox property");
}

void tst_Phase6CutoverReadiness::productionExampleUsesRuntimePanel()
{
    const QString mainQml = readTextFile(QStringLiteral("customize_example/qml/Main.qml"));
    QVERIFY2(!mainQml.isEmpty(), "Could not read customize example QML");
    QVERIFY2(mainQml.contains(QStringLiteral("ExecutionPanel")),
             "Customize example must retain the runtime execution panel");
    QVERIFY2(!mainQml.contains(QStringLiteral("executionSandbox")),
             "Customize example must not access the local execution sandbox");
}

void tst_Phase6CutoverReadiness::editorBuildSourcesExcludeExecutionStack()
{
    const QString cmake = readTextFile(QStringLiteral("component_map_editor/CMakeLists.txt"));
    QVERIFY2(!cmake.isEmpty(), "Could not read editor CMake file");
    const int compatIndex = cmake.indexOf(QStringLiteral("add_library(cme_editor_compat"));
    QVERIFY2(compatIndex > 0, "Legacy simulator compatibility target is missing");
    const QString productionTarget = cmake.left(compatIndex);
    QVERIFY2(!productionTarget.contains(QStringLiteral("GraphExecutionSandbox.cpp")),
             "Production editor target must not compile GraphExecutionSandbox");
    QVERIFY2(!productionTarget.contains(QStringLiteral("ActorMailboxExecutionEngine.cpp")),
             "Production editor target must not compile actor execution");
    QVERIFY2(cmake.contains(QStringLiteral("cme_editor_compat")),
             "Legacy simulator must be isolated in the compatibility target");
    QVERIFY2(cmake.contains(QStringLiteral("GraphExecutionSandbox.cpp")),
             "Compatibility target must retain simulator sources for legacy tests");
    QVERIFY2(cmake.contains(QStringLiteral("ActorMailboxExecutionEngine.cpp")),
             "Compatibility target must retain actor sources for legacy tests");
}

QTEST_MAIN(tst_Phase6CutoverReadiness)
#include "tst_Phase6CutoverReadiness.moc"
