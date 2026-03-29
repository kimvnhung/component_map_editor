#ifndef VALIDATIONPROVIDERV1TOV2ADAPTER_H
#define VALIDATIONPROVIDERV1TOV2ADAPTER_H

#include "IValidationProvider.h"
#include "IValidationProviderV2.h"

class ValidationProviderV1ToV2Adapter : public IValidationProviderV2
{
public:
    explicit ValidationProviderV1ToV2Adapter(const IValidationProvider *legacyProvider);

    QString providerId() const override;

    bool validateGraph(const cme::GraphSnapshot &graphSnapshot,
                       cme::GraphValidationResult *outResult,
                       QString *error) const override;

private:
    const IValidationProvider *m_legacyProvider = nullptr;
};

#endif // VALIDATIONPROVIDERV1TOV2ADAPTER_H
