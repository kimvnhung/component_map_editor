#ifndef MEMORYMONITOR_H
#define MEMORYMONITOR_H

#include <QObject>
#include <QQmlEngine>

// MemoryMonitor — exposes process memory stats to QML.
// On Linux reads /proc/self/status; returns -1 on unsupported platforms.
//
// QML usage:
//   MemoryMonitor { id: mem }
//   Label { text: (mem.rssKb() / 1024).toFixed(1) + " MB" }
class MemoryMonitor : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit MemoryMonitor(QObject *parent = nullptr);

    // Current resident-set size in kB.
    Q_INVOKABLE qint64 rssKb() const;

    // Peak virtual memory size in kB.
    Q_INVOKABLE qint64 vmPeakKb() const;

private:
    static qint64 readProcStatusKb(const char *field);
};

#endif // MEMORYMONITOR_H
