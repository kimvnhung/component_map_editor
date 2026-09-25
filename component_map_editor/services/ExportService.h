#ifndef EXPORTSERVICE_H
#define EXPORTSERVICE_H

#include <QObject>
#include <QString>
#include <QQmlEngine>

#include "models/GraphModel.h"

class ExportService : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit ExportService(QObject *parent = nullptr);

    // Serialise the graph to a JSON string.
    Q_INVOKABLE QString exportToJson(GraphModel *graph);

    // Populate the graph from a previously exported JSON string.
    // Returns true on success, false on parse error.
    Q_INVOKABLE bool importFromJson(GraphModel *graph, const QString &json);

    // Export the graph to a JSON file at the specified path.
    Q_INVOKABLE bool exportToJsonFile(GraphModel *graph, const QString &filePath);

    // Import the graph from a JSON file at the specified path.
    // Returns true on success, false on parse error or file read error.
    Q_INVOKABLE bool importFromJsonFile(GraphModel *graph, const QString &filePath);
};

#endif // EXPORTSERVICE_H
