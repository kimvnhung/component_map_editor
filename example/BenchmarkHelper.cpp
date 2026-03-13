#include "BenchmarkHelper.h"
#include <GraphModel.h>
#include <ComponentModel.h>
#include <ConnectionModel.h>

#include <QtMath>

BenchmarkHelper::BenchmarkHelper(QObject *parent)
    : QObject(parent)
{}

void BenchmarkHelper::populateGraph(GraphModel *graph, int nodeCount)
{
    if (!graph || nodeCount <= 0)
        return;

    graph->beginBatchUpdate();
    graph->clear();

    const int   cols    = qCeil(qSqrt(static_cast<qreal>(nodeCount)));
    const qreal spacing = 200.0;

    // --- Nodes ---
    for (int i = 0; i < nodeCount; ++i) {
        const int col = i % cols;
        const int row = i / cols;
        // x/y are component centers in world coordinates.
        auto *c = new ComponentModel(
            QStringLiteral("bench_n%1").arg(i),
            QStringLiteral("N%1").arg(i),
            col * spacing + spacing,
            row * spacing + spacing,
            QStringLiteral("#5c6bc0"),
            QStringLiteral("default"),
            graph);
        graph->addComponent(c);
    }

    // --- Edges: right neighbor + down neighbor (grid topology) ---
    int edgeIdx = 0;
    for (int i = 0; i < nodeCount; ++i) {
        const int col = i % cols;

        // Right edge: connect i → i+1 (same row only)
        const int rightIdx = i + 1;
        if (col + 1 < cols && rightIdx < nodeCount) {
            auto *e = new ConnectionModel(
                QStringLiteral("bench_e%1").arg(edgeIdx++),
                QStringLiteral("bench_n%1").arg(i),
                QStringLiteral("bench_n%1").arg(rightIdx),
                QString(), graph);
            graph->addConnection(e);
        }

        // Down edge: connect i → i+cols
        const int downIdx = i + cols;
        if (downIdx < nodeCount) {
            auto *e = new ConnectionModel(
                QStringLiteral("bench_e%1").arg(edgeIdx++),
                QStringLiteral("bench_n%1").arg(i),
                QStringLiteral("bench_n%1").arg(downIdx),
                QString(), graph);
            graph->addConnection(e);
        }
    }

    graph->endBatchUpdate();
}
