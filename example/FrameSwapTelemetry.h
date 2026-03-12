#ifndef FRAMESWAPTELEMETRY_H
#define FRAMESWAPTELEMETRY_H

#include <QObject>
#include <QQmlEngine>
#include <QPointer>
#include <QVector>
#include <QElapsedTimer>

class QQuickWindow;

// FrameSwapTelemetry
// Measures time between consecutive QQuickWindow::frameSwapped signals using
// a monotonic high-resolution clock.
class FrameSwapTelemetry : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QObject *window READ window WRITE setWindow NOTIFY windowChanged FINAL)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(int maxSamples READ maxSamples WRITE setMaxSamples NOTIFY maxSamplesChanged FINAL)

    Q_PROPERTY(double frameSwapP50 READ frameSwapP50 NOTIFY statsChanged FINAL)
    Q_PROPERTY(double frameSwapP95 READ frameSwapP95 NOTIFY statsChanged FINAL)
    Q_PROPERTY(int frameSwapSampleCount READ frameSwapSampleCount NOTIFY statsChanged FINAL)

public:
    explicit FrameSwapTelemetry(QObject *parent = nullptr);

    QObject *window() const;
    void setWindow(QObject *windowObj);

    bool enabled() const;
    void setEnabled(bool value);

    int maxSamples() const;
    void setMaxSamples(int value);

    double frameSwapP50() const;
    double frameSwapP95() const;
    int frameSwapSampleCount() const;

    Q_INVOKABLE void reset();
    Q_INVOKABLE void computeStats();
    Q_INVOKABLE void report(const QString &tag = QString());

signals:
    void windowChanged();
    void enabledChanged();
    void maxSamplesChanged();
    void statsChanged();

private slots:
    void onFrameSwapped();

private:
    void connectWindow();
    void disconnectWindow();
    static double percentile(const QVector<double> &samples, double p);

    QPointer<QQuickWindow> m_window;
    bool m_enabled = false;
    int m_maxSamples = 4000;

    QElapsedTimer m_timer;
    qint64 m_lastNs = -1;

    QVector<double> m_samplesMs;

    double m_p50 = 0.0;
    double m_p95 = 0.0;
};

#endif // FRAMESWAPTELEMETRY_H
