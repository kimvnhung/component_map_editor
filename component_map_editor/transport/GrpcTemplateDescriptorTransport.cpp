#include "GrpcTemplateDescriptorTransport.h"

#include <QElapsedTimer>

namespace cme::transport {

GrpcTemplateDescriptorTransportFacade::GrpcTemplateDescriptorTransportFacade(
    ITemplateDescriptorLocalSource *localSource,
    IGrpcTemplateDescriptorClient *grpcClient)
    : m_localSource(localSource)
    , m_grpcClient(grpcClient)
{}

void GrpcTemplateDescriptorTransportFacade::setFeatureFlags(const DescriptorTransportFeatureFlags &flags)
{
    m_flags = flags;
}

DescriptorTransportFeatureFlags GrpcTemplateDescriptorTransportFacade::featureFlags() const
{
    return m_flags;
}

bool GrpcTemplateDescriptorTransportFacade::fetch(const TemplateDescriptorFetchRequest &request,
                                                  TemplateDescriptorFetchResponse *out,
                                                  QString *error,
                                                  const DescriptorCallOptions &options)
{
    if (options.cancelRequested && options.cancelRequested->load()) {
        if (error)
            *error = QStringLiteral("gRPC template descriptor fetch cancelled");
        return false;
    }

    if (out)
        *out = TemplateDescriptorFetchResponse{};

    if (!m_flags.grpcTransportEnabled || !m_grpcClient)
        return fetchLocal(request, out, error);

    ++m_stats.grpcCalls;

    QElapsedTimer timer;
    timer.start();

    QString grpcError;
    const DescriptorCallStatus status = m_grpcClient->fetch(request, options, out, &grpcError);

    m_stats.lastLatencyMs = timer.elapsed();

    switch (status) {
    case DescriptorCallStatus::Ok:
        if (m_flags.enforceLatencySlo
            && m_flags.maxLatencyMs > 0
            && m_stats.lastLatencyMs > m_flags.maxLatencyMs) {
            ++m_stats.latencySloViolations;
            if (m_flags.fallbackToLocalOnGrpcFailure) {
                noteFallback();
                return fetchLocal(request, out, error);
            }
            if (error) {
                *error = QStringLiteral("gRPC template descriptor latency SLO exceeded (%1ms > %2ms)")
                             .arg(m_stats.lastLatencyMs)
                             .arg(m_flags.maxLatencyMs);
            }
            return false;
        }
        return true;

    case DescriptorCallStatus::Cancelled:
        if (error)
            *error = grpcError.isEmpty() ? QStringLiteral("gRPC descriptor call cancelled") : grpcError;
        return false;

    case DescriptorCallStatus::DeadlineExceeded:
        if (error)
            *error = grpcError.isEmpty() ? QStringLiteral("gRPC descriptor deadline exceeded") : grpcError;
        return false;

    case DescriptorCallStatus::Failed:
    case DescriptorCallStatus::Unavailable:
        ++m_stats.grpcFailures;
        if (m_flags.fallbackToLocalOnGrpcFailure) {
            noteFallback();
            return fetchLocal(request, out, error);
        }
        if (error)
            *error = grpcError.isEmpty() ? QStringLiteral("gRPC descriptor transport unavailable") : grpcError;
        return false;
    }

    if (error)
        *error = QStringLiteral("gRPC descriptor transport returned unknown status");
    return false;
}

DescriptorTransportStats GrpcTemplateDescriptorTransportFacade::stats() const
{
    return m_stats;
}

bool GrpcTemplateDescriptorTransportFacade::fetchLocal(const TemplateDescriptorFetchRequest &request,
                                                       TemplateDescriptorFetchResponse *out,
                                                       QString *error)
{
    if (!m_localSource) {
        if (error)
            *error = QStringLiteral("Local template descriptor source is not configured");
        return false;
    }

    if (out)
        *out = TemplateDescriptorFetchResponse{};

    ++m_stats.localCalls;
    return m_localSource->fetch(request, out, error);
}

void GrpcTemplateDescriptorTransportFacade::noteFallback()
{
    ++m_stats.fallbacks;
}

} // namespace cme::transport
