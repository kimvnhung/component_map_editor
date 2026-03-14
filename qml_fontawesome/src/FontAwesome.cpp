#include "FontAwesome.h"

#include <QFontDatabase>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMutex>

namespace {

const QString kStyleSolid = QStringLiteral("solid");
const QString kStyleRegular = QStringLiteral("regular");
const QString kStyleBrands = QStringLiteral("brands");

const QString kFontSolidPath =
    QStringLiteral(":/qt/qml/QmlFontAwesome/fontawesome/otfs/Font Awesome 7 Free-Solid-900.otf");
const QString kFontRegularPath =
    QStringLiteral(":/qt/qml/QmlFontAwesome/fontawesome/otfs/Font Awesome 7 Free-Regular-400.otf");
const QString kFontBrandsPath =
    QStringLiteral(":/qt/qml/QmlFontAwesome/fontawesome/otfs/Font Awesome 7 Brands-Regular-400.otf");

} // namespace

FontAwesome::FontAwesome(QObject *parent)
    : QObject(parent)
{}

bool FontAwesome::ensureLoaded()
{
    QMutexLocker<QRecursiveMutex> locker(&sharedMutex());
    return loadLocked();
}

QString FontAwesome::icon(const QString &name, Style style) const
{
    return iconByStyleName(name, styleKey(style));
}

QString FontAwesome::iconByStyleName(const QString &name, const QString &styleName) const
{
    QMutexLocker<QRecursiveMutex> locker(&sharedMutex());
    if (!loadLocked())
        return QString();

    const QString normalizedName = normalizeIconName(name);
    const QString normalizedStyle = normalizeStyleName(styleName);

    QString glyph = glyphForNameAndStyle(normalizedName, normalizedStyle);
    if (!glyph.isEmpty())
        return glyph;

    return fallbackIcon(normalizedName);
}

bool FontAwesome::contains(const QString &name, Style style) const
{
    QMutexLocker<QRecursiveMutex> locker(&sharedMutex());
    if (!loadLocked())
        return false;

    const QString normalizedName = normalizeIconName(name);
    const QString normalizedStyle = styleKey(style);
    const auto styleIt = sharedData().glyphByStyle.constFind(normalizedStyle);
    if (styleIt == sharedData().glyphByStyle.constEnd())
        return false;

    return styleIt->contains(normalizedName);
}

QString FontAwesome::family(Style style) const
{
    QMutexLocker<QRecursiveMutex> locker(&sharedMutex());
    if (!loadLocked())
        return QString();

    return sharedData().familyByStyle.value(styleKey(style));
}

int FontAwesome::weight(Style style) const
{
    QMutexLocker<QRecursiveMutex> locker(&sharedMutex());
    if (!loadLocked())
        return 400;

    return sharedData().weightByStyle.value(styleKey(style), 400);
}

QStringList FontAwesome::availableStyles(const QString &name) const
{
    QMutexLocker<QRecursiveMutex> locker(&sharedMutex());
    if (!loadLocked())
        return {};

    const QString normalizedName = normalizeIconName(name);
    QStringList result;

    for (const QString &style : {kStyleSolid, kStyleRegular, kStyleBrands}) {
        const auto styleIt = sharedData().glyphByStyle.constFind(style);
        if (styleIt != sharedData().glyphByStyle.constEnd() && styleIt->contains(normalizedName))
            result.push_back(style);
    }

    return result;
}

int FontAwesome::iconCount() const
{
    QMutexLocker<QRecursiveMutex> locker(&sharedMutex());
    if (!loadLocked())
        return 0;

    QSet<QString> uniqueNames;
    for (const auto &styleMap : sharedData().glyphByStyle) {
        for (auto keyIt = styleMap.constKeyValueBegin(); keyIt != styleMap.constKeyValueEnd(); ++keyIt)
            uniqueNames.insert(keyIt->first);
    }

    return uniqueNames.size();
}

bool FontAwesome::initialize()
{
    QMutexLocker<QRecursiveMutex> locker(&sharedMutex());
    return loadLocked();
}

QString FontAwesome::iconForCpp(const QString &name, Style style)
{
    FontAwesome instance;
    return instance.icon(name, style);
}

QString FontAwesome::familyForCpp(Style style)
{
    FontAwesome instance;
    return instance.family(style);
}

int FontAwesome::weightForCpp(Style style)
{
    FontAwesome instance;
    return instance.weight(style);
}

FontAwesome::SharedData &FontAwesome::sharedData()
{
    static SharedData data;
    return data;
}

QRecursiveMutex &FontAwesome::sharedMutex()
{
    static QRecursiveMutex mutex;
    return mutex;
}

bool FontAwesome::loadLocked()
{
    if (sharedData().loaded)
        return true;

    const bool metadataLoaded = loadMetadataLocked();
    const bool fontsLoaded = loadFontsLocked();
    sharedData().loaded = metadataLoaded && fontsLoaded;
    return sharedData().loaded;
}

