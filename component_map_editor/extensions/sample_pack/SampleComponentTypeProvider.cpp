#include "SampleComponentTypeProvider.h"

QString SampleComponentTypeProvider::providerId() const
{
    return QStringLiteral("sample.workflow.componentTypes");
}

QStringList SampleComponentTypeProvider::componentTypeIds() const
{
    return {
        QString::fromLatin1(TypeStart),
        QString::fromLatin1(TypeTask),
        QString::fromLatin1(TypeDecision),
        QString::fromLatin1(TypeEnd)
    };
}

QVariantMap SampleComponentTypeProvider::componentTypeDescriptor(const QString &componentTypeId) const
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
        { TypeStart,    "Start",    "control", 80.0,  80.0,  "#66bb6a", false, true  },
        { TypeTask,     "Task",     "work",    160.0, 96.0,  "#4fc3f7", true,  true  },
        { TypeDecision, "Decision", "control", 120.0, 120.0, "#ffa726", true,  true  },
        { TypeEnd,      "End",      "control", 80.0,  80.0,  "#ef5350", true,  false }
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

QVariantMap SampleComponentTypeProvider::defaultComponentProperties(const QString &componentTypeId) const
{
    if (componentTypeId == QLatin1String(TypeTask)) {
        return {
            { QStringLiteral("priority"),    QStringLiteral("normal") },
            { QStringLiteral("description"), QString() }
        };
    }
    if (componentTypeId == QLatin1String(TypeDecision)) {
        return {
            { QStringLiteral("condition"), QString() }
        };
    }
    return {};
}