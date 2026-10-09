#ifndef SCHEMAOPTIONSSPEC_H
#define SCHEMAOPTIONSSPEC_H

#include <QString>

struct SchemaOptionsSpec
{
    QString sourceId;                    // "" = không có dynamic options
    bool allowUnset = false;             // thay cho rule "endsWith(Ref)" (P6)
    QString unsetLabel;                  // vd "Use fallback value"
    bool fallbackToTextWhenEmpty = true; // thay cho hành vi ngầm (P10)

    static SchemaOptionsSpec none();
    static SchemaOptionsSpec fromSource(QString id);
};

namespace OptionsSources
{ // hằng duy nhất cho tên (P4)
inline const QString TokenKeys = QStringLiteral("tokenKeys");
inline const QString TokenKeyOptions = QStringLiteral("tokenKeyOptions");
} // namespace OptionsSources

namespace Options
{ // factory đọc được ngay
SchemaOptionsSpec tokenKeys();
SchemaOptionsSpec tokenRef(QString unsetLabel = "Use fallback value"); // tokenKeyOptions + allowUnset
SchemaOptionsSpec custom(QString sourceId);
SchemaOptionsSpec fixed(QVariantList items); // thay cho tham số `options` và hard-code `shape`
} // namespace Options

#endif // SCHEMAOPTIONSSPEC_H
