#include "customizeexecutionsanticsprovider.h"

#include <QDebug>
#include <QJsonDocument>
#include <QMetaType>

#include <cmath>

QString CustomizeExecutionSemanticsProvider::providerId() const
{
    return QStringLiteral("customize.workflow.execution");
}

QStringList CustomizeExecutionSemanticsProvider::supportedComponentTypes() const
{
    return {
        QString::fromLatin1(TypeLoop),
        QString::fromLatin1(TypeIfElse),
        QString::fromLatin1(TypeAdd),
        QString::fromLatin1(TypeSubtract),
        QString::fromLatin1(TypeMultiply),
        QString::fromLatin1(TypeDivide),
        QString::fromLatin1(TypeErrorHandler)
    };
}

bool CustomizeExecutionSemanticsProvider::executeComponent(const QString &componentType,
                                                           const QString &componentId,
                                                           const QVariantMap &componentSnapshot,
                                                           const QVariantMap &inputState,
                                                           QVariantMap *outputState,
                                                           QVariantMap *trace,
                                                           QString *error) const
{
    cme::execution::IncomingTokens incomingTokens;
    incomingTokens.insert(QStringLiteral("__legacy_global_state__"), inputState);
    return executeComponentV2(componentType,
                              componentId,
                              componentSnapshot,
                              incomingTokens,
                              outputState,
                              trace,
                              error);
}

bool CustomizeExecutionSemanticsProvider::executeComponentV2(
    const QString &componentType,
    const QString &componentId,
    const QVariantMap &componentSnapshot,
    const cme::execution::IncomingTokens &incomingTokens,
    cme::execution::ExecutionPayload *outputPayload,
    QVariantMap *trace,
    QString *error) const
{
    const QVariantMap context = mergeIncomingTokens(incomingTokens);

    if (componentType == QLatin1String(TypeAdd)
        || componentType == QLatin1String(TypeSubtract)
        || componentType == QLatin1String(TypeMultiply)
        || componentType == QLatin1String(TypeDivide)) {
        return executeBinary(componentType, componentId, componentSnapshot, context, outputPayload, trace, error);
    }

    if (componentType == QLatin1String(TypeIfElse)) {
        QVariantMap out = context;
        const QString conditionKey = resolveText(componentSnapshot,
                                                 QStringLiteral("conditionKey"),
                                                 QStringLiteral("condition"));
        const QString trueRouteKey = resolveText(componentSnapshot,
                                                 QStringLiteral("trueRouteKey"),
                                                 QStringLiteral("routeTrue"));
        const QString falseRouteKey = resolveText(componentSnapshot,
                                                  QStringLiteral("falseRouteKey"),
                                                  QStringLiteral("routeFalse"));

        bool okCondition = false;
        const bool condition = resolveBool(context,
                                           componentSnapshot,
                                           conditionKey,
                                           QStringLiteral("condition"),
                                           false,
                                           &okCondition);
        if (!okCondition) {
            const QString msg = QStringLiteral("Invalid ifelse condition value.");
            out.insert(QStringLiteral("error"), msg);
            if (outputPayload)
                *outputPayload = out;
            if (trace)
                *trace = makeTracePayload(componentType, componentId, context, out, msg);
            if (error)
                *error = msg;
            return false;
        }

        out.insert(trueRouteKey, condition);
        out.insert(falseRouteKey, !condition);

        if (outputPayload)
            *outputPayload = out;
        if (trace)
            *trace = makeTracePayload(componentType, componentId, context, out);
        return true;
    }

    if (componentType == QLatin1String(TypeLoop)) {
        QVariantMap out = context;
        const QString iterKey = resolveText(componentSnapshot, QStringLiteral("iterKey"), QStringLiteral("iter"));
        const QString maxIterKey = resolveText(componentSnapshot, QStringLiteral("maxIterKey"), QStringLiteral("maxIter"));
        const QString continueKey = resolveText(componentSnapshot,
                                                QStringLiteral("continueKey"),
                                                QStringLiteral("continueLoop"));
        const QString conditionKey = resolveText(componentSnapshot,
                                                 QStringLiteral("conditionKey"),
                                                 QStringLiteral("condition"));

        bool okIter = false;
        bool okMaxIter = false;
        bool okCondition = false;
        const int iter = static_cast<int>(resolveNumber(
            context, componentSnapshot, iterKey, QStringLiteral("iter"), 0.0, &okIter));
        const int maxIter = static_cast<int>(resolveNumber(
            context, componentSnapshot, maxIterKey, QStringLiteral("maxIter"), 10.0, &okMaxIter));
        const bool condition = resolveBool(context,
                                           componentSnapshot,
                                           conditionKey,
                                           QStringLiteral("condition"),
                                           true,
                                           &okCondition);

        if (!okIter || !okMaxIter || !okCondition || maxIter <= 0) {
            const QString msg = QStringLiteral("Invalid loop inputs (iter/maxIter/condition).");
            out.insert(QStringLiteral("error"), msg);
            if (outputPayload)
                *outputPayload = out;
            if (trace)
                *trace = makeTracePayload(componentType, componentId, context, out, msg);
            if (error)
                *error = msg;
            return false;
        }

        const int nextIter = iter + 1;
        const bool shouldContinue = condition && (nextIter < maxIter);
        out.insert(iterKey, nextIter);
        out.insert(continueKey, shouldContinue);

        if (outputPayload)
            *outputPayload = out;
        if (trace)
            *trace = makeTracePayload(componentType, componentId, context, out);
        return true;
    }

    if (componentType == QLatin1String(TypeErrorHandler)) {
        QVariantMap out = context;
        const QString errorKey = resolveText(componentSnapshot, QStringLiteral("errorKey"), QStringLiteral("error"));
        if (!out.contains(errorKey))
            out.insert(errorKey, resolveText(componentSnapshot, QStringLiteral("message"), QStringLiteral("Unhandled workflow error.")));

        if (outputPayload)
            *outputPayload = out;
        if (trace)
            *trace = makeTracePayload(componentType, componentId, context, out);

        qWarning().noquote() << QStringLiteral("[Trace][%1] %2 handled=%3")
                                    .arg(componentType, componentId, out.value(errorKey).toString());
        return true;
    }

    QVariantMap passthrough = context;
    if (outputPayload)
        *outputPayload = passthrough;
    if (trace)
        *trace = makeTracePayload(componentType, componentId, context, passthrough);
    return true;
}

