#ifndef EXTENSIONSTARTUPLOADER_H
#define EXTENSIONSTARTUPLOADER_H

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

#include <functional>
#include <memory>
#include <vector>

#include "extensions/contracts/ExtensionManifest.h"
#include "extensions/contracts/IExtensionPack.h"

class ExtensionContractRegistry;

struct ExtensionLoadDiagnostic
{
    enum class Severity {
        Info,
        Warning,
        Error
    };

    Severity severity = Severity::Info;
    QString extensionId;
    QString manifestPath;
    QString message;
};

struct ExtensionLoadResult
{
    int discoveredManifestCount = 0;
    int loadedPackCount = 0;
    QStringList loadedExtensionIds;
    QVector<ExtensionLoadDiagnostic> diagnostics;

    bool hasErrors() const;
};

class ExtensionStartupLoader
{
public:
    using PackFactory = std::function<std::unique_ptr<IExtensionPack>()>;

    void registerFactory(const QString &extensionId, PackFactory factory);
    void clearFactories();

    // Discovers *.json manifests under manifestDirectory recursively.
    // Valid packs are loaded while failures are reported in diagnostics.
    // The loader keeps loaded packs alive for registry pointer safety.
    ExtensionLoadResult loadFromDirectory(const QString &manifestDirectory,
                                          ExtensionContractRegistry &registry);

    int loadedPackCount() const;

private:
    struct DiscoveredManifest {
        ExtensionManifest manifest;
        QString path;
    };

    static QVector<QString> dependencyOrder(const QHash<QString, DiscoveredManifest> &manifests,
                                            QVector<ExtensionLoadDiagnostic> *diagnostics);

    QHash<QString, PackFactory> m_factories;
    std::vector<std::unique_ptr<IExtensionPack>> m_loadedPacks;
};

#endif // EXTENSIONSTARTUPLOADER_H
