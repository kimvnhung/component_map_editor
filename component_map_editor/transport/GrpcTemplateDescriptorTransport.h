#ifndef GRPCTEMPLATEDESCRIPTORTRANSPORT_H
#define GRPCTEMPLATEDESCRIPTORTRANSPORT_H

#include <QString>
#include <atomic>

#include "provider_templates.pb.h"

namespace cme::transport {

struct DescriptorTransportFeatureFlags {
    bool grpcTransportEnabled = false;
    bool fallbackToLocalOnGrpcFailure = true;

    // Enforced only for successful gRPC responses.
    bool enforceLatencySlo = true;
    int maxLatencyMs = 50;
};

struct DescriptorCallOptions {
    int timeoutMs = 0;
    std::atomic_bool *cancelRequested = nullptr;
};

enum class DescriptorCallStatus {
    Ok,
    Failed,
    Cancelled,
    DeadlineExceeded,
    Unavailable
};

enum class TemplateDescriptorKind {
    ComponentType,
    ConnectionPolicy,
    PropertySchema
};

struct TemplateDescriptorFetchRequest {
    TemplateDescriptorKind kind = TemplateDescriptorKind::ComponentType;
    QString providerId;
};

struct TemplateDescriptorFetchResponse {
    cme::templates::v1::ComponentTypeTemplateBundle componentTypeBundle;
    cme::templates::v1::ConnectionPolicyTemplateBundle connectionPolicyBundle;
    cme::templates::v1::PropertySchemaTemplateBundle propertySchemaBundle;
};

struct DescriptorTransportStats {
    int grpcCalls = 0;
    int localCalls = 0;
    int grpcFailures = 0;
    int fallbacks = 0;
    int latencySloViolations = 0;
    qint64 lastLatencyMs = 0;
};

class ITemplateDescriptorLocalSource
{
public:
    virtual ~ITemplateDescriptorLocalSource() = default;

    virtual bool fetch(const TemplateDescriptorFetchRequest &request,
                       TemplateDescriptorFetchResponse *out,
                       QString *error) = 0;
};

class IGrpcTemplateDescriptorClient
{
public:
    virtual ~IGrpcTemplateDescriptorClient() = default;

    virtual DescriptorCallStatus fetch(const TemplateDescriptorFetchRequest &request,
                                       const DescriptorCallOptions &options,
                                       TemplateDescriptorFetchResponse *out,
                                       QString *error) = 0;
};

class GrpcTemplateDescriptorTransportFacade
{
public:
    explicit GrpcTemplateDescriptorTransportFacade(ITemplateDescriptorLocalSource *localSource,
                                                   IGrpcTemplateDescriptorClient *grpcClient = nullptr);

    void setFeatureFlags(const DescriptorTransportFeatureFlags &flags);
    DescriptorTransportFeatureFlags featureFlags() const;

    bool fetch(const TemplateDescriptorFetchRequest &request,
               TemplateDescriptorFetchResponse *out,
               QString *error = nullptr,
               const DescriptorCallOptions &options = {});

    DescriptorTransportStats stats() const;

private:
    bool fetchLocal(const TemplateDescriptorFetchRequest &request,
                    TemplateDescriptorFetchResponse *out,
                    QString *error);

    void noteFallback();

    ITemplateDescriptorLocalSource *m_localSource = nullptr;
    IGrpcTemplateDescriptorClient *m_grpcClient = nullptr;
    DescriptorTransportFeatureFlags m_flags;
    DescriptorTransportStats m_stats;
};

} // namespace cme::transport

#endif // GRPCTEMPLATEDESCRIPTORTRANSPORT_H