QVariantMap CustomizeExecutionSemanticsProvider::mergeIncomingTokens(
    const cme::execution::IncomingTokens &incomingTokens)
{
    QVariantMap merged;
    QStringList tokenKeys = incomingTokens.keys();
    std::sort(tokenKeys.begin(), tokenKeys.end());
    for (const QString &tokenKey : tokenKeys)
        merged.insert(incomingTokens.value(tokenKey));
    return merged;
}

double CustomizeExecutionSemanticsProvider::resolveNumber(const QVariantMap &context,
                                                          const QVariantMap &componentSnapshot,
                                                          const QString &contextKey,
                                                          const QString &fallbackSnapshotKey,
                                                          double fallbackValue,
                                                          bool *ok)
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

QString CustomizeExecutionSemanticsProvider::resolveText(const QVariantMap &componentSnapshot,
                                                         const QString &key,
                                                         const QString &fallback)
{
    const QString text = componentSnapshot.value(key).toString().trimmed();
    return text.isEmpty() ? fallback : text;
}

bool CustomizeExecutionSemanticsProvider::resolveBool(const QVariantMap &context,
                                                      const QVariantMap &componentSnapshot,
                                                      const QString &contextKey,
                                                      const QString &fallbackSnapshotKey,
                                                      bool fallbackValue,
                                                      bool *ok)
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

QVariantMap CustomizeExecutionSemanticsProvider::makeTracePayload(const QString &componentType,
                                                                  const QString &componentId,
                                                                  const QVariantMap &inputs,
                                                                  const QVariantMap &outputs,
                                                                  const QString &errorText)
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

