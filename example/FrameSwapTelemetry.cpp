#include "FrameSwapTelemetry.h"

#include <QQuickWindow>
#include <QtAlgorithms>
#include <QDebug>

FrameSwapTelemetry::FrameSwapTelemetry(QObject *parent)
    : QObject(parent)
{
    m_timer.start();
}

QObject *FrameSwapTelemetry::window() const
{
    return m_window;
}

void FrameSwapTelemetry::setWindow(QObject *windowObj)
{
    QQuickWindow *nextWindow = qobject_cast<QQuickWindow *>(windowObj);
    if (m_window == nextWindow)
        return;

    // Always disconnect from the previous window (if any).
    disconnectWindow();
    m_window = nextWindow;

    // Only connect to frameSwapped when telemetry is enabled.
    if (m_window && m_enabled)
        connectWindow();

    emit windowChanged();
}

bool FrameSwapTelemetry::enabled() const
{
    return m_enabled;
}

void FrameSwapTelemetry::setEnabled(bool value)
{
    if (m_enabled == value)
        return;

    m_enabled = value;
    m_lastNs = -1;

    // Manage the frameSwapped connection based on the new enabled state.
    if (m_window) {
        if (m_enabled)
            connectWindow();
        else
            disconnectWindow();
    }

    emit enabledChanged();
}

int FrameSwapTelemetry::maxSamples() const
{
    return m_maxSamples;
}

void FrameSwapTelemetry::setMaxSamples(int value)
{
    if (value <= 0 || m_maxSamples == value)
        return;

    m_maxSamples = value;
    while (m_samplesMs.size() > m_maxSamples)
        m_samplesMs.removeFirst();
    emit maxSamplesChanged();
}

double FrameSwapTelemetry::frameSwapP50() const
{
    return m_p50;
}

double FrameSwapTelemetry::frameSwapP95() const
{
    return m_p95;
}

int FrameSwapTelemetry::frameSwapSampleCount() const
{
    return m_samplesMs.size();
}

void FrameSwapTelemetry::reset()
{
    m_samplesMs.clear();
    m_p50 = 0.0;
    m_p95 = 0.0;
    m_lastNs = -1;
    emit statsChanged();
}

void FrameSwapTelemetry::computeStats()
{
    m_p50 = percentile(m_samplesMs, 0.50);
    m_p95 = percentile(m_samplesMs, 0.95);
    emit statsChanged();
}

void FrameSwapTelemetry::report(const QString &tag)
{
    computeStats();

    const QString reportTag = tag.isEmpty() ? QStringLiteral("Baseline Report") : tag;
    qDebug().noquote() << "=======================================================";
    qDebug().noquote() << "[FrameSwapTelemetry]" << reportTag;
    qDebug().noquote() << QStringLiteral("  Frame swap interval   p50=%1 ms   p95=%2 ms   n=%3")
                              .arg(QString::number(m_p50, 'f', 2),
                                   QString::number(m_p95, 'f', 2),
                                   QString::number(m_samplesMs.size()));
    qDebug().noquote() << "=======================================================";
}

void FrameSwapTelemetry::onFrameSwapped()
{
    if (!m_enabled)
        return;

    const qint64 nowNs = m_timer.nsecsElapsed();
    if (m_lastNs >= 0) {
        const double dtMs = static_cast<double>(nowNs - m_lastNs) / 1000000.0;
        // Ignore unrealistic outliers caused by long app stalls or startup.
        if (dtMs > 0.0 && dtMs < 500.0) {
            m_samplesMs.push_back(dtMs);
            if (m_samplesMs.size() > m_maxSamples)
                m_samplesMs.removeFirst();
        }
    }
    m_lastNs = nowNs;
}

void FrameSwapTelemetry::connectWindow()
{
    if (!m_window)
        return;

    connect(m_window, &QQuickWindow::frameSwapped,
            this, &FrameSwapTelemetry::onFrameSwapped,
            Qt::UniqueConnection);
}

void FrameSwapTelemetry::disconnectWindow()
{
    if (!m_window)
        return;

    disconnect(m_window, &QQuickWindow::frameSwapped,
               this, &FrameSwapTelemetry::onFrameSwapped);
}

double FrameSwapTelemetry::percentile(const QVector<double> &samples, double p)
{
    if (samples.isEmpty())
        return 0.0;

    QVector<double> sorted = samples;
    std::sort(sorted.begin(), sorted.end());

    const int idx = qBound(0, static_cast<int>(p * (sorted.size() - 1)), sorted.size() - 1);
    return sorted.at(idx);
}
