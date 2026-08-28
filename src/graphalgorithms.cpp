#include "graphalgorithms.h"

#include <QQueue>
#include <QStack>
#include <algorithm>
#include <limits>
#include <queue>

namespace GraphAlgorithms {

namespace {

// Small helper: initialise the three parallel arrays of a Traversal.
Traversal makeEmptyTraversal(int n)
{
    Traversal t;
    t.parent.fill(-1, n);
    t.distance.fill(-1, n);
    return t;
}

// Disjoint-set union (union-find) with path compression and union by rank.
class DisjointSet
{
public:
    explicit DisjointSet(int n) : m_parent(n), m_rank(n, 0)
    {
        for (int i = 0; i < n; ++i)
            m_parent[i] = i;
    }

    int find(int x)
    {
        while (m_parent[x] != x) {
            m_parent[x] = m_parent[m_parent[x]];   // path halving
            x = m_parent[x];
        }
        return x;
    }

    bool unite(int a, int b)
    {
        a = find(a);
        b = find(b);
        if (a == b)
            return false;
        if (m_rank[a] < m_rank[b])
            std::swap(a, b);
        m_parent[b] = a;
        if (m_rank[a] == m_rank[b])
            ++m_rank[a];
        return true;
    }

private:
    QList<int> m_parent;
    QList<int> m_rank;
};

} // namespace

Traversal breadthFirst(const Graph &g, int source)
{
    Traversal t = makeEmptyTraversal(g.nodeCount());
    if (!g.isValidIndex(source))
        return t;

    QQueue<int> queue;
    t.distance[source] = 0;
    queue.enqueue(source);

    while (!queue.isEmpty()) {
        const int u = queue.dequeue();
        t.order.append(u);
        for (int v : g.neighbours(u)) {
            if (t.distance[v] != -1)        // already discovered
                continue;
            t.distance[v] = t.distance[u] + 1;
            t.parent[v]   = u;
            t.treeEdges.append(g.edgeIndex(u, v));
            queue.enqueue(v);
        }
    }
    return t;
}

Traversal depthFirst(const Graph &g, int source)
{
    Traversal t = makeEmptyTraversal(g.nodeCount());
    if (!g.isValidIndex(source))
        return t;

    // Iterative DFS so that a 100k-node graph cannot blow the call stack.
    QStack<int> stack;
    stack.push(source);
    t.distance[source] = 0;

    QList<bool> visited(g.nodeCount(), false);
    while (!stack.isEmpty()) {
        const int u = stack.pop();
        if (visited[u])
            continue;
        visited[u] = true;
        t.order.append(u);
        if (t.parent[u] != -1)
            t.treeEdges.append(g.edgeIndex(t.parent[u], u));

        // Push in reverse so the lowest-numbered neighbour is explored first.
        QList<int> adj = g.neighbours(u);
        std::sort(adj.begin(), adj.end(), std::greater<int>());
        for (int v : adj) {
            if (visited[v])
                continue;
            t.parent[v]   = u;
            t.distance[v] = t.distance[u] + 1;
            stack.push(v);
        }
    }
    return t;
}

QList<int> connectedComponents(const Graph &g, int *count)
{
    QList<int> component(g.nodeCount(), -1);
    int next = 0;

    for (int start = 0; start < g.nodeCount(); ++start) {
        if (component[start] != -1)
            continue;
        // Flood fill this component.
        QQueue<int> queue;
        queue.enqueue(start);
        component[start] = next;
        while (!queue.isEmpty()) {
            const int u = queue.dequeue();
            for (int v : g.neighbours(u)) {
                if (component[v] == -1) {
                    component[v] = next;
                    queue.enqueue(v);
                }
            }
        }
        ++next;
    }

    if (count)
        *count = next;
    return component;
}

ShortestPath shortestPath(const Graph &g, int source, int target)
{
    ShortestPath result;
    if (!g.isValidIndex(source) || !g.isValidIndex(target))
        return result;

    constexpr double inf = std::numeric_limits<double>::infinity();
    QList<double> dist(g.nodeCount(), inf);
    QList<int>    parent(g.nodeCount(), -1);
    QList<bool>   done(g.nodeCount(), false);

    // std::priority_queue is a max-heap, so store negated keys via greater<>.
    using Entry = std::pair<double, int>;   // (distance, node)
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> pq;

    dist[source] = 0.0;
    pq.push({0.0, source});

    while (!pq.empty()) {
        const int u = pq.top().second;
        pq.pop();
        if (done[u])
            continue;               // stale heap entry
        done[u] = true;
        if (u == target)
            break;

        for (int v : g.neighbours(u)) {
            const int ei = g.edgeIndex(u, v);
            if (ei < 0)
                continue;
            const double w  = g.edge(ei).weight;
            const double alt = dist[u] + w;
            if (alt < dist[v]) {
                dist[v]   = alt;
                parent[v] = u;
                pq.push({alt, v});
            }
        }
    }

    if (dist[target] == inf)
        return result;              // unreachable

    // Walk the parent chain backwards, then reverse it.
    for (int at = target; at != -1; at = parent[at])
        result.nodes.prepend(at);
    for (int i = 1; i < result.nodes.size(); ++i)
        result.edges.append(g.edgeIndex(result.nodes[i - 1], result.nodes[i]));

    result.length = dist[target];
    result.found  = true;
    return result;
}

SpanningForest minimumSpanningForest(const Graph &g)
{
    SpanningForest forest;

    // Sort edge *indices* by weight - we want to report indices, not copies.
    QList<int> order;
    order.reserve(g.edgeCount());
    for (int i = 0; i < g.edgeCount(); ++i)
        order.append(i);
    std::sort(order.begin(), order.end(), [&g](int lhs, int rhs) {
        return g.edge(lhs).weight < g.edge(rhs).weight;
    });

    DisjointSet dsu(g.nodeCount());
    for (int ei : order) {
        const Edge &e = g.edge(ei);
        if (dsu.unite(e.a, e.b)) {       // joining two different trees?
            forest.edges.append(ei);
            forest.weight += e.weight;
        }
    }

    // |V| - |E_forest| is exactly the number of connected components.
    forest.components = g.nodeCount() - forest.edges.size();
    return forest;
}

bool twoColouring(const Graph &g, QList<int> *colours, int *oddCycleEdge)
{
    QList<int> colour(g.nodeCount(), -1);
    bool bipartite = true;
    if (oddCycleEdge)
        *oddCycleEdge = -1;

    for (int start = 0; start < g.nodeCount() && bipartite; ++start) {
        if (colour[start] != -1)
            continue;
        colour[start] = 0;
        QQueue<int> queue;
        queue.enqueue(start);
        while (!queue.isEmpty() && bipartite) {
            const int u = queue.dequeue();
            for (int v : g.neighbours(u)) {
                if (colour[v] == -1) {
                    colour[v] = 1 - colour[u];
                    queue.enqueue(v);
                } else if (colour[v] == colour[u]) {
                    // Same colour on both ends => an odd cycle => not bipartite.
                    bipartite = false;
                    if (oddCycleEdge)
                        *oddCycleEdge = g.edgeIndex(u, v);
                    break;
                }
            }
        }
    }

    if (colours)
        *colours = colour;
    return bipartite;
}

QList<double> degreeCentrality(const Graph &g)
{
    QList<double> result(g.nodeCount(), 0.0);
    if (g.nodeCount() < 2)
        return result;
    const double denom = g.nodeCount() - 1;
    for (int i = 0; i < g.nodeCount(); ++i)
        result[i] = g.degree(i) / denom;
    return result;
}

} // namespace GraphAlgorithms
