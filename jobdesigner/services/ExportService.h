#pragma once

#include <QObject>
#include <QString>
#include <qqml.h>

#include "../models/GraphModel.h"

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
};
