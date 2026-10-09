// tst_PropertySchemaNormalizedRowBaseline.cpp
//
// Phase 0 of the SchemaOptionsSource refactor.
//
// Captures a snapshot of SchemaFieldLegacyAdapter::toNormalizedRow() for every
// field of every known property schema provider (graph, sample pack,
// customize_example, factory pack). Later refactor phases must keep this
// snapshot byte-for-byte equal (until the phase explicitly changes the row
// contract, in which case the snapshot is regenerated on purpose).
//
// Snapshot file : tests/baseline/property_schema_normalized_rows.json
// First run     : when the snapshot file does not exist it is created and the
//                 test is skipped. Commit the generated file.
// Regenerate    : run with CME_UPDATE_SCHEMA_BASELINE=1.

#include <QtTest>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <memory>
#include <vector>

#include "extensions/contracts/IPropertySchemaProvider.h"
#include "extensions/runtime/GraphPropertySchemaProvider.h"
#include "extensions/runtime/SchemaFieldDefinition.h"
#include "extensions/sample_pack/SamplePropertySchemaProvider.h"

#include "customizepropertyschemaprovider.h"
#include "property_schemas/connectionpropertyschemaprovider.h"
#include "property_schemas/controlpropertyschemaprovider.h"
#include "property_schemas/mathpropertyschemaprovider.h"
#include "property_schemas/startstoppropertyschemaprovider.h"
#include "property_schemas/systempropertyschemaprovider.h"
#include "property_schemas/workflowpropertyschemaprovider.h"
#include "FactoryPropertySchemaProvider.h"

#ifndef CME_SCHEMA_BASELINE_FILE
#error "CME_SCHEMA_BASELINE_FILE must be defined by the build system"
#endif

namespace {

// Same pipeline as PropertySchemaRegistry::normalizeFieldRow().
QVariantMap normalize(const QVariantMap &raw, const QString &targetId)
{
    const cme::runtime::SchemaFieldDefinition field =
        cme::runtime::SchemaFieldLegacyAdapter::fromLegacyRow(raw, targetId);
    return cme::runtime::SchemaFieldLegacyAdapter::toNormalizedRow(field);
}

std::vector<std::unique_ptr<IPropertySchemaProvider>> makeProviders()
{
    std::vector<std::unique_ptr<IPropertySchemaProvider>> providers;
    providers.emplace_back(std::make_unique<GraphPropertySchemaProvider>());
    providers.emplace_back(std::make_unique<SamplePropertySchemaProvider>());
    providers.emplace_back(std::make_unique<CustomizePropertySchemaProvider>());
    // Sub-providers are snapshotted individually as well so a regression is
    // attributed to the exact provider instead of the aggregate.
    providers.emplace_back(std::make_unique<StartStopPropertySchemaProvider>());
    providers.emplace_back(std::make_unique<ControlPropertySchemaProvider>());
    providers.emplace_back(std::make_unique<MathPropertySchemaProvider>());
    providers.emplace_back(std::make_unique<WorkflowPropertySchemaProvider>());
    providers.emplace_back(std::make_unique<SystemPropertySchemaProvider>());
    providers.emplace_back(std::make_unique<ConnectionPropertySchemaProvider>());
    providers.emplace_back(std::make_unique<FactoryPropertySchemaProvider>());
    return providers;
}

// { "<providerId>": { "<targetId>": [ normalizedRow, ... ] } }
QJsonObject buildSnapshot()
{
    QJsonObject snapshot;
    for (const auto &provider : makeProviders()) {
        QJsonObject targets;
        QStringList targetIds = provider->schemaTargets();
        targetIds.sort();
        for (const QString &targetId : targetIds) {
            QJsonArray rows;
            const QVariantList raw = provider->propertySchema(targetId);
            for (const QVariant &value : raw)
                rows.append(QJsonObject::fromVariantMap(normalize(value.toMap(), targetId)));
            targets.insert(targetId, rows);
        }
        snapshot.insert(provider->providerId(), targets);
    }
    return snapshot;
}

QString describeFirstDifference(const QJsonObject &expected, const QJsonObject &actual)
{
    QStringList providerIds = expected.keys() + actual.keys();
    providerIds.removeDuplicates();
    providerIds.sort();

    for (const QString &providerId : providerIds) {
        if (!expected.contains(providerId))
            return QStringLiteral("unexpected provider '%1'").arg(providerId);
        if (!actual.contains(providerId))
            return QStringLiteral("missing provider '%1'").arg(providerId);

        const QJsonObject expTargets = expected.value(providerId).toObject();
        const QJsonObject actTargets = actual.value(providerId).toObject();
        QStringList targetIds = expTargets.keys() + actTargets.keys();
        targetIds.removeDuplicates();
        targetIds.sort();

        for (const QString &targetId : targetIds) {
            const QString where = QStringLiteral("%1 / %2").arg(providerId, targetId);
            if (!expTargets.contains(targetId))
                return QStringLiteral("unexpected target '%1'").arg(where);
            if (!actTargets.contains(targetId))
                return QStringLiteral("missing target '%1'").arg(where);

            const QJsonArray expRows = expTargets.value(targetId).toArray();
            const QJsonArray actRows = actTargets.value(targetId).toArray();
            if (expRows.size() != actRows.size()) {
                return QStringLiteral("field count differs in '%1': expected %2, actual %3")
                    .arg(where).arg(expRows.size()).arg(actRows.size());
            }

            for (int i = 0; i < expRows.size(); ++i) {
                const QJsonObject expRow = expRows.at(i).toObject();
                const QJsonObject actRow = actRows.at(i).toObject();
                if (expRow == actRow)
                    continue;

                QStringList keys = expRow.keys() + actRow.keys();
                keys.removeDuplicates();
                keys.sort();
                for (const QString &key : keys) {
                    if (expRow.value(key) != actRow.value(key)) {
                        const auto dump = [](const QJsonValue &v) {
                            return QString::fromUtf8(QJsonDocument(QJsonArray{v}).toJson(QJsonDocument::Compact));
                        };
                        return QStringLiteral("'%1' row #%2 (field '%3'), property '%4': expected %5, actual %6")
                            .arg(where).arg(i)
                            .arg(expRow.value(QStringLiteral("key")).toString())
                            .arg(key, dump(expRow.value(key)), dump(actRow.value(key)));
                    }
                }
            }
        }
    }
    return {};
}

} // namespace