bool CustomizeExecutionSemanticsProvider::executeBinary(
    const QString &opName,
    const QString &componentId,
    const QVariantMap &componentSnapshot,
    const QVariantMap &context,
    cme::execution::ExecutionPayload *outputPayload,
    QVariantMap *trace,
    QString *error)
{
    QVariantMap out = context;

    const QString inputAKey = resolveText(componentSnapshot, QStringLiteral("inputAKey"), QStringLiteral("a"));
    const QString inputBKey = resolveText(componentSnapshot, QStringLiteral("inputBKey"), QStringLiteral("b"));
    const QString outputKey = resolveText(componentSnapshot, QStringLiteral("outputKey"), QStringLiteral("result"));
    const QString errorKey = resolveText(componentSnapshot, QStringLiteral("errorKey"), QStringLiteral("error"));

    bool okA = false;
    bool okB = false;
    const double a = resolveNumber(context, componentSnapshot, inputAKey, QStringLiteral("a"), 0.0, &okA);
    const double b = resolveNumber(context, componentSnapshot, inputBKey, QStringLiteral("b"), 0.0, &okB);

    if (!okA || !okB) {
        const QString msg = QStringLiteral("Invalid numeric input for operation '%1'.").arg(opName);
        out.insert(errorKey, msg);
        if (outputPayload)
            *outputPayload = out;
        if (trace)
            *trace = makeTracePayload(opName, componentId, context, out, msg);
        if (error)
            *error = msg;
        qWarning().noquote() << QStringLiteral("[Trace][%1] %2 error=%3")
                                    .arg(opName, componentId, msg);
        return false;
    }

    double value = 0.0;
    if (opName == QLatin1String(TypeAdd))
        value = a + b;
    else if (opName == QLatin1String(TypeSubtract))
        value = a - b;
    else if (opName == QLatin1String(TypeMultiply))
        value = a * b;
    else if (opName == QLatin1String(TypeDivide)) {
        if (std::abs(b) < 1e-12) {
            const QString msg = QStringLiteral("Division by zero.");
            out.insert(errorKey, msg);
            if (outputPayload)
                *outputPayload = out;
            if (trace)
                *trace = makeTracePayload(opName, componentId, context, out, msg);
            if (error)
                *error = msg;
            qWarning().noquote() << QStringLiteral("[Trace][%1] %2 error=%3")
                                        .arg(opName, componentId, msg);
            return false;
        }
        value = a / b;
    }

    out.insert(outputKey, value);
    out.remove(errorKey);

    if (outputPayload)
        *outputPayload = out;
    if (trace)
        *trace = makeTracePayload(opName, componentId, context, out);

    qInfo().noquote() << QStringLiteral("[Trace][%1] %2 a=%3 b=%4 result=%5")
                             .arg(opName, componentId)
                             .arg(a)
                             .arg(b)
                             .arg(value);

    return true;
}

CustomizeExecutionSemanticsProvider::SqrtResult
CustomizeExecutionSemanticsProvider::computeSqrtNewton(double s,
                                                       double epsilon,
                                                       int maxIterations,
                                                       double initialGuess)
{
    SqrtResult result;
    if (s < 0.0) {
        result.error = QStringLiteral("Square root of negative number is not supported.");
        return result;
    }

    if (s == 0.0) {
        result.ok = true;
        result.value = 0.0;
        return result;
    }

    double xOld = initialGuess;
    if (xOld <= 0.0)
        xOld = s > 1.0 ? s / 2.0 : 1.0;

    for (int i = 0; i < maxIterations; ++i) {
        if (std::abs(xOld) < 1e-12) {
            result.error = QStringLiteral("Newton iteration encountered zero denominator.");
            return result;
        }

        const double xNew = (xOld + (s / xOld)) / 2.0;
        const double delta = std::abs(xNew - xOld);
        result.iterations = i + 1;
        result.lastDelta = delta;

        if (delta < epsilon) {
            result.ok = true;
            result.value = xNew;
            return result;
        }

        xOld = xNew;
    }

    result.ok = true;
    result.value = xOld;
    return result;
}
