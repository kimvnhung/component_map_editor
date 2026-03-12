#include "MemoryMonitor.h"

#ifdef Q_OS_LINUX
#include <QFile>
#endif

MemoryMonitor::MemoryMonitor(QObject *parent)
    : QObject(parent)
{}

qint64 MemoryMonitor::readProcStatusKb(const char *field)
{
#ifdef Q_OS_LINUX
    QFile f(QStringLiteral("/proc/self/status"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return -1;

    const QByteArray prefix = QByteArray(field) + ':';
    while (!f.atEnd()) {
        const QByteArray line = f.readLine();
        if (!line.startsWith(prefix))
            continue;

        // Line format: "VmRSS:\t 12345 kB\n"
        const QByteArray rest = line.mid(prefix.size()).trimmed();
        const int spaceIdx    = rest.indexOf(' ');
        const QByteArray num  = (spaceIdx >= 0) ? rest.left(spaceIdx) : rest;
        bool ok               = false;
        const qint64 val      = num.toLongLong(&ok);
        return ok ? val : -1;
    }
    return -1;
#else
    Q_UNUSED(field)
    return -1;
#endif
}

qint64 MemoryMonitor::rssKb() const
{
    return readProcStatusKb("VmRSS");
}

qint64 MemoryMonitor::vmPeakKb() const
{
    return readProcStatusKb("VmPeak");
}
