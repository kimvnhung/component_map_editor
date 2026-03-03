#pragma once

#include <QObject>
#include <QStringList>
#include <qqml.h>

#include "../models/GraphModel.h"

class ValidationService : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit ValidationService(QObject *parent = nullptr);

    // Returns true if the graph has no structural errors.
    Q_INVOKABLE bool validate(GraphModel *graph);

    // Returns a list of human-readable error messages, empty if valid.
    Q_INVOKABLE QStringList validationErrors(GraphModel *graph);
};
