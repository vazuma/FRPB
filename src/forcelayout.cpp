#include "forcelayout.h"
#include "graphalgorithms.h"

#include <QRandomGenerator>
#include <QtMath>

double ForceLayout::step(Graph &g)
{
    const int n = g.nodeCount();
    if (n < 2)
        return 0.0;

    QList<Node> &nodes = g.nodes();

    // k is the ideal distance between two adjacent vertices.  Fruchterman &
    // Reingold derive it from the available area; here we let the caller pick
    // it and scale gently with the node count so big graphs still fit.
    const double k = idealEdgeLength * qMax(0.6, qMin(1.6, 24.0 / qSqrt(double(n))));
    const double k2 = k * k;

    QList<QPointF> disp(n, QPointF());

    // --- repulsion: every pair of vertices pushes each other apart ---------
    // O(n^2).  Fine up to a few thousand nodes; beyond that you would use a
    // Barnes-Hut quad tree, which is the natural next exercise.
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            QPointF delta = nodes[i].pos - nodes[j].pos;
            double d2 = QPointF::dotProduct(delta, delta);
            if (d2 < 1e-4) {
                // Coincident nodes: nudge them apart deterministically so the
                // simulation cannot divide by zero or get stuck.
                delta = QPointF(0.01 * (i + 1), 0.01 * (j + 1));
                d2 = QPointF::dotProduct(delta, delta);
            }
            const double d = qSqrt(d2);
            const QPointF force = (delta / d) * (k2 / d);   // magnitude k^2 / d
            disp[i] += force;
            disp[j] -= force;
        }
    }

    // --- attraction: every edge pulls its endpoints together ---------------
    for (const Edge &e : g.edges()) {
        QPointF delta = nodes[e.a].pos - nodes[e.b].pos;
        const double d = qMax(1e-3, qSqrt(QPointF::dotProduct(delta, delta)));
        const QPointF force = (delta / d) * (d * d / k);    // magnitude d^2 / k
        disp[e.a] -= force;
        disp[e.b] += force;
    }

    // --- gravity: keeps disconnected components from drifting to infinity --
    for (int i = 0; i < n; ++i)
        disp[i] -= nodes[i].pos * gravity * k;

    // --- integrate, clamped by the current temperature --------------------
    double totalMovement = 0.0;
    for (int i = 0; i < n; ++i) {
        if (nodes[i].pinned)
            continue;
        const double len = qSqrt(QPointF::dotProduct(disp[i], disp[i]));
        if (len < 1e-9)
            continue;
        const QPointF move = (disp[i] / len) * qMin(len, m_temperature);
        nodes[i].pos += move;
        totalMovement += qSqrt(QPointF::dotProduct(move, move));
    }

    // Simulated annealing: shrink the maximum step each frame so the layout
    // settles instead of oscillating forever.
    m_temperature *= cooling;
    if (m_temperature < 0.05)
        m_temperature = 0.05;

    return totalMovement;
}

void ForceLayout::reheat(double temperature)
{
    m_temperature = temperature < 0.0 ? 60.0 : temperature;
}

namespace Layouts {

void circular(Graph &g, double radius)
{
    const int n = g.nodeCount();
    if (n == 0)
        return;
    if (radius <= 0.0)
        radius = qMax(120.0, 26.0 * n / (2.0 * M_PI) * 2.2);
    for (int i = 0; i < n; ++i) {
        const double a = 2.0 * M_PI * i / n - M_PI / 2.0;
        g.node(i).pos = QPointF(radius * qCos(a), radius * qSin(a));
    }
}

void grid(Graph &g, double spacing)
{
    const int n = g.nodeCount();
    if (n == 0)
        return;
    const int cols = qMax(1, int(qCeil(qSqrt(double(n)))));
    const int rows = (n + cols - 1) / cols;
    for (int i = 0; i < n; ++i) {
        const int c = i % cols;
        const int r = i / cols;
        g.node(i).pos = QPointF((c - (cols - 1) / 2.0) * spacing,
                                (r - (rows - 1) / 2.0) * spacing);
    }
}

void random(Graph &g, double extent, quint32 seed)
{
    QRandomGenerator rng(seed ? seed : QRandomGenerator::global()->generate());
    for (int i = 0; i < g.nodeCount(); ++i) {
        g.node(i).pos = QPointF((rng.generateDouble() * 2.0 - 1.0) * extent,
                                (rng.generateDouble() * 2.0 - 1.0) * extent);
    }
}

void radialTree(Graph &g, int root, double ringSpacing)
{
    if (!g.isValidIndex(root))
        return;

    const GraphAlgorithms::Traversal t = GraphAlgorithms::breadthFirst(g, root);

    // Bucket the nodes by BFS distance, then spread each bucket on its ring.
    QList<QList<int>> rings;
    for (int i = 0; i < g.nodeCount(); ++i) {
        const int d = t.distance[i];
        const int ring = d < 0 ? 0 : d;          // unreachable nodes join ring 0
        while (rings.size() <= ring)
            rings.append(QList<int>());
        rings[ring].append(i);
    }

    for (int ring = 0; ring < rings.size(); ++ring) {
        const QList<int> &members = rings[ring];
        if (ring == 0 && members.size() == 1) {
            g.node(members.first()).pos = QPointF(0, 0);
            continue;
        }
        const double radius = ring * ringSpacing;
        for (int i = 0; i < members.size(); ++i) {
            const double a = 2.0 * M_PI * i / members.size();
            g.node(members[i]).pos = QPointF(radius * qCos(a), radius * qSin(a));
        }
    }
}

} // namespace Layouts
