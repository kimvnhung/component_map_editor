#ifndef ACTIONINVOCATIONSERVICE_H
#define ACTIONINVOCATIONSERVICE_H

#include <QHash>
#include <QObject>
#include <QString>
#include <QVariantMap>

#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/contracts/IActionProvider.h"
#include "public_api.pb.h"

class ActionInvocationService : public QObject
{
    Q_OBJECT

public:
    explicit ActionInvocationService(QObject *parent = nullptr);

    void setActionProviders(const QList<const IActionProvider *> &providers);
    void rebuildFromRegistry(const ExtensionContractRegistry &registry);

    // Typed external entrypoint. Preferred for integrations outside the library.
    bool invokeActionTyped(const cme::publicapi::v1::ActionInvokeRequest &request,
                           cme::publicapi::v1::ActionInvokeResponse *response,
                           QString *error = nullptr) const;

    // Legacy wrapper for QVariant-based action invocation.
    bool invokeAction(const QString &actionId,
                      const QVariantMap &context,
                      QVariantMap *commandRequest,
                      QString *error = nullptr) const;

private:
    const IActionProvider *providerForAction(const QString &actionId) const;

    QList<const IActionProvider *> m_providers;
    QHash<QString, const IActionProvider *> m_actionIndex;
};

#endif // ACTIONINVOCATIONSERVICE_H
