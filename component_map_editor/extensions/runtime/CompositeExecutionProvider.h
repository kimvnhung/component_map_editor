#ifndef COMPOSITEEXECUTIONPROVIDER_H
#define COMPOSITEEXECUTIONPROVIDER_H

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariantMap>

#include "extensions/contracts/IExecutionSemanticsProvider.h"
#include "models/ConnectionModel.h"

namespace cme::runtime {

struct CompositeInputPortMapping {
    QString outerTokenKey;
    QString innerEntryComponentId;
    QString innerPortKey;
};

struct CompositeOutputPortMapping {
    QString innerExitComponentId;
    QString innerPortKey;
    QString outerPayloadKey;
};

struct CompositeComponentSpec {
    QString id;
    QString type;
    QString title;
    QVariantMap attributes;
};

struct CompositeConnectionSpec {
    QString id;
    QString sourceId;
    QString targetId;
    QString label;
    ConnectionModel::Side sourceSide = ConnectionModel::SideAuto;
    ConnectionModel::Side targetSide = ConnectionModel::SideAuto;
};

struct CompositeGraphDefinition {
    QString componentTypeId;
    QList<CompositeComponentSpec> components;
    QList<CompositeConnectionSpec> connections;
    QList<CompositeInputPortMapping> inputMappings;
    QList<CompositeOutputPortMapping> outputMappings;
};

class CompositeExecutionProvider : public IExecutionSemanticsProvider
{
public:
    static QString entryComponentType();
    static QString exitComponentType();

    CompositeExecutionProvider();

    void setDefinitions(const QList<CompositeGraphDefinition> &definitions);
    void setDelegateProviders(const QList<const IExecutionSemanticsProvider *> &providers);

    bool isValid() const;
    QStringList diagnostics() const;

    QString providerId() const override;
    QStringList supportedComponentTypes() const override;

    bool executeComponent(const QString &componentType,
                          const QString &componentId,
                          const QVariantMap &componentSnapshot,
                          const QVariantMap &inputState,
                          QVariantMap *outputState,
                          QVariantMap *trace,
                          QString *error) const override;

    bool executeComponentV2(const QString &componentType,
                            const QString &componentId,
                            const QVariantMap &componentSnapshot,
                            const cme::execution::IncomingTokens &incomingTokens,
                            cme::execution::ExecutionPayload *outputPayload,
                            QVariantMap *trace,
                            QString *error) const override;

private:
    void validateDefinitions();
    bool validateDefinitionGraph(QStringList *diagnostics) const;
    bool executeEntryComponent(const QString &componentId,
                               const QVariantMap &componentSnapshot,
                               const cme::execution::IncomingTokens &incomingTokens,
                               cme::execution::ExecutionPayload *outputPayload,
                               QVariantMap *trace,
                               QString *error) const;
    bool executeExitComponent(const QString &componentId,
                              const QVariantMap &componentSnapshot,
                              const cme::execution::IncomingTokens &incomingTokens,
                              cme::execution::ExecutionPayload *outputPayload,
                              QVariantMap *trace,
                              QString *error) const;
    bool executeCompositeComponent(const CompositeGraphDefinition &definition,
                                   const QString &componentId,
                                   const cme::execution::IncomingTokens &incomingTokens,
                                   cme::execution::ExecutionPayload *outputPayload,
                                   QVariantMap *trace,
                                   QString *error) const;

    QHash<QString, CompositeGraphDefinition> m_definitionsByType;
    QList<const IExecutionSemanticsProvider *> m_delegateProviders;
    QStringList m_diagnostics;
    bool m_valid = true;
};

} // namespace cme::runtime

#endif // COMPOSITEEXECUTIONPROVIDER_H
