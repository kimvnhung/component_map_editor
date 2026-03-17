#ifndef GRAPHJSONMIGRATION_H
#define GRAPHJSONMIGRATION_H

#include <QJsonArray>
#include <QJsonObject>

namespace GraphJsonMigration {

struct CanonicalDocument {
    QJsonArray components;
    QJsonArray connections;
};

CanonicalDocument migrateToCurrentSchema(const QJsonObject &root);

} // namespace GraphJsonMigration

#endif // GRAPHJSONMIGRATION_H