bool FontAwesome::loadMetadataLocked()
{
    QFile file(metadataPath());
    if (!file.open(QIODevice::ReadOnly))
        return false;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return false;

    const QJsonObject root = doc.object();
    auto &glyphByStyle = sharedData().glyphByStyle;

    for (auto it = root.constBegin(); it != root.constEnd(); ++it) {
        const QString iconName = normalizeIconName(it.key());
        const QJsonObject iconObject = it.value().toObject();
        const QString unicodeHex = iconObject.value(QStringLiteral("unicode")).toString();
        const QString glyph = codepointToString(unicodeHex);
        if (glyph.isEmpty())
            continue;

        const QJsonArray styles = iconObject.value(QStringLiteral("styles")).toArray();
        for (const QJsonValue &styleValue : styles) {
            const QString style = normalizeStyleName(styleValue.toString());
            if (style.isEmpty())
                continue;
            glyphByStyle[style].insert(iconName, glyph);
        }
    }

    return !glyphByStyle.isEmpty();
}

bool FontAwesome::loadFontsLocked()
{
    auto loadOne = [](const QString &resourcePath) -> QString {
        const int id = QFontDatabase::addApplicationFont(resourcePath);
        if (id < 0)
            return QString();

        const QStringList families = QFontDatabase::applicationFontFamilies(id);
        if (families.isEmpty())
            return QString();

        return families.first();
    };

    auto &families = sharedData().familyByStyle;
    auto &weights = sharedData().weightByStyle;

    const QString solidFamily = loadOne(kFontSolidPath);
    const QString regularFamily = loadOne(kFontRegularPath);
    const QString brandsFamily = loadOne(kFontBrandsPath);

    if (solidFamily.isEmpty() && regularFamily.isEmpty() && brandsFamily.isEmpty())
        return false;

    if (!solidFamily.isEmpty()) {
        families.insert(kStyleSolid, solidFamily);
        weights.insert(kStyleSolid, 900);
    }

    if (!regularFamily.isEmpty()) {
        families.insert(kStyleRegular, regularFamily);
        weights.insert(kStyleRegular, 400);
    }

    if (!brandsFamily.isEmpty()) {
        families.insert(kStyleBrands, brandsFamily);
        weights.insert(kStyleBrands, 400);
    }

    return !families.isEmpty();
}

QString FontAwesome::resourcePrefix()
{
    return QStringLiteral(":/qt/qml/QmlFontAwesome");
}

QString FontAwesome::metadataPath()
{
    return resourcePrefix() + QStringLiteral("/fontawesome/metadata/icons.json");
}

QString FontAwesome::styleKey(Style style)
{
    switch (style) {
    case Solid:
        return kStyleSolid;
    case Regular:
        return kStyleRegular;
    case Brands:
        return kStyleBrands;
    }

    return kStyleSolid;
}

QString FontAwesome::normalizeStyleName(const QString &styleName)
{
    const QString cleaned = styleName.trimmed().toLower();

    if (cleaned == QStringLiteral("solid") || cleaned == QStringLiteral("fas")
        || cleaned == QStringLiteral("fa-solid")) {
        return kStyleSolid;
    }

    if (cleaned == QStringLiteral("regular") || cleaned == QStringLiteral("far")
        || cleaned == QStringLiteral("fa-regular")) {
        return kStyleRegular;
    }

    if (cleaned == QStringLiteral("brands") || cleaned == QStringLiteral("fab")
        || cleaned == QStringLiteral("fa-brands")) {
        return kStyleBrands;
    }

    return QString();
}

QString FontAwesome::normalizeIconName(QString name)
{
    name = name.trimmed().toLower();
    name.replace(QLatin1Char('_'), QLatin1Char('-'));
    return name;
}

QString FontAwesome::codepointToString(const QString &unicodeHex)
{
    bool ok = false;
    const uint codepoint = unicodeHex.toUInt(&ok, 16);
    if (!ok || codepoint == 0 || codepoint > 0x10FFFFu)
        return QString();

    const char32_t scalar = static_cast<char32_t>(codepoint);
    return QString::fromUcs4(&scalar, 1);
}

QString FontAwesome::fallbackIcon(const QString &name)
{
    QString glyph = glyphForNameAndStyle(name, kStyleSolid);
    if (!glyph.isEmpty())
        return glyph;

    glyph = glyphForNameAndStyle(name, kStyleRegular);
    if (!glyph.isEmpty())
        return glyph;

    return glyphForNameAndStyle(name, kStyleBrands);
}

QString FontAwesome::glyphForNameAndStyle(const QString &name, const QString &styleKey)
{
    const auto styleIt = sharedData().glyphByStyle.constFind(styleKey);
    if (styleIt == sharedData().glyphByStyle.constEnd())
        return QString();

    return styleIt->value(name);
}
