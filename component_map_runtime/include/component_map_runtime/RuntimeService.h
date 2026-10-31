#ifndef COMPONENT_MAP_RUNTIME_RUNTIME_SERVICE_H
#define COMPONENT_MAP_RUNTIME_RUNTIME_SERVICE_H

#include <QString>

#include <runtime.pb.h>

namespace cme::runtime
{

class RuntimeService
{
public:
    explicit RuntimeService(const cme::runtime::v1::RuntimeDescriptor &descriptor = {});

    const cme::runtime::v1::RuntimeDescriptor &descriptor() const;
    cme::runtime::v1::RuntimeStatus status() const;
    QString lastError() const;

    bool validateConfiguration(QString *error = nullptr) const;
    bool start(QString *error = nullptr);
    bool stop(QString *error = nullptr);

    cme::runtime::v1::RuntimeStatusSnapshot statusSnapshot() const;

private:
    void setFailure(const QString &message, QString *error);

    cme::runtime::v1::RuntimeDescriptor m_descriptor;
    cme::runtime::v1::RuntimeStatus m_status = cme::runtime::v1::RUNTIME_STATUS_STOPPED;
    QString m_lastError;
};

} // namespace cme::runtime

#endif // COMPONENT_MAP_RUNTIME_RUNTIME_SERVICE_H
