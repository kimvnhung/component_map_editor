#include "customizecomponenttypeprovider.h"

QString CustomizeComponentTypeProvider::providerId() const
{
    return QStringLiteral("sample.workflow.componentTypes");
}

QStringList CustomizeComponentTypeProvider::componentTypeIds() const
{
    return {
        QString::fromLatin1(TypeStart),
        QString::fromLatin1(TypeCondition),
        QString::fromLatin1(TypeProcess),
        QString::fromLatin1(TypeStop)
    };
}

QVariantMap CustomizeComponentTypeProvider::componentTypeDescriptor(const QString &componentTypeId) const
{
    struct Descriptor {
        const char *id;
        const char *title;
        const char *category;
        double      defaultWidth;
        double      defaultHeight;
        const char *defaultColor;
        bool        allowIncoming;
        bool        allowOutgoing;
    };

    static const Descriptor descriptors[] = {
        { TypeStart,   "Start",   "control", 92.0,  92.0,  "#66bb6a", false, true  },
        { TypeCondition, "Condition", "control", 164.0, 100.0, "#ffca28", true,  true  },
        { TypeProcess, "Process", "work",    164.0, 100.0, "#4fc3f7", true,  true  },
        { TypeStop,    "Stop",    "control", 92.0,  92.0,  "#ef5350", true,  false }
    };

    for (const auto &d : descriptors) {
        if (componentTypeId == QLatin1String(d.id)) {
            return QVariantMap{
                { QStringLiteral("id"),              QString::fromLatin1(d.id) },
                { QStringLiteral("title"),           QString::fromLatin1(d.title) },
                { QStringLiteral("category"),        QString::fromLatin1(d.category) },
                { QStringLiteral("defaultWidth"),    d.defaultWidth },
                { QStringLiteral("defaultHeight"),   d.defaultHeight },
                { QStringLiteral("defaultColor"),    QString::fromLatin1(d.defaultColor) },
                { QStringLiteral("allowIncoming"),   d.allowIncoming },
                { QStringLiteral("allowOutgoing"),   d.allowOutgoing }
            };
        }
    }
    return {};
}

QVariantMap CustomizeComponentTypeProvider::defaultComponentProperties(const QString &componentTypeId) const
{
    if (componentTypeId == QLatin1String(TypeStart)) {
        return {
            { QStringLiteral("inputNumber"), 0 }
        };
    }

    if (componentTypeId == QLatin1String(TypeProcess)) {
        return {
            { QStringLiteral("addValue"),    9 },
            { QStringLiteral("description"), QString() }
        };
    }

    if (componentTypeId == QLatin1String(TypeCondition)) {
        return {
            { QStringLiteral("condition"), QString() }
        };
    }

    return {};
}