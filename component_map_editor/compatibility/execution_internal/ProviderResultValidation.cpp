#include "ProviderResultValidation.h"

namespace cme::execution
{

bool validateExecutionResult(const ExecuteResult &result, QString &message)
{
    // Validate that outputPayload contains at least one of the provider's
    // declared output keys. Also accept any keys explicitly configured
    // in the component's snapshot (e.g. outputKey="mySum"), since those
    // override the default declared names.
    const QStringList declared = result.requiredOutputKeys;

    if (declared.isEmpty())
    {
        return true;
    }

    bool matched = false;

    for (const QString &key : declared)
    {
        if (result.output.contains(key))
        {
            matched = true;
            break;
        }
    }

    if (!matched)
    {
        // Fall back: check any snapshot-level output-key property
        static const QStringList kOutputKeyProps =
        {
            QStringLiteral("outputKey"),
            QStringLiteral("trueRouteKey"),
            QStringLiteral("falseRouteKey"),
            QStringLiteral("iterKey"),
            QStringLiteral("continueKey"),
            QStringLiteral("errorKey")
        };

        const auto &properties = result.componentData.properties();

        for (const QString &prop : kOutputKeyProps)
        {
            auto it = properties.find(prop.toStdString());

            if (it != properties.end())
            {
                const QString configured = QString::fromStdString(it->second);

                if (!configured.isEmpty() && result.output.contains(configured))
                {
                    matched = true;
                    break;
                }
            }
        }
    }

    if (!matched)
    {
        message = QString("Provider '%1': output payload for type '%2' is missing all declared keys [%3].")
                  .arg(result.providerId, result.componentData.type_id().c_str(), declared.join(QStringLiteral(", ")));
    }

    return matched;
}

} // namespace cme::execution
