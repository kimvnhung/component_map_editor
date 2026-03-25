#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

#include "extensions/runtime/ExtensionCompatibilityChecker.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QTextStream out(stdout);
    QTextStream err(stderr);

    const QStringList args = app.arguments();
    if (args.size() < 2) {
        err << "Usage: compatibility_checker_tool <manifest.json> [core-api-version]\n";
        return 2;
    }

    const QString manifestPath = args.at(1);
    const QString coreVersionText = (args.size() >= 3) ? args.at(2) : QStringLiteral("1.0.0");

    bool versionOk = false;
    const ExtensionApiVersion coreVersion = ExtensionApiVersion::parse(coreVersionText, &versionOk);
    if (!versionOk) {
        err << "Invalid core API version: " << coreVersionText << "\n";
        return 2;
    }

    ExtensionCompatibilityChecker checker(coreVersion);
    QVariantMap report;
    QString error;
    if (!checker.analyzeManifestFile(manifestPath, &report, &error)) {
        err << "Compatibility check failed: " << error << "\n";
        return 1;
    }

    out << QJsonDocument::fromVariant(report).toJson(QJsonDocument::Indented);
    return 0;
}
