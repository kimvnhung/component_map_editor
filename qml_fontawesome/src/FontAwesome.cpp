#include "FontAwesome.h"

#include <QFontDatabase>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMutex>
#include <QSettings>

namespace {

const QString kStyleSolid = QStringLiteral("solid");
const QString kStyleRegular = QStringLiteral("regular");
const QString kStyleBrands = QStringLiteral("brands");
const QString kSettingsGroup = QStringLiteral("QmlFontAwesome");
const QString kSettingsFavoritesKey = QStringLiteral("favorites");
const QString kSettingsRecentsKey = QStringLiteral("recents");
const int kMaxRecentIcons = 30;

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

QStringList FontAwesome::searchIcons(const QString &keyword, int limit) const
{
    QMutexLocker<QRecursiveMutex> locker(&sharedMutex());
    if (!loadLocked())
        return {};

    const QString key = normalizeIconName(keyword);
    QStringList result;
    if (limit <= 0)
        return result;

    for (auto it = sharedData().searchTermsByIcon.constBegin();
         it != sharedData().searchTermsByIcon.constEnd();
         ++it) {
        if (result.size() >= limit)
            break;

        const QString iconName = it.key();
        const QStringList terms = it.value();
        bool matched = key.isEmpty() || iconName.contains(key);
        if (!matched) {
            for (const QString &term : terms) {
                if (term.contains(key)) {
                    matched = true;
                    break;
                }
            }
        }

        if (matched)
            result.push_back(iconName);
    }

    result.sort(Qt::CaseInsensitive);
    if (result.size() > limit)
        result = result.mid(0, limit);

    return result;
}

QStringList FontAwesome::favoriteIcons() const
{
    QMutexLocker<QRecursiveMutex> locker(&sharedMutex());
    if (!loadLocked())
        return {};
    return sharedData().favorites;
}

QStringList FontAwesome::recentIcons() const
{
    QMutexLocker<QRecursiveMutex> locker(&sharedMutex());
    if (!loadLocked())
        return {};
    return sharedData().recents;
}

bool FontAwesome::isFavorite(const QString &name) const
{
    QMutexLocker<QRecursiveMutex> locker(&sharedMutex());
    if (!loadLocked())
        return false;
    return sharedData().favorites.contains(normalizeIconName(name));
}

void FontAwesome::addFavorite(const QString &name)
{
    QMutexLocker<QRecursiveMutex> locker(&sharedMutex());
    if (!loadLocked())
        return;

    const QString iconName = normalizeIconName(name);
    if (iconName.isEmpty() || !containsIconAnyStyle(iconName))
        return;

    if (!sharedData().favorites.contains(iconName)) {
        sharedData().favorites.prepend(iconName);
        saveFavoritesLocked();
    }
}

void FontAwesome::removeFavorite(const QString &name)
{
    QMutexLocker<QRecursiveMutex> locker(&sharedMutex());
    if (!loadLocked())
        return;

    const QString iconName = normalizeIconName(name);
    sharedData().favorites.removeAll(iconName);
    saveFavoritesLocked();
}

void FontAwesome::clearFavorites()
{
    QMutexLocker<QRecursiveMutex> locker(&sharedMutex());
    if (!loadLocked())
        return;

    sharedData().favorites.clear();
    saveFavoritesLocked();
}

void FontAwesome::markIconUsed(const QString &name)
{
    QMutexLocker<QRecursiveMutex> locker(&sharedMutex());
    if (!loadLocked())
        return;

    const QString iconName = normalizeIconName(name);
    if (iconName.isEmpty() || !containsIconAnyStyle(iconName))
        return;

    sharedData().recents.removeAll(iconName);
    sharedData().recents.prepend(iconName);
    while (sharedData().recents.size() > kMaxRecentIcons)
        sharedData().recents.removeLast();

    saveRecentsLocked();
}

void FontAwesome::clearRecent()
{
    QMutexLocker<QRecursiveMutex> locker(&sharedMutex());
    if (!loadLocked())
        return;

    sharedData().recents.clear();
    saveRecentsLocked();
}

bool FontAwesome::initialize()
{
    QMutexLocker<QRecursiveMutex> locker(&sharedMutex());
    return loadLocked();
}

QString FontAwesome::iconForCpp(const QString &name, Style style)
{
    static FontAwesome instance;
    return instance.icon(name, style);
}

QString FontAwesome::familyForCpp(Style style)
{
    static FontAwesome instance;
    return instance.family(style);
}

int FontAwesome::weightForCpp(Style style)
{
    static FontAwesome instance;
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
    if (metadataLoaded && fontsLoaded)
        loadUserStateLocked();
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

        QStringList terms;
        terms.push_back(iconName);

        const QString label = iconObject.value(QStringLiteral("label")).toString();
        if (!label.isEmpty())
            terms.push_back(label.toLower());

        const QJsonObject searchObject = iconObject.value(QStringLiteral("search")).toObject();
        const QJsonArray searchTerms = searchObject.value(QStringLiteral("terms")).toArray();
        for (const QJsonValue &termValue : searchTerms) {
            const QString term = termValue.toString().trimmed().toLower();
            if (!term.isEmpty())
                terms.push_back(term);
        }

        terms.removeDuplicates();
        sharedData().searchTermsByIcon.insert(iconName, terms);
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

bool FontAwesome::containsIconAnyStyle(const QString &name)
{
    return !glyphForNameAndStyle(name, kStyleSolid).isEmpty()
        || !glyphForNameAndStyle(name, kStyleRegular).isEmpty()
        || !glyphForNameAndStyle(name, kStyleBrands).isEmpty();
}

QSettings FontAwesome::createSettings()
{
    // Use host application's organizationName() and applicationName()
    // instead of hard-coded identifiers to avoid settings collisions.
    return QSettings();
}

void FontAwesome::loadUserStateLocked()
{
    QSettings settings = createSettings();
    settings.beginGroup(kSettingsGroup);

    QStringList favorites = settings.value(kSettingsFavoritesKey).toStringList();
    favorites.removeDuplicates();
    for (auto it = favorites.begin(); it != favorites.end();) {
        *it = normalizeIconName(*it);
        if (it->isEmpty() || !containsIconAnyStyle(*it))
            it = favorites.erase(it);
        else
            ++it;
    }

    QStringList recents = settings.value(kSettingsRecentsKey).toStringList();
    recents.removeDuplicates();
    for (auto it = recents.begin(); it != recents.end();) {
        *it = normalizeIconName(*it);
        if (it->isEmpty() || !containsIconAnyStyle(*it))
            it = recents.erase(it);
        else
            ++it;
    }

    if (recents.size() > kMaxRecentIcons)
        recents = recents.mid(0, kMaxRecentIcons);

    sharedData().favorites = favorites;
    sharedData().recents = recents;
    settings.endGroup();
}

void FontAwesome::saveFavoritesLocked()
{
    QSettings settings = createSettings();
    settings.beginGroup(kSettingsGroup);
    settings.setValue(kSettingsFavoritesKey, sharedData().favorites);
    settings.endGroup();
}

void FontAwesome::saveRecentsLocked()
{
    QSettings settings = createSettings();
    settings.beginGroup(kSettingsGroup);
    settings.setValue(kSettingsRecentsKey, sharedData().recents);
    settings.endGroup();
}