class tst_PropertySchemaNormalizedRowBaseline : public QObject
{
    Q_OBJECT

private slots:
    void providersExposeSchemas()
    {
        const QJsonObject snapshot = buildSnapshot();
        QVERIFY(!snapshot.isEmpty());

        for (auto it = snapshot.constBegin(); it != snapshot.constEnd(); ++it) {
            QVERIFY2(!it.key().isEmpty(), "provider with empty providerId");
            QVERIFY2(!it.value().toObject().isEmpty(),
                     qPrintable(QStringLiteral("provider '%1' exposes no targets").arg(it.key())));
        }
    }

    void allFieldsNormalizeAsValid()
    {
        const QJsonObject snapshot = buildSnapshot();
        for (auto p = snapshot.constBegin(); p != snapshot.constEnd(); ++p) {
            const QJsonObject targets = p.value().toObject();
            for (auto t = targets.constBegin(); t != targets.constEnd(); ++t) {
                const QJsonArray rows = t.value().toArray();
                QVERIFY2(!rows.isEmpty(),
                         qPrintable(QStringLiteral("empty schema: %1 / %2").arg(p.key(), t.key())));
                for (const QJsonValue &row : rows) {
                    const QJsonObject obj = row.toObject();
                    QVERIFY2(obj.value(QStringLiteral("valid")).toBool(),
                             qPrintable(QStringLiteral("invalid field '%1' in %2 / %3: %4")
                                            .arg(obj.value(QStringLiteral("key")).toString(), p.key(), t.key(),
                                                 obj.value(QStringLiteral("schemaError")).toString())));
                }
            }
        }
    }

    // Documents the current optionsSource contract that the refactor must
    // preserve (or migrate deliberately): a dropdown either carries static
    // options or names a dynamic options source, and sources are limited to
    // the known set below.
    void optionsSourceContract()
    {
        const QStringList knownSources = {
            QString(), QStringLiteral("tokenKeys"), QStringLiteral("tokenKeyOptions")
        };

        const QJsonObject snapshot = buildSnapshot();
        for (auto p = snapshot.constBegin(); p != snapshot.constEnd(); ++p) {
            const QJsonObject targets = p.value().toObject();
            for (auto t = targets.constBegin(); t != targets.constEnd(); ++t) {
                for (const QJsonValue &row : t.value().toArray()) {
                    const QJsonObject obj = row.toObject();
                    const QString where = QStringLiteral("%1 / %2 / %3")
                                              .arg(p.key(), t.key(), obj.value(QStringLiteral("key")).toString());
                    const QString source = obj.value(QStringLiteral("optionsSource")).toString();
                    const int sourceEnum = obj.value(QStringLiteral("optionsSourceEnum")).toInt();

                    QVERIFY2(knownSources.contains(source), qPrintable(QStringLiteral("unknown optionsSource at ") + where));

                    const int expectedEnum = source == QStringLiteral("tokenKeys") ? 1
                        : source == QStringLiteral("tokenKeyOptions") ? 2
                        : 0;
                    QVERIFY2(sourceEnum == expectedEnum,
                             qPrintable(QStringLiteral("optionsSourceEnum mismatch at ") + where));
                }
            }
        }
    }

    void normalizedRowsMatchBaselineSnapshot()
    {
        const QJsonObject actual = buildSnapshot();
        const QString path = QStringLiteral(CME_SCHEMA_BASELINE_FILE);
        const bool update = qEnvironmentVariableIntValue("CME_UPDATE_SCHEMA_BASELINE") != 0;

        if (update || !QFile::exists(path)) {
            QFile out(path);
            QVERIFY2(out.open(QIODevice::WriteOnly | QIODevice::Truncate),
                     qPrintable(QStringLiteral("cannot write baseline: ") + path));
            out.write(QJsonDocument(actual).toJson(QJsonDocument::Indented));
            out.close();
            QSKIP(qPrintable(QStringLiteral("Baseline snapshot written to %1. Review and commit it, then re-run.").arg(path)));
        }

        QFile in(path);
        QVERIFY2(in.open(QIODevice::ReadOnly), qPrintable(QStringLiteral("cannot read baseline: ") + path));
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(in.readAll(), &parseError);
        QVERIFY2(parseError.error == QJsonParseError::NoError,
                 qPrintable(QStringLiteral("baseline JSON parse error: ") + parseError.errorString()));

        const QJsonObject expected = doc.object();
        if (expected != actual) {
            QFAIL(qPrintable(QStringLiteral("Normalized schema rows differ from baseline: %1")
                                 .arg(describeFirstDifference(expected, actual))));
        }
    }
};

QTEST_MAIN(tst_PropertySchemaNormalizedRowBaseline)
#include "tst_PropertySchemaNormalizedRowBaseline.moc"
