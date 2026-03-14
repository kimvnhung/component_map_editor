#ifndef QMLFONTAWESOME_FONTAWESOME_H
#define QMLFONTAWESOME_FONTAWESOME_H

#include <QObject>
#include <QHash>
#include <QRecursiveMutex>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <qqmlintegration.h>

class FontAwesome : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    enum Style {
        Solid = 0,
        Regular = 1,
        Brands = 2
    };
    Q_ENUM(Style)

    explicit FontAwesome(QObject *parent = nullptr);

    Q_INVOKABLE bool ensureLoaded();
    Q_INVOKABLE QString icon(const QString &name, Style style = Solid) const;
    Q_INVOKABLE QString iconByStyleName(const QString &name,
                                        const QString &styleName = QStringLiteral("solid")) const;
    Q_INVOKABLE bool contains(const QString &name, Style style = Solid) const;
    Q_INVOKABLE QString family(Style style = Solid) const;
    Q_INVOKABLE int weight(Style style = Solid) const;
    Q_INVOKABLE QStringList availableStyles(const QString &name) const;
    Q_INVOKABLE int iconCount() const;

    static bool initialize();
    static QString iconForCpp(const QString &name, Style style = Solid);
    static QString familyForCpp(Style style = Solid);
    static int weightForCpp(Style style = Solid);

private:
    struct SharedData {
        bool loaded = false;
        QHash<QString, QHash<QString, QString>> glyphByStyle;
        QHash<QString, QString> familyByStyle;
        QHash<QString, int> weightByStyle;
    };

    static SharedData &sharedData();
    static QRecursiveMutex &sharedMutex();

    static bool loadLocked();
    static bool loadMetadataLocked();
    static bool loadFontsLocked();

    static QString resourcePrefix();
    static QString metadataPath();
    static QString styleKey(Style style);
    static QString normalizeStyleName(const QString &styleName);
    static QString normalizeIconName(QString name);
    static QString codepointToString(const QString &unicodeHex);
    static QString fallbackIcon(const QString &name);
    static QString glyphForNameAndStyle(const QString &name, const QString &styleKey);
};

#endif // QMLFONTAWESOME_FONTAWESOME_H
