#include "component_map_runtime/RuntimeService.h"

namespace cme::runtime
{

RuntimeService::RuntimeService(const cme::runtime::v1::RuntimeDescriptor &descriptor)
    : m_descriptor(descriptor)
{
}

const cme::runtime::v1::RuntimeDescriptor &RuntimeService::descriptor() const
{
    return m_descriptor;
}

cme::runtime::v1::RuntimeStatus RuntimeService::status() const
{
    return m_status;
}

QString RuntimeService::lastError() const
{
    return m_lastError;
}

bool RuntimeService::validateConfiguration(QString *error) const
{
    if (m_descriptor.runtime_id().empty())
    {
        if (error)
        {
            *error = QStringLiteral("Runtime ID must not be empty.");
        }
        return false;
    }

    if (m_descriptor.protocol_version().empty())
    {
        if (error)
        {
            *error = QStringLiteral("Runtime protocol version must not be empty.");
        }
        return false;
    }

    if (m_descriptor.mode() == cme::runtime::v1::RUNTIME_MODE_UNSPECIFIED)
    {
        if (error)
        {
            *error = QStringLiteral("Runtime mode must be specified.");
        }
        return false;
    }

    if (error)
    {
        error->clear();
    }
    return true;
}

bool RuntimeService::start(QString *error)
{
    if (m_status == cme::runtime::v1::RUNTIME_STATUS_RUNNING)
    {
        if (error)
        {
            error->clear();
        }
        return true;
    }

    QString validationError;
    if (!validateConfiguration(&validationError))
    {
        setFailure(validationError, error);
        return false;
    }

    m_lastError.clear();
    m_status = cme::runtime::v1::RUNTIME_STATUS_STARTING;

    // Phase 1 establishes lifecycle and build boundaries only. Event dispatch,
    // persistence, IPC, and worker scheduling are intentionally not started here.
    m_status = cme::runtime::v1::RUNTIME_STATUS_RUNNING;
    if (error)
    {
        error->clear();
    }
    return true;
}

bool RuntimeService::stop(QString *error)
{
    if (m_status == cme::runtime::v1::RUNTIME_STATUS_STOPPED)
    {
        if (error)
        {
            error->clear();
        }
        return true;
    }

    m_status = cme::runtime::v1::RUNTIME_STATUS_STOPPING;
    m_status = cme::runtime::v1::RUNTIME_STATUS_STOPPED;
    m_lastError.clear();
    if (error)
    {
        error->clear();
    }
    return true;
}

cme::runtime::v1::RuntimeStatusSnapshot RuntimeService::statusSnapshot() const
{
    cme::runtime::v1::RuntimeStatusSnapshot snapshot;
    *snapshot.mutable_runtime_descriptor() = m_descriptor;
    snapshot.set_status(m_status);
    snapshot.set_message(m_lastError.toStdString());
    return snapshot;
}

void RuntimeService::setFailure(const QString &message, QString *error)
{
    m_lastError = message;
    m_status = cme::runtime::v1::RUNTIME_STATUS_FAILED;
    if (error)
    {
        *error = message;
    }
}

} // namespace cme::runtime
