#include "graphgenerators.h"

#include <QRandomGenerator>
#include <QtMath>

namespace GraphGenerators {

namespace {

// Spread n nodes evenly around a circle - a decent neutral starting layout.
void placeOnCircle(Graph &g, int first, int count, qreal radius, QPointF centre = QPointF())
{
    for (int i = 0; i < count; ++i) {
        const qreal angle = 2.0 * M_PI * i / qMax(1, count);
        g.node(first + i).pos = centre + QPointF(radius * qCos(angle), radius * qSin(angle));
    }
}

} // namespace

Graph empty(int n)
{
    Graph g;
    for (int i = 0; i < n; ++i)
        g.addNode();
    placeOnCircle(g, 0, n, 60.0 + 18.0 * n / 3.0);
    return g;
}

Graph path(int n)
{
    Graph g = empty(n);
    for (int i = 0; i + 1 < n; ++i)
        g.addEdge(i, i + 1);
    // A straight line reads better than a circle for a path.
    const qreal spacing = 70.0;
    for (int i = 0; i < n; ++i)
        g.node(i).pos = QPointF((i - (n - 1) / 2.0) * spacing, 0.0);
    return g;
}

Graph cycle(int n)
{
    Graph g = empty(n);
    for (int i = 0; i < n; ++i)
        g.addEdge(i, (i + 1) % n);
    return g;
}

Graph complete(int n)
{
    Graph g = empty(n);
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j)
            g.addEdge(i, j);
    return g;
}

Graph completeBipartite(int m, int n)
{
    Graph g;
    for (int i = 0; i < m; ++i)
        g.addNode(QStringLiteral("a%1").arg(i), QPointF((i - (m - 1) / 2.0) * 80.0, -140.0));
    for (int j = 0; j < n; ++j)
        g.addNode(QStringLiteral("b%1").arg(j), QPointF((j - (n - 1) / 2.0) * 80.0, 140.0));
    for (int i = 0; i < m; ++i)
        for (int j = 0; j < n; ++j)
            g.addEdge(i, m + j);
    return g;
}

Graph grid(int cols, int rows)
{
    Graph g;
    const qreal spacing = 80.0;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            g.addNode(QStringLiteral("%1,%2").arg(c).arg(r),
                      QPointF((c - (cols - 1) / 2.0) * spacing,
                              (r - (rows - 1) / 2.0) * spacing));
        }
    }
    auto id = [cols](int c, int r) { return r * cols + c; };
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (c + 1 < cols) g.addEdge(id(c, r), id(c + 1, r));
            if (r + 1 < rows) g.addEdge(id(c, r), id(c, r + 1));
        }
    }
    return g;
}

Graph star(int leaves)
{
    Graph g;
    g.addNode(QStringLiteral("hub"), QPointF(0, 0));
    for (int i = 0; i < leaves; ++i)
        g.addNode();
    placeOnCircle(g, 1, leaves, 160.0);
    for (int i = 1; i <= leaves; ++i)
        g.addEdge(0, i);
    return g;
}

Graph wheel(int rim)
{
    Graph g = star(rim);
    for (int i = 1; i <= rim; ++i)
        g.addEdge(i, 1 + (i % rim));
    return g;
}

Graph tree(int depth, int branching)
{
    Graph g;
    g.addNode(QStringLiteral("root"), QPointF(0, -180.0));

    QList<int> currentLevel{0};
    for (int d = 1; d <= depth; ++d) {
        QList<int> nextLevel;
        for (int parent : currentLevel) {
            for (int b = 0; b < branching; ++b) {
                const int child = g.addNode();
                g.addEdge(parent, child);
                nextLevel.append(child);
            }
        }
        // Lay the level out horizontally, one row below its parent.
        const qreal y = -180.0 + d * 110.0;
        const qreal spacing = qMin(90.0, 900.0 / qMax(1, nextLevel.size()));
        for (int i = 0; i < nextLevel.size(); ++i)
            g.node(nextLevel[i]).pos = QPointF((i - (nextLevel.size() - 1) / 2.0) * spacing, y);
        currentLevel = nextLevel;
    }
    return g;
}

Graph petersen()
{
    // Outer pentagon 0..4, inner pentagram 5..9 - the standard textbook drawing.
    Graph g;
    for (int i = 0; i < 10; ++i)
        g.addNode();
    placeOnCircle(g, 0, 5, 190.0);
    placeOnCircle(g, 5, 5, 95.0);

    for (int i = 0; i < 5; ++i) {
        g.addEdge(i, (i + 1) % 5);              // outer cycle
        g.addEdge(5 + i, 5 + ((i + 2) % 5));    // inner pentagram
        g.addEdge(i, 5 + i);                    // spokes
    }
    return g;
}

Graph hypercube(int dimension)
{
    const int n = 1 << qBound(0, dimension, 10);
    Graph g;
    for (int i = 0; i < n; ++i)
        g.addNode(QString::number(i, 2).rightJustified(qMax(1, dimension), QLatin1Char('0')));
    placeOnCircle(g, 0, n, 60.0 + 22.0 * dimension * dimension);

    // Two vertices are adjacent exactly when their labels differ in one bit.
    for (int i = 0; i < n; ++i)
        for (int b = 0; b < dimension; ++b)
            g.addEdge(i, i ^ (1 << b));   // duplicates are rejected by addEdge
    return g;
}

Graph erdosRenyi(int n, double p, quint32 seed)
{
    Graph g = empty(n);
    QRandomGenerator rng(seed);
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j)
            if (rng.generateDouble() < p)
                g.addEdge(i, j);
    return g;
}

} // namespace GraphGenerators
