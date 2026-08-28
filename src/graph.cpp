#include "graph.h"

#include <QRectF>
#include <algorithm>

int Graph::addNode(const QString &label, QPointF pos)
{
    Node n;
    n.label = label.isEmpty() ? QString::number(m_nodes.size()) : label;
    n.pos   = pos;
    m_nodes.append(n);
    m_adjacency.append(QList<int>());
    return m_nodes.size() - 1;
}

bool Graph::addEdge(int a, int b, double weight)
{
    if (!isValidIndex(a) || !isValidIndex(b))
        return false;
    if (a == b)                 // no self-loops in a simple graph
        return false;
    if (hasEdge(a, b))          // no parallel edges either
        return false;

    m_edges.append(Edge{a, b, weight});
    m_adjacency[a].append(b);
    m_adjacency[b].append(a);
    return true;
}

bool Graph::removeEdge(int a, int b)
{
    const int idx = edgeIndex(a, b);
    if (idx < 0)
        return false;
    m_edges.removeAt(idx);
    m_adjacency[a].removeAll(b);
    m_adjacency[b].removeAll(a);
    return true;
}

void Graph::removeNode(int index)
{
    if (!isValidIndex(index))
        return;

    // Drop every incident edge, then shift the indices of everything that came
    // after the removed node.  This is the price of storing nodes in a vector;
    // a real editor would use stable ids and a hash map instead.
    for (int i = m_edges.size() - 1; i >= 0; --i) {
        Edge &e = m_edges[i];
        if (e.a == index || e.b == index) {
            m_edges.removeAt(i);
            continue;
        }
        if (e.a > index) --e.a;
        if (e.b > index) --e.b;
    }

    m_nodes.removeAt(index);
    rebuildAdjacency();
}

void Graph::clear()
{
    m_nodes.clear();
    m_edges.clear();
    m_adjacency.clear();
}

const QList<int> &Graph::neighbours(int i) const
{
    static const QList<int> empty;
    if (!isValidIndex(i))
        return empty;
    return m_adjacency.at(i);
}

int Graph::edgeIndex(int a, int b) const
{
    for (int i = 0; i < m_edges.size(); ++i) {
        const Edge &e = m_edges.at(i);
        if ((e.a == a && e.b == b) || (e.a == b && e.b == a))
            return i;
    }
    return -1;
}

double Graph::density() const
{
    const int n = nodeCount();
    if (n < 2)
        return 0.0;
    return (2.0 * edgeCount()) / (double(n) * (n - 1));
}

double Graph::averageDegree() const
{
    if (m_nodes.isEmpty())
        return 0.0;
    return (2.0 * edgeCount()) / double(nodeCount());
}

int Graph::maxDegree() const
{
    int best = 0;
    for (const QList<int> &adj : m_adjacency)
        best = std::max(best, int(adj.size()));
    return best;
}

QRectF Graph::boundingBox() const
{
    if (m_nodes.isEmpty())
        return QRectF();

    qreal minX = m_nodes.first().pos.x(), maxX = minX;
    qreal minY = m_nodes.first().pos.y(), maxY = minY;
    for (const Node &n : m_nodes) {
        minX = std::min(minX, n.pos.x());
        maxX = std::max(maxX, n.pos.x());
        minY = std::min(minY, n.pos.y());
        maxY = std::max(maxY, n.pos.y());
    }
    return QRectF(QPointF(minX, minY), QPointF(maxX, maxY));
}

void Graph::rebuildAdjacency()
{
    m_adjacency.clear();
    m_adjacency.resize(m_nodes.size());
    for (const Edge &e : m_edges) {
        m_adjacency[e.a].append(e.b);
        m_adjacency[e.b].append(e.a);
    }
}
