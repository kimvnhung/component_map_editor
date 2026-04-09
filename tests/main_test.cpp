#include <QtQuickTest/quicktest.h>
#include <QtQml/QQmlEngine>

// Provides the ComponentMapEditor QML module import path so the test QML can
// resolve ComponentMapEditor types (ComponentModel, ConnectionModel, etc.).
class TestSetup : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE void qmlEngineAvailable(QQmlEngine *engine)
    {
        // The build system's cmake configure step sets this macro to the
        // directory that contains the auto-generated ComponentMapEditor/ QML
        // module output directory.
        engine->addImportPath(QStringLiteral(COMPONENT_MAP_EDITOR_IMPORT_DIR));
    }
};

QUICK_TEST_MAIN_WITH_SETUP(InteractionStateManager, TestSetup)

#include "main_test.moc"
