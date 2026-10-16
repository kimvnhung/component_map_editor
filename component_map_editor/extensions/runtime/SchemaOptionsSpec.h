#ifndef SCHEMAOPTIONSSPEC_H
#define SCHEMAOPTIONSSPEC_H

#include <QString>
#include <QVariantList>

namespace cme::runtime
{

enum class SchemaOptionsSource
{
    None,
    TokenKeys,
    Defined,
    Custom,
    TokenKeyOptions, // Note: to be deprecated
};

struct SchemaOptionsSpec
{
    QString sourceId;                    // "" = không có dynamic options
    bool allowUnset = false;             // thay cho rule "endsWith(Ref)" (P6)
    QString unsetLabel;                  // vd "Use fallback value"
    bool fallbackToTextWhenEmpty = true; // thay cho hành vi ngầm (P10)
    QVariantList items;                  // thay cho tham số `options` và hard-code `shape` (P11)

    static SchemaOptionsSpec none();
    static SchemaOptionsSpec fromSource(QString id);
};

namespace Options
{
SchemaOptionsSpec tokenKeys();
SchemaOptionsSpec defined(QString sourceId);
SchemaOptionsSpec tokenRef(QString unsetLabel = QString());
SchemaOptionsSpec custom(QString sourceId);
SchemaOptionsSpec fixed(QVariantList items);
}; // namespace Options

} // namespace cme::runtime
#endif // SCHEMAOPTIONSSPEC_H
