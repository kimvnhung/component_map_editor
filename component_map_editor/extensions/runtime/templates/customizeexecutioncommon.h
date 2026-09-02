#ifndef CUSTOMIZEEXECUTIONCOMMON_H
#define CUSTOMIZEEXECUTIONCOMMON_H

#include "extensions/contracts/IExecutionSemanticsProvider.h"

namespace customize::executors {

QString fallbackReferenceSelectionValue();

bool isFallbackReferenceSelection(const QString &reference);

enum class BinaryOperation {
    Add,
    Subtract,
    Multiply,
    Divide
};

QVariantMap mergeIncomingTokens(const cme::execution::IncomingTokens &incomingTokens);

QVariant resolveUniqueIncomingTokenValue(const cme::execution::IncomingTokens &incomingTokens,
                                         const QString &fieldKey,
                                         bool *found = nullptr,
                                         bool *ambiguous = nullptr);

bool parseTokenFieldReference(const QString &reference,
                              QString *tokenId,
                              QString *fieldKey);

double resolveReferencedNumber(const cme::execution::IncomingTokens &incomingTokens,
                               const QVariantMap &componentSnapshot,
                               const QString &referenceProperty,
                               const QString &fallbackSnapshotKey,
                               double fallbackValue,
                               bool *ok = nullptr);

QVariant resolveSelectedContextValue(const cme::execution::IncomingTokens &incomingTokens,
                                     const QVariantMap &context,
                                     const QVariantMap &componentSnapshot,
                                     const QString &referenceProperty,
                                     const QString &keyProperty,
                                     const QString &fallbackKey,
                                     bool *ambiguous = nullptr);

double resolveSelectedNumber(const cme::execution::IncomingTokens &incomingTokens,
                             const QVariantMap &context,
                             const QVariantMap &componentSnapshot,
                             const QString &referenceProperty,
                             const QString &keyProperty,
                             const QString &fallbackSnapshotKey,
                             double fallbackValue,
                             bool *ok = nullptr);

QString resolveText(const QVariantMap &componentSnapshot,
                   const QString &key,
                   const QString &fallback);

double resolveNumber(const QVariantMap &context,
                     const QVariantMap &componentSnapshot,
                     const QString &contextKey,
                     const QString &fallbackSnapshotKey,
                     double fallbackValue,
                     bool *ok = nullptr);

bool resolveBool(const QVariantMap &context,
                 const QVariantMap &componentSnapshot,
                 const QString &contextKey,
                 const QString &fallbackSnapshotKey,
                 bool fallbackValue,
                 bool *ok = nullptr);

QVariantMap makeTracePayload(const QString &componentType,
                             const QString &componentId,
                             const QVariantMap &inputs,
                             const QVariantMap &outputs,
                             const QString &errorText = QString());

bool failExecution(const QString &componentType,
                   const QString &componentId,
                   const QVariantMap &inputs,
                   QVariantMap out,
                   const QString &errorKey,
                   const QString &message,
                   cme::execution::ExecutionPayload *outputPayload,
                   QVariantMap *trace,
                   QString *error);

} // namespace customize::executors

#endif // CUSTOMIZEEXECUTIONCOMMON_H