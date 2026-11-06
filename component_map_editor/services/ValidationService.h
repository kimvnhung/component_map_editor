#ifndef VALIDATIONSERVICE_H
#define VALIDATIONSERVICE_H

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <qqml.h>

#include "extensions/contracts/IValidationProvider.h"
#include "models/GraphModel.h"
#include "graph.pb.h"

class ExtensionContractRegistry;

class ValidationService : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit ValidationService(QObject *parent = nullptr);

    void setValidationProviders(const QList<const IValidationProvider *> &providers);
    void rebuildValidationFromRegistry(const ExtensionContractRegistry &registry);

    // UI/text-only helper: returns true if the graph has no structural errors.
    // NOT an inter-layer contract — upstream layers must use validationIssues().
    Q_INVOKABLE bool validate(GraphModel *graph);

    // Normalized structured contract (T008/T012/T014/T024): returns
    // QVariantList of QVariantMap entries with keys "code" (normalized),
    // "severity" ("error"/"warning"/"info"), "path" (stable entity
    // identifier), "message", and optional "details"/"remediation".
    // This is the inter-layer contract; only the UI layer formats it to text.
    Q_INVOKABLE QVariantList validationIssues(GraphModel *graph);

    // UI/text-only helper: returns a list of human-readable error messages,
    // empty if valid. NOT an inter-layer contract.
    Q_INVOKABLE QStringList validationErrors(GraphModel *graph);

private:
    // Legacy: Returns QVariantMap snapshot for backward compatibility
    QVariantMap buildGraphSnapshot(GraphModel *graph) const;
    
    // Phase 5: Returns typed GraphSnapshot proto internally
    cme::GraphSnapshot buildTypedGraphSnapshot(GraphModel *graph) const;
    
    static bool issueIsError(const QVariantMap &issue);

    QList<const IValidationProvider *> m_validationProviders;
};

#endif // VALIDATIONSERVICE_H
