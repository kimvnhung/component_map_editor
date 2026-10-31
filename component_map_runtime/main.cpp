#include <QCoreApplication>
#include <QDebug>

#include "component_map_runtime/RuntimeService.h"

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);

    cme::runtime::v1::RuntimeDescriptor descriptor;
    descriptor.set_runtime_id(QStringLiteral("component-map-runtime").toStdString());
    descriptor.set_protocol_version(QStringLiteral("1.0").toStdString());
    descriptor.set_mode(cme::runtime::v1::RUNTIME_MODE_BACKGROUND);

    cme::runtime::RuntimeService service(descriptor);
    QString error;
    if (!service.start(&error))
    {
        qCritical().noquote() << error;
        return 1;
    }

    qInfo().noquote() << "component_map_runtime process skeleton is running."
                      << "Event workers and IPC are not enabled in Phase 1.";
    service.stop();
    return 0;
}
