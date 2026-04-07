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
    QStringList tokenKeys = incomingTokens.keys();
    std::sort(tokenKeys.begin(), tokenKeys.end());
    for (const QString &tokenKey : tokenKeys)
        merged.insert(incomingTokens.value(tokenKey));
    return merged;
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