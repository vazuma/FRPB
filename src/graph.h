#pragma once

// ---------------------------------------------------------------------------
// Graph - a plain C++ undirected, simple (no self-loops, no multi-edges) graph.
//
// Deliberately NOT a QObject:  the data structure knows nothing about signals,
// properties or QML.  That separation is worth internalising early - it keeps
// the interesting code testable without spinning up a GUI, and it means the
// Qt-specific glue lives in exactly one place (GraphController).
// ---------------------------------------------------------------------------

#include <QList>
#include <QPointF>
#include <QRectF>
#include <QString>

struct Node
{
    QString label;
    QPointF pos;        // world coordinates (y grows downwards, like the screen)
    QPointF velocity;   // scratch space used by the force-directed layout
    bool    pinned = false;
};

struct Edge
{
    int    a = -1;
    int    b = -1;
    double weight = 1.0;
};

class Graph
{
public:
    Graph() = default;

    // -- construction -------------------------------------------------------
    int  addNode(const QString &label = QString(), QPointF pos = QPointF());
    bool addEdge(int a, int b, double weight = 1.0);   // false if invalid/duplicate
    bool removeEdge(int a, int b);
    void removeNode(int index);                        // re-indexes everything after it
    void clear();

    // -- queries ------------------------------------------------------------
    int  nodeCount() const { return m_nodes.size(); }
    int  edgeCount() const { return m_edges.size(); }
    bool isValidIndex(int i) const { return i >= 0 && i < m_nodes.size(); }

    const QList<Node> &nodes() const { return m_nodes; }
    QList<Node>       &nodes()       { return m_nodes; }   // layout needs to write positions
    const QList<Edge> &edges() const { return m_edges; }

    const Node &node(int i) const { return m_nodes.at(i); }
    Node       &node(int i)       { return m_nodes[i]; }
    const Edge &edge(int i) const { return m_edges.at(i); }

    // Adjacency list: neighbours(i) is the list of node indices adjacent to i.
    const QList<int> &neighbours(int i) const;
    int  degree(int i) const { return neighbours(i).size(); }

    bool hasEdge(int a, int b) const { return edgeIndex(a, b) >= 0; }
    int  edgeIndex(int a, int b) const;

    // -- derived statistics -------------------------------------------------
    double density() const;         // 2|E| / (|V|(|V|-1)) for undirected graphs
    double averageDegree() const;   // 2|E| / |V|
    int    maxDegree() const;

    // -- bounding box of all node positions (empty QRectF for an empty graph)
    QRectF boundingBox() const;

private:
    void rebuildAdjacency();

    QList<Node>       m_nodes;
    QList<Edge>       m_edges;
    QList<QList<int>> m_adjacency;   // parallel to m_nodes
};
