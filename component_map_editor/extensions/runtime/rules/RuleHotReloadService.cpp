#include "RuleHotReloadService.h"

#include <QFileInfo>

RuleHotReloadService::RuleHotReloadService(RuleRuntimeRegistry *registry,
                                           QObject *parent)
    : QObject(parent)
    , m_registry(registry)
{
    connect(&m_watcher,
            &QFileSystemWatcher::fileChanged,
            this,
            &RuleHotReloadService::onWatchedFileChanged);
}

bool RuleHotReloadService::startWatchingFile(const QString &filePath)
{
    m_filePath = QFileInfo(filePath).absoluteFilePath();

    const QStringList watchedFiles = m_watcher.files();
    if (!watchedFiles.isEmpty())
        m_watcher.removePaths(watchedFiles);

    if (!m_filePath.isEmpty())
        m_watcher.addPath(m_filePath);

    return reloadNow();
}

bool RuleHotReloadService::reloadNow()
{
    if (!m_registry || m_filePath.isEmpty())
        return false;

    const RuleCompileResult result = m_compiler.compileFromFile(m_filePath);
    m_lastDiagnostics = result.diagnostics;

    QVector<RuleDiagnostic> failureDiagnostics;
    if (!m_registry->applyCompileResult(result, &failureDiagnostics)) {
        return false;
    }

    return true;
}

QVector<RuleDiagnostic> RuleHotReloadService::lastDiagnostics() const
{
    return m_lastDiagnostics;
}

QString RuleHotReloadService::watchedFilePath() const
{
    return m_filePath;
}

void RuleHotReloadService::onWatchedFileChanged(const QString &path)
{
    if (!path.isEmpty() && !m_watcher.files().contains(path))
        m_watcher.addPath(path);

    reloadNow();
}
