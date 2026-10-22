#ifndef SCHEMAOPTIONSSPEC_H
#define SCHEMAOPTIONSSPEC_H

#include <QString>
#include <QVariantList>

namespace cme::runtime
{

enum class SchemaOptionsSource
{
    None,
    TokenRef,
    Defined,
    Custom,
    TokenKeyOptions, // Note: to be deprecated
};

QString schemaOptionsSourceToString(SchemaOptionsSource value);
SchemaOptionsSource schemaOptionsSourceFromString(const QString& value);

struct SchemaOptionsSpec
{
    SchemaOptionsSource source;          // None, TokenKeys, Defined, Custom
    bool allowUnset = false;             // thay cho rule "endsWith(Ref)" (P6)
    QString unsetLabel;                  // vd "Use fallback value"
    bool fallbackToTextWhenEmpty = true; // thay cho hành vi ngầm (P10)
    QVariantList items;                  // thay cho tham số `options` và hard-code `shape` (P11)

    QString sourceKey;
    QString textRole;
    QString valueRole;

    static SchemaOptionsSpec none();
};

QVariantMap schemaOptionsSpecToMap(const SchemaOptionsSpec& spec);

namespace Options
{
// Deprecated: No options replaced by SchemaOptionsSpec::none()
SchemaOptionsSpec tokenKeys();
SchemaOptionsSpec defined(QString sourceKey = QString(), QString textRole = "text", QString valueRole = "value");
SchemaOptionsSpec tokenRef(QString fallbackFieldKey = QString());
SchemaOptionsSpec custom(QVariantList items);
}; // namespace Options

} // namespace cme::runtime
#endif // SCHEMAOPTIONSSPEC_H
