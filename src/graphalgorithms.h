#pragma once

// ---------------------------------------------------------------------------
// Free functions over Graph.  Nothing here knows about Qt Quick; each function
// takes a const Graph & and returns a plain result object, which makes them
// trivial to unit test (see tests/tst_graphcore.cpp).
// ---------------------------------------------------------------------------

#include "graph.h"

namespace GraphAlgorithms {

// Result of a BFS/DFS from a single source.
struct Traversal
{
    QList<int> order;       // nodes in the order they were visited
    QList<int> parent;      // parent[i] = predecessor of i in the tree, -1 if none
    QList<int> distance;    // hop count from the source, -1 if unreachable
                            // (for DFS this is depth in the DFS tree)
    QList<int> treeEdges;   // edge indices forming the traversal tree
};

// Breadth-first search: explores by layers, so `distance` really is the
// shortest path length in an unweighted graph.
Traversal breadthFirst(const Graph &g, int source);

// Depth-first search: dives as deep as possible before backtracking.
Traversal depthFirst(const Graph &g, int source);

// Labels every node with the id of its connected component (0, 1, 2, ...).
// Returns the labels; *count receives the number of components.
QList<int> connectedComponents(const Graph &g, int *count = nullptr);

// Dijkstra.  Returns the node sequence source -> ... -> target, or an empty
// list when no path exists.  Edge weights must be non-negative.
struct ShortestPath
{
    QList<int> nodes;       // the path itself
    QList<int> edges;       // edge indices along the path
    double     length = 0.0;
    bool       found = false;
};
ShortestPath shortestPath(const Graph &g, int source, int target);

// Kruskal's algorithm using a disjoint-set union.  Returns the indices of the
// edges in the minimum spanning forest (forest, not tree, if g is disconnected).
struct SpanningForest
{
    QList<int> edges;
    double     weight = 0.0;
    int        components = 0;
};
SpanningForest minimumSpanningForest(const Graph &g);

// Two-colouring via BFS.  Returns true if the graph is bipartite; `colours`
// then holds 0/1 per node.  On failure `colours` holds the partial colouring
// and `oddCycleEdge` is the index of an edge that closes an odd cycle.
bool twoColouring(const Graph &g, QList<int> *colours, int *oddCycleEdge = nullptr);

// degree(i) / (n - 1): the classic normalised degree centrality.
QList<double> degreeCentrality(const Graph &g);

} // namespace GraphAlgorithms
