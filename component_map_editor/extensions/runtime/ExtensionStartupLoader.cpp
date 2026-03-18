#include "ExtensionStartupLoader.h"

#include <QDirIterator>
#include <QFileInfo>
#include <QQueue>
#include <QSet>

#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/runtime/ExtensionManifestJson.h"

bool ExtensionLoadResult::hasErrors() const
{
    for (const ExtensionLoadDiagnostic &d : diagnostics) {
        if (d.severity == ExtensionLoadDiagnostic::Severity::Error) {
            return true;
        }
    }
    return false;
}

void ExtensionStartupLoader::registerFactory(const QString &extensionId, PackFactory factory)
{
    m_factories.insert(extensionId.trimmed(), std::move(factory));
}

void ExtensionStartupLoader::clearFactories()
{
    m_factories.clear();
}

int ExtensionStartupLoader::loadedPackCount() const
{
    return m_loadedPacks.size();
}

ExtensionLoadResult ExtensionStartupLoader::loadFromDirectory(const QString &manifestDirectory,
                                                              ExtensionContractRegistry &registry)
{
    ExtensionLoadResult result;

    QHash<QString, DiscoveredManifest> manifests;

    QDirIterator it(manifestDirectory,
                    QStringList{ QStringLiteral("*.json") },
                    QDir::Files,
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
        const QString path = it.next();
        const QFileInfo fileInfo(path);
        const QString baseName = fileInfo.fileName();

        // Rule DSL files can live beside manifests in development builds.
        // They are not extension manifests and should not be parsed here.
        if (baseName.startsWith(QStringLiteral("rules."), Qt::CaseInsensitive))
            continue;

        ++result.discoveredManifestCount;

        ExtensionManifest manifest;
        QString parseError;
        if (!ExtensionManifestJson::parseFile(path, &manifest, &parseError)) {
            result.diagnostics.append({
                ExtensionLoadDiagnostic::Severity::Error,
                QString(),
                path,
                QStringLiteral("Manifest parse failed: %1").arg(parseError)
            });
            continue;
        }

        if (manifests.contains(manifest.extensionId)) {
            result.diagnostics.append({
                ExtensionLoadDiagnostic::Severity::Error,
                manifest.extensionId,
                path,
                QStringLiteral("Duplicate extensionId '%1' discovered in manifests.")
                    .arg(manifest.extensionId)
            });
            continue;
        }

        manifests.insert(manifest.extensionId, { manifest, path });
    }

    const QVector<QString> orderedIds = dependencyOrder(manifests, &result.diagnostics);
    for (const QString &extensionId : orderedIds) {
        const DiscoveredManifest discovered = manifests.value(extensionId);

        const auto factoryIt = m_factories.constFind(extensionId);
        if (factoryIt == m_factories.constEnd()) {
            result.diagnostics.append({
                ExtensionLoadDiagnostic::Severity::Error,
                extensionId,
                discovered.path,
                QStringLiteral("No pack factory registered for extensionId '%1'.")
                    .arg(extensionId)
            });
            continue;
        }

        QString registrationError;
        if (!registry.registerManifest(discovered.manifest, &registrationError)) {
            result.diagnostics.append({
                ExtensionLoadDiagnostic::Severity::Error,
                extensionId,
                discovered.path,
                QStringLiteral("Manifest rejected: %1").arg(registrationError)
            });
            continue;
        }

        std::unique_ptr<IExtensionPack> pack = (*factoryIt)();
        if (!pack) {
            result.diagnostics.append({
                ExtensionLoadDiagnostic::Severity::Error,
                extensionId,
                discovered.path,
                QStringLiteral("Pack factory returned null instance for '%1'.")
                    .arg(extensionId)
            });
            continue;
        }

        if (!pack->registerProviders(registry, &registrationError)) {
            result.diagnostics.append({
                ExtensionLoadDiagnostic::Severity::Error,
                extensionId,
                discovered.path,
                QStringLiteral("Provider registration failed: %1").arg(registrationError)
            });
            continue;
        }

        m_loadedPacks.push_back(std::move(pack));
        ++result.loadedPackCount;
        result.loadedExtensionIds.append(extensionId);
        result.diagnostics.append({
            ExtensionLoadDiagnostic::Severity::Info,
            extensionId,
            discovered.path,
            QStringLiteral("Pack loaded successfully.")
        });
    }

    return result;
}

QVector<QString> ExtensionStartupLoader::dependencyOrder(
    const QHash<QString, DiscoveredManifest> &manifests,
    QVector<ExtensionLoadDiagnostic> *diagnostics)
{
    QHash<QString, int> indegree;
    QHash<QString, QStringList> edges;
    QSet<QString> blockedIds;

    for (auto it = manifests.constBegin(); it != manifests.constEnd(); ++it) {
        const QString id = it.key();
        indegree.insert(id, 0);
    }

    for (auto it = manifests.constBegin(); it != manifests.constEnd(); ++it) {
        const QString id = it.key();
        const DiscoveredManifest discovered = it.value();

        for (const QString &depId : discovered.manifest.dependencies) {
            if (!manifests.contains(depId)) {
                blockedIds.insert(id);
                if (diagnostics) {
                    diagnostics->append({
                        ExtensionLoadDiagnostic::Severity::Error,
                        id,
                        discovered.path,
                        QStringLiteral("Missing dependency '%1' for extension '%2'.")
                            .arg(depId, id)
                    });
                }
                continue;
            }

            edges[depId].append(id);
            indegree[id] = indegree.value(id) + 1;
        }
    }

    QQueue<QString> queue;
    for (auto it = indegree.constBegin(); it != indegree.constEnd(); ++it) {
        if (it.value() == 0 && !blockedIds.contains(it.key())) {
            queue.enqueue(it.key());
        }
    }

    QVector<QString> ordered;
    while (!queue.isEmpty()) {
        const QString current = queue.dequeue();
        ordered.append(current);

        for (const QString &next : edges.value(current)) {
            indegree[next] = indegree.value(next) - 1;
            if (indegree[next] == 0 && !blockedIds.contains(next)) {
                queue.enqueue(next);
            }
        }
    }

    for (auto it = indegree.constBegin(); it != indegree.constEnd(); ++it) {
        const QString id = it.key();
        if (it.value() > 0 && !blockedIds.contains(id)) {
            if (diagnostics) {
                diagnostics->append({
                    ExtensionLoadDiagnostic::Severity::Error,
                    id,
                    manifests.value(id).path,
                    QStringLiteral("Dependency cycle detected involving '%1'.")
                        .arg(id)
                });
            }
        }
    }

    return ordered;
}
