#ifndef BENCHMARKHELPER_H
#define BENCHMARKHELPER_H

#include <QObject>
#include <QQmlEngine>
#include <GraphModel.h>

// BenchmarkHelper — C++ scenario generator for Phase 0 performance baseline.
//
// Creates synthetic grid-topology graphs entirely in C++, bypassing the
// Qt.createQmlObject overhead of QML-based creation.  Uses GraphModel's
// batch-update API so only one componentsChanged signal fires per load.
//
// Grid topology for N nodes:
//   cols = ceil(sqrt(N))
//   nodes laid out in cols-wide rows, spaced 200 world units apart
//   edges: each node connects right to (col+1) and down to (row+1)
//   expected edge count ≈ 2*(N - sqrt(N))
class BenchmarkHelper : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit BenchmarkHelper(QObject *parent = nullptr);

    // Clears graph then populates it with nodeCount nodes in a grid topology.
    // Blocks the calling thread; intended for benchmark setup, not production use.
    Q_INVOKABLE void populateGraph(GraphModel *graph, int nodeCount);
};

#endif // BENCHMARKHELPER_H
