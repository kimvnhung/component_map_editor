#ifndef CUSTOMIZEEXECUTIONCOMMON_H
#define CUSTOMIZEEXECUTIONCOMMON_H

#include "extensions/contracts/IExecutionSemanticsProvider.h"

#include <QDebug>
#include <QMetaType>

#include <algorithm>
#include <cmath>

namespace customize::executors {

enum class BinaryOperation {
    Add,
    Subtract,
    Multiply,
    Divide
};

inline QVariantMap mergeIncomingTokens(const cme::execution::IncomingTokens &incomingTokens)
{
    QVariantMap merged;
    QHash<QString, int> fieldCounts;
    QStringList tokenKeys = incomingTokens.keys();
    std::sort(tokenKeys.begin(), tokenKeys.end());
    for (const QString &tokenKey : tokenKeys) {
        QStringList fieldKeys = incomingTokens.value(tokenKey).keys();
        std::sort(fieldKeys.begin(), fieldKeys.end());
        for (const QString &fieldKey : fieldKeys)
            ++fieldCounts[fieldKey];
    }

    for (const QString &tokenKey : tokenKeys) {
        const QVariantMap tokenPayload = incomingTokens.value(tokenKey);
        QStringList fieldKeys = tokenPayload.keys();
        std::sort(fieldKeys.begin(), fieldKeys.end());
        for (const QString &fieldKey : fieldKeys) {
            if (fieldCounts.value(fieldKey) == 1)
                merged.insert(fieldKey, tokenPayload.value(fieldKey));
        }
    }
    return merged;
}

inline QVariant resolveUniqueIncomingTokenValue(const cme::execution::IncomingTokens &incomingTokens,
                                                const QString &fieldKey,
                                                bool *found = nullptr,
                                                bool *ambiguous = nullptr)
{
    bool resolved = false;
    bool duplicate = false;
    QVariant value;

    QStringList tokenKeys = incomingTokens.keys();
    std::sort(tokenKeys.begin(), tokenKeys.end());
    for (const QString &tokenKey : tokenKeys) {
        const QVariantMap tokenPayload = incomingTokens.value(tokenKey);
        if (!tokenPayload.contains(fieldKey))
            continue;

        if (resolved) {
            duplicate = true;
            value = {};
            break;
        }

        value = tokenPayload.value(fieldKey);
        resolved = true;
    }

    if (found)
        *found = resolved && !duplicate;
    if (ambiguous)
        *ambiguous = duplicate;
    return value;
}

inline bool parseTokenFieldReference(const QString &reference,
                                     QString *tokenId,
                                     QString *fieldKey)
{
    const QString trimmed = reference.trimmed();
    const int sep = trimmed.indexOf(QStringLiteral("::"));
    if (sep <= 0 || sep >= trimmed.size() - 2)
        return false;

    const QString left = trimmed.left(sep).trimmed();
    const QString right = trimmed.mid(sep + 2).trimmed();
    if (left.isEmpty() || right.isEmpty())
        return false;

    if (tokenId)
        *tokenId = left;
    if (fieldKey)
        *fieldKey = right;
    return true;
}

inline double resolveReferencedNumber(const cme::execution::IncomingTokens &incomingTokens,
                                     const QVariantMap &componentSnapshot,
                                     const QString &referenceProperty,
                                     const QString &fallbackSnapshotKey,
                                     double fallbackValue,
                                     bool *ok = nullptr)
{
    bool parsed = false;
    const QString reference = componentSnapshot.value(referenceProperty).toString().trimmed();
    if (!reference.isEmpty()) {
        QString tokenId;
        QString fieldKey;
        if (parseTokenFieldReference(reference, &tokenId, &fieldKey)) {
            const QVariantMap tokenPayload = incomingTokens.value(tokenId);
            if (tokenPayload.contains(fieldKey)) {
                const double value = tokenPayload.value(fieldKey).toDouble(&parsed);
                if (ok)
                    *ok = parsed;
                if (parsed)
                    return value;
            }
        }
    }

    double value = componentSnapshot.value(fallbackSnapshotKey, fallbackValue).toDouble(&parsed);
    if (!parsed)
        value = fallbackValue;
    if (ok)
        *ok = parsed;
    return value;
}

inline QVariant resolveSelectedContextValue(const cme::execution::IncomingTokens &incomingTokens,
                                            const QVariantMap &context,
                                            const QVariantMap &componentSnapshot,
                                            const QString &referenceProperty,
                                            const QString &keyProperty,
                                            const QString &fallbackKey,
                                            bool *ambiguous = nullptr)
{
    if (ambiguous)
        *ambiguous = false;

    const QString reference = componentSnapshot.value(referenceProperty).toString().trimmed();
    QString tokenId;
    QString fieldKey;
    if (parseTokenFieldReference(reference, &tokenId, &fieldKey)) {
        const QVariantMap tokenPayload = incomingTokens.value(tokenId);
        if (tokenPayload.contains(fieldKey))
            return tokenPayload.value(fieldKey);
    }

    const QString contextKeyRaw = componentSnapshot.value(keyProperty).toString().trimmed();
    const QString contextKey = contextKeyRaw.isEmpty() ? fallbackKey : contextKeyRaw;
    bool foundUniqueIncoming = false;
    bool ambiguousIncoming = false;
    const QVariant selectedIncoming = resolveUniqueIncomingTokenValue(incomingTokens,
                                                                      contextKey,
                                                                      &foundUniqueIncoming,
                                                                      &ambiguousIncoming);
    if (ambiguousIncoming) {
        if (ambiguous)
            *ambiguous = true;
        return {};
    }
    if (foundUniqueIncoming)
        return selectedIncoming;

    if (context.contains(contextKey))
        return context.value(contextKey);
    if (componentSnapshot.contains(fallbackKey))
        return componentSnapshot.value(fallbackKey);
    return {};
}

inline double resolveSelectedNumber(const cme::execution::IncomingTokens &incomingTokens,
                                    const QVariantMap &context,
                                    const QVariantMap &componentSnapshot,
                                    const QString &referenceProperty,
                                    const QString &keyProperty,
                                    const QString &fallbackSnapshotKey,
                                    double fallbackValue,
                                    bool *ok = nullptr)
{
    bool ambiguous = false;
    bool parsed = false;
    const QVariant selected = resolveSelectedContextValue(incomingTokens,
                                                          context,
                                                          componentSnapshot,
                                                          referenceProperty,
                                                          keyProperty,
                                                          fallbackSnapshotKey,
                                                          &ambiguous);
    double value = selected.toDouble(&parsed);
    if (!parsed)
        value = componentSnapshot.value(fallbackSnapshotKey, fallbackValue).toDouble(&parsed);
    if (ambiguous)
        parsed = false;
    if (!parsed)
        value = fallbackValue;
    if (ok)
        *ok = parsed;
    return value;
}

inline QString resolveText(const QVariantMap &componentSnapshot,
                          const QString &key,
                          const QString &fallback)
{
    const QString text = componentSnapshot.value(key).toString().trimmed();
    return text.isEmpty() ? fallback : text;
}

inline double resolveNumber(const QVariantMap &context,
                            const QVariantMap &componentSnapshot,
                            const QString &contextKey,
                            const QString &fallbackSnapshotKey,
                            double fallbackValue,
                            bool *ok = nullptr)
{
    bool parsed = false;
    double value = context.value(contextKey).toDouble(&parsed);
    if (!parsed)
        value = componentSnapshot.value(fallbackSnapshotKey, fallbackValue).toDouble(&parsed);
    if (!parsed)
        value = fallbackValue;
    if (ok)
        *ok = parsed;
    return value;
}

inline bool resolveBool(const QVariantMap &context,
                        const QVariantMap &componentSnapshot,
                        const QString &contextKey,
                        const QString &fallbackSnapshotKey,
                        bool fallbackValue,
                        bool *ok = nullptr)
{
    auto parseBool = [](const QVariant &value, bool *parsed) {
        if (!value.isValid() || value.isNull()) {
            if (parsed)
                *parsed = false;
            return false;
        }

        if (value.metaType().id() == QMetaType::Bool) {
            if (parsed)
                *parsed = true;
            return value.toBool();
        }

        if (value.canConvert<bool>()) {
            if (parsed)
                *parsed = true;
            return value.toBool();
        }

        const QString text = value.toString().trimmed().toLower();
        if (text == QStringLiteral("true") || text == QStringLiteral("1")) {
            if (parsed)
                *parsed = true;
            return true;
        }
        if (text == QStringLiteral("false") || text == QStringLiteral("0")) {
            if (parsed)
                *parsed = true;
            return false;
        }

        if (parsed)
            *parsed = false;
        return false;
    };

    bool parsed = false;
    bool value = parseBool(context.value(contextKey), &parsed);
    if (!parsed)
        value = parseBool(componentSnapshot.value(fallbackSnapshotKey), &parsed);
    if (!parsed)
        value = fallbackValue;
    if (ok)
        *ok = parsed;
    return value;
}

inline QVariantMap makeTracePayload(const QString &componentType,
                                    const QString &componentId,
                                    const QVariantMap &inputs,
                                    const QVariantMap &outputs,
                                    const QString &errorText = QString())
{
    QVariantMap trace{
        { QStringLiteral("componentType"), componentType },
        { QStringLiteral("componentId"), componentId },
        { QStringLiteral("inputs"), inputs },
        { QStringLiteral("outputs"), outputs }
    };

    if (!errorText.isEmpty())
        trace.insert(QStringLiteral("error"), errorText);
    return trace;
}

inline bool failExecution(const QString &componentType,
                          const QString &componentId,
                          const QVariantMap &inputs,
                          QVariantMap out,
                          const QString &errorKey,
                          const QString &message,
                          cme::execution::ExecutionPayload *outputPayload,
                          QVariantMap *trace,
                          QString *error)
{
    out.insert(errorKey, message);
    if (outputPayload)
        *outputPayload = out;
    if (trace)
        *trace = makeTracePayload(componentType, componentId, inputs, out, message);
    if (error)
        *error = message;
    return false;
}

} // namespace customize::executors

#endif // CUSTOMIZEEXECUTIONCOMMON_H