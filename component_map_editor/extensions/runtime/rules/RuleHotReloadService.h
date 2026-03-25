#ifndef RULEHOTRELOADSERVICE_H
#define RULEHOTRELOADSERVICE_H

#include <QObject>
#include <QFileSystemWatcher>

#include "RuleCompiler.h"
#include "RuleRuntimeRegistry.h"

class RuleHotReloadService : public QObject
{
public:
    explicit RuleHotReloadService(RuleRuntimeRegistry *registry,
                                  QObject *parent = nullptr);

    bool startWatchingFile(const QString &filePath);
    bool reloadNow();

    QVector<RuleDiagnostic> lastDiagnostics() const;
    QString watchedFilePath() const;

private:
    void onWatchedFileChanged(const QString &path);

    RuleRuntimeRegistry *m_registry = nullptr;
    QFileSystemWatcher m_watcher;
    QString m_filePath;
    RuleCompiler m_compiler;
    QVector<RuleDiagnostic> m_lastDiagnostics;
};

#endif // RULEHOTRELOADSERVICE_H
