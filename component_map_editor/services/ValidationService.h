#ifndef VALIDATIONSERVICE_H
#define VALIDATIONSERVICE_H

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <qqml.h>

#include "extensions/contracts/IValidationProvider.h"
#include "extensions/contracts/IValidationProviderV2.h"
#include "extensions/contracts/ValidationProviderV1ToV2Adapter.h"
#include "models/GraphModel.h"
#include "graph.pb.h"

#include <memory>
#include <vector>

class ExtensionContractRegistry;

class ValidationService : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit ValidationService(QObject *parent = nullptr);

    void setValidationProviders(const QList<const IValidationProvider *> &providers);
    void setValidationProvidersV2(const QList<const IValidationProviderV2 *> &providers);
    void rebuildValidationFromRegistry(const ExtensionContractRegistry &registry);

    // Returns true if the graph has no structural errors.
    Q_INVOKABLE bool validate(GraphModel *graph);

    // Returns raw provider issues (QVariantMap entries with keys like
    // code, severity, message, entityType, entityId).
    Q_INVOKABLE QVariantList validationIssues(GraphModel *graph);

    // Returns a list of human-readable error messages, empty if valid.
    Q_INVOKABLE QStringList validationErrors(GraphModel *graph);

private:
    // Legacy: Returns QVariantMap snapshot for backward compatibility
    QVariantMap buildGraphSnapshot(GraphModel *graph) const;
    
    // Phase 5: Returns typed GraphSnapshot proto internally
    cme::GraphSnapshot buildTypedGraphSnapshot(GraphModel *graph) const;
    
    static bool issueIsError(const QVariantMap &issue);

    QList<const IValidationProviderV2 *> m_validationProviders;
    std::vector<std::unique_ptr<ValidationProviderV1ToV2Adapter>> m_validationV1Adapters;
};

#endif // VALIDATIONSERVICE_H
