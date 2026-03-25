#ifndef VALIDATIONSERVICE_H
#define VALIDATIONSERVICE_H

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <qqml.h>

#include "extensions/contracts/IValidationProvider.h"
#include "models/GraphModel.h"

class ExtensionContractRegistry;

class ValidationService : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit ValidationService(QObject *parent = nullptr);

    void setValidationProviders(const QList<const IValidationProvider *> &providers);
    void rebuildValidationFromRegistry(const ExtensionContractRegistry &registry);

    // Returns true if the graph has no structural errors.
    Q_INVOKABLE bool validate(GraphModel *graph);

    // Returns raw provider issues (QVariantMap entries with keys like
    // code, severity, message, entityType, entityId).
    Q_INVOKABLE QVariantList validationIssues(GraphModel *graph);

    // Returns a list of human-readable error messages, empty if valid.
    Q_INVOKABLE QStringList validationErrors(GraphModel *graph);

private:
    QVariantMap buildGraphSnapshot(GraphModel *graph) const;
    static bool issueIsError(const QVariantMap &issue);

    QList<const IValidationProvider *> m_validationProviders;
};

#endif // VALIDATIONSERVICE_H
