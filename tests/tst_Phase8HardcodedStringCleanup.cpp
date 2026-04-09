// tst_Phase8HardcodedStringCleanup.cpp
//
// Phase 8 static-check tests.
// Verify that forbidden raw string literals no longer appear in core decision-logic
// modules.  Adapters are the ONLY allowed location for such literals.
//
// Pass condition: each test reads the target source file, searches for the
// forbidden pattern, and asserts it is absent.

#include <QFile>
#include <QRegularExpression>
#include <QString>
#include <QTest>

#ifndef CME_SOURCE_DIR
#  error "CME_SOURCE_DIR must be defined via CMake target_compile_definitions"
#endif

class tst_Phase8HardcodedStringCleanup : public QObject
{
    Q_OBJECT

private:
    // Helper: read a source file relative to the project root.
    static QString readSource(const QString &relPath)
    {
        const QString fullPath = QStringLiteral(CME_SOURCE_DIR) + QLatin1Char('/') + relPath;
        QFile f(fullPath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            return {};
        return QString::fromUtf8(f.readAll());
    }

    // Helper: assert that a forbidden literal is NOT present in the source text.
    // Reports file + literal on failure for easy diagnosis.
    static void assertAbsent(const QString &source,
                             const QString &relPath,
                             const QString &forbidden)
    {
        const bool found = source.contains(forbidden);
        if (found) {
            const QString msg = QStringLiteral("Forbidden literal \"%1\" found in %2")
                                    .arg(forbidden, relPath);
            QFAIL(qPrintable(msg));
        }
    }

private slots:
    // ── CommandGateway must not contain command-type string literals ──────────

    void commandGateway_noRawCommandTypeStrings()
    {
        const QString path = QStringLiteral("component_map_editor/services/CommandGateway.cpp");
        const QString src = readSource(path);
        QVERIFY2(!src.isEmpty(), "Could not read CommandGateway.cpp");

        // These were the dispatching literals in the old if/else if chain.
        // After Phase 8 they must only live in CommandAdapter.cpp.
        assertAbsent(src, path, QStringLiteral("\"addComponent\""));
        assertAbsent(src, path, QStringLiteral("\"removeComponent\""));
        assertAbsent(src, path, QStringLiteral("\"moveComponent\""));
        assertAbsent(src, path, QStringLiteral("\"addConnection\""));
        assertAbsent(src, path, QStringLiteral("\"removeConnection\""));
        assertAbsent(src, path, QStringLiteral("\"setComponentProperty\""));
        assertAbsent(src, path, QStringLiteral("\"setConnectionProperty\""));
    }

    // ── RuleRuntimeEngine must not contain rule-kind string literals ──────────

    void ruleRuntimeEngine_noRawRuleKindStrings()
    {
        const QString path =
            QStringLiteral("component_map_editor/extensions/runtime/rules/RuleRuntimeEngine.cpp");
        const QString src = readSource(path);
        QVERIFY2(!src.isEmpty(), "Could not read RuleRuntimeEngine.cpp");

        assertAbsent(src, path, QStringLiteral("\"exactlyOneType\""));
        assertAbsent(src, path, QStringLiteral("\"endpointExists\""));
        assertAbsent(src, path, QStringLiteral("\"noIsolated\""));
    }

    // ── RuleCompiler must only reference rule-kind strings in ruleKindFromString ─

    void ruleCompiler_kindStringsConfinedToConverterFunction()
    {
        const QString path =
            QStringLiteral("component_map_editor/extensions/runtime/rules/RuleCompiler.cpp");
        const QString src = readSource(path);
        QVERIFY2(!src.isEmpty(), "Could not read RuleCompiler.cpp");

        // The raw kind strings are allowed ONLY inside ruleKindFromString().
        // Verify they do NOT appear more than once (the converter gets one copy each).
        auto countOccurrences = [&](const QString &literal) {
            int count = 0;
            int pos = 0;
            while ((pos = src.indexOf(literal, pos)) != -1) {
                ++count;
                pos += literal.size();
            }
            return count;
        };

        QVERIFY2(countOccurrences(QStringLiteral("\"exactlyOneType\"")) <= 1,
                 "\"exactlyOneType\" appears more than once in RuleCompiler.cpp — likely leaked outside converter");
        QVERIFY2(countOccurrences(QStringLiteral("\"endpointExists\"")) <= 1,
                 "\"endpointExists\" appears more than once in RuleCompiler.cpp");
        QVERIFY2(countOccurrences(QStringLiteral("\"noIsolated\"")) <= 1,
                 "\"noIsolated\" appears more than once in RuleCompiler.cpp");
    }

    // ── ExtensionCompatibilityChecker must not compare severity as raw strings ──

    void compatibilityChecker_noRawSeverityConditions()
    {
        const QString path =
            QStringLiteral("component_map_editor/extensions/runtime/ExtensionCompatibilityChecker.cpp");
        const QString src = readSource(path);
        QVERIFY2(!src.isEmpty(), "Could not read ExtensionCompatibilityChecker.cpp");

        // The severity string literals "breaking" and "deprecated" may appear inside
        // the apiChangeSeverityToString() helper (one occurrence each) and for output
        // map keys ("breaking" / "deprecated" as QVariantMap insertion keys), but
        // must NOT appear inside any condition (== comparison).
        // We check that no '== "breaking"' or '== "deprecated"' pattern exists.
        assertAbsent(src, path, QStringLiteral("== QStringLiteral(\"breaking\")"));
        assertAbsent(src, path, QStringLiteral("== QStringLiteral(\"deprecated\")"));
        assertAbsent(src, path, QStringLiteral("== QLatin1String(\"breaking\")"));
        assertAbsent(src, path, QStringLiteral("== QLatin1String(\"deprecated\")"));
    }

    // ── ValidationService must not hardcode "error" severity in core logic ────

    void validationService_noRawErrorSeverityLiteral()
    {
        const QString path =
            QStringLiteral("component_map_editor/services/ValidationService.cpp");
        const QString src = readSource(path);
        QVERIFY2(!src.isEmpty(), "Could not read ValidationService.cpp");

        // Old pattern: QStringLiteral("error") used as severity value in issue maps
        // or in issueIsError() comparison.
        assertAbsent(src, path, QStringLiteral("QStringLiteral(\"error\")"));
        assertAbsent(src, path, QStringLiteral("QLatin1String(\"error\")"));
    }

    // ── UndoStack must use GraphSchema::Keys instead of raw property name literals ─

    void undoStack_noRawConnectionPropertyKeyLiterals()
    {
        const QString path =
            QStringLiteral("component_map_editor/commands/UndoStack.cpp");
        const QString src = readSource(path);
        QVERIFY2(!src.isEmpty(), "Could not read UndoStack.cpp");

        // These were raw string literals in isConnectionPropertyUndoable().
        // After Phase 8 they must be replaced by GraphSchema::Keys::* calls.
        assertAbsent(src, path, QStringLiteral("QStringLiteral(\"sourceId\")"));
        assertAbsent(src, path, QStringLiteral("QStringLiteral(\"targetId\")"));
        assertAbsent(src, path, QStringLiteral("QStringLiteral(\"sourceSide\")"));
        assertAbsent(src, path, QStringLiteral("QStringLiteral(\"targetSide\")"));
    }

    // ── Sanity: adapters still CONTAIN the string literals (boundary is intact) ─

    void commandAdapter_containsCommandTypeStrings()
    {
        const QString path =
            QStringLiteral("component_map_editor/adapters/CommandAdapter.cpp");
        const QString src = readSource(path);
        QVERIFY2(!src.isEmpty(), "Could not read CommandAdapter.cpp");

        // Adapters are the only allowed location — they MUST have these strings.
        QVERIFY2(src.contains(QStringLiteral("\"addComponent\"")),
                 "CommandAdapter.cpp must contain \"addComponent\" (it is the adapter boundary)");
        QVERIFY2(src.contains(QStringLiteral("\"removeComponent\"")),
                 "CommandAdapter.cpp must contain \"removeComponent\"");
    }

    void validationAdapter_containsErrorSeverityString()
    {
        const QString path =
            QStringLiteral("component_map_editor/adapters/ValidationAdapter.cpp");
        const QString src = readSource(path);
        QVERIFY2(!src.isEmpty(), "Could not read ValidationAdapter.cpp");

        // The adapter is the canonical location for "error" severity string.
        QVERIFY2(src.contains(QStringLiteral("\"error\"")),
                 "ValidationAdapter.cpp must contain \"error\" (it is the adapter boundary)");
    }
};

QTEST_MAIN(tst_Phase8HardcodedStringCleanup)
#include "tst_Phase8HardcodedStringCleanup.moc"
