// ---------------------------------------------------------------------------
// Unit tests for the GUI-free core, using Qt Test.
//
//   ctest --test-dir build --output-on-failure
//
// Every function here is a private slot; Qt Test discovers them via the
// meta-object system, which is why the class needs Q_OBJECT and why the file
// ends with #include "tst_graphcore.moc".
// ---------------------------------------------------------------------------

#include <QTest>

#include "forcelayout.h"
#include "graph.h"
#include "graphalgorithms.h"
#include "graphgenerators.h"

using namespace GraphAlgorithms;

class TestGraphCore : public QObject
{
    Q_OBJECT

private slots:
    void addAndQuery();
    void rejectsLoopsAndDuplicates();
    void removeNodeReindexesEdges();
    void generatorsHaveExpectedSizes();
    void bfsComputesHopDistance();
    void dfsVisitsEveryReachableNode();
    void componentsCountsIslands();
    void dijkstraFindsCheapestRoute();
    void mstMatchesKnownWeight();
    void bipartiteDetectsOddCycle();
    void forceLayoutSeparatesNodes();
};

void TestGraphCore::addAndQuery()
{
    Graph g;
    const int a = g.addNode(QStringLiteral("a"));
    const int b = g.addNode(QStringLiteral("b"));
    QVERIFY(g.addEdge(a, b, 2.5));

    QCOMPARE(g.nodeCount(), 2);
    QCOMPARE(g.edgeCount(), 1);
    QCOMPARE(g.degree(a), 1);
    QVERIFY(g.hasEdge(b, a));                 // undirected: order must not matter
    QCOMPARE(g.edge(g.edgeIndex(a, b)).weight, 2.5);
    QCOMPARE(g.averageDegree(), 1.0);
    QCOMPARE(g.density(), 1.0);
}

void TestGraphCore::rejectsLoopsAndDuplicates()
{
    Graph g = GraphGenerators::empty(3);
    QVERIFY(g.addEdge(0, 1));
    QVERIFY(!g.addEdge(1, 0));   // duplicate, reversed
    QVERIFY(!g.addEdge(2, 2));   // self loop
    QVERIFY(!g.addEdge(0, 9));   // out of range
    QCOMPARE(g.edgeCount(), 1);
}

void TestGraphCore::removeNodeReindexesEdges()
{
    Graph g = GraphGenerators::path(4);   // 0-1-2-3
    QCOMPARE(g.edgeCount(), 3);

    g.removeNode(1);                      // leaves 0, and 2-3 renumbered to 1-2
    QCOMPARE(g.nodeCount(), 3);
    QCOMPARE(g.edgeCount(), 1);
    QVERIFY(g.hasEdge(1, 2));
    QCOMPARE(g.degree(0), 0);
}

void TestGraphCore::generatorsHaveExpectedSizes()
{
    QCOMPARE(GraphGenerators::complete(6).edgeCount(), 15);          // C(6,2)
    QCOMPARE(GraphGenerators::cycle(7).edgeCount(), 7);
    QCOMPARE(GraphGenerators::path(7).edgeCount(), 6);
    QCOMPARE(GraphGenerators::grid(4, 3).nodeCount(), 12);
    QCOMPARE(GraphGenerators::grid(4, 3).edgeCount(), 3 * 3 + 4 * 2);
    QCOMPARE(GraphGenerators::completeBipartite(3, 3).edgeCount(), 9);

    const Graph p = GraphGenerators::petersen();
    QCOMPARE(p.nodeCount(), 10);
    QCOMPARE(p.edgeCount(), 15);
    for (int i = 0; i < p.nodeCount(); ++i)
        QCOMPARE(p.degree(i), 3);       // the Petersen graph is 3-regular

    const Graph q = GraphGenerators::hypercube(4);
    QCOMPARE(q.nodeCount(), 16);
    QCOMPARE(q.edgeCount(), 32);        // d * 2^(d-1)

    const Graph t = GraphGenerators::tree(3, 2);
    QCOMPARE(t.nodeCount(), 15);        // 1 + 2 + 4 + 8
    QCOMPARE(t.edgeCount(), 14);        // a tree has n - 1 edges
}

void TestGraphCore::bfsComputesHopDistance()
{
    const Graph g = GraphGenerators::path(5);      // 0-1-2-3-4
    const Traversal t = breadthFirst(g, 0);

    QCOMPARE(t.order.size(), 5);
    QCOMPARE(t.distance[4], 4);
    QCOMPARE(t.parent[3], 2);

    // On a cycle the far side is reachable in both directions.
    const Traversal c = breadthFirst(GraphGenerators::cycle(6), 0);
    QCOMPARE(c.distance[3], 3);
    QCOMPARE(c.distance[5], 1);
}

void TestGraphCore::dfsVisitsEveryReachableNode()
{
    Graph g = GraphGenerators::complete(5);
    g.addNode();                                   // an isolated 6th node
    const Traversal t = depthFirst(g, 0);

    QCOMPARE(t.order.size(), 5);                   // the isolated node is unreachable
    QCOMPARE(t.distance[5], -1);
    QCOMPARE(t.order.first(), 0);
}

void TestGraphCore::componentsCountsIslands()
{
    Graph g;
    for (int i = 0; i < 3; ++i) {                  // three separate triangles
        const int base = g.nodeCount();
        g.addNode(); g.addNode(); g.addNode();
        g.addEdge(base, base + 1);
        g.addEdge(base + 1, base + 2);
        g.addEdge(base + 2, base);
    }

    int count = 0;
    const QList<int> labels = connectedComponents(g, &count);
    QCOMPARE(count, 3);
    QCOMPARE(labels[0], labels[2]);
    QVERIFY(labels[0] != labels[3]);
}

void TestGraphCore::dijkstraFindsCheapestRoute()
{
    //   0 --1-- 1 --1-- 2
    //    \____ 10 ____/
    Graph g = GraphGenerators::empty(3);
    g.addEdge(0, 1, 1.0);
    g.addEdge(1, 2, 1.0);
    g.addEdge(0, 2, 10.0);

    const ShortestPath sp = shortestPath(g, 0, 2);
    QVERIFY(sp.found);
    QCOMPARE(sp.length, 2.0);
    QCOMPARE(sp.nodes, (QList<int>{0, 1, 2}));
    QCOMPARE(sp.edges.size(), 2);

    // Disconnected target.
    g.addNode();
    QVERIFY(!shortestPath(g, 0, 3).found);
}

void TestGraphCore::mstMatchesKnownWeight()
{
    Graph g = GraphGenerators::empty(4);
    g.addEdge(0, 1, 1.0);
    g.addEdge(1, 2, 2.0);
    g.addEdge(2, 3, 3.0);
    g.addEdge(0, 3, 9.0);
    g.addEdge(0, 2, 8.0);

    const SpanningForest f = minimumSpanningForest(g);
    QCOMPARE(f.edges.size(), 3);       // n - 1 for a connected graph
    QCOMPARE(f.weight, 6.0);           // 1 + 2 + 3
    QCOMPARE(f.components, 1);

    // A forest over two disjoint edges has two components.
    Graph h = GraphGenerators::empty(4);
    h.addEdge(0, 1);
    h.addEdge(2, 3);
    QCOMPARE(minimumSpanningForest(h).components, 2);
}

void TestGraphCore::bipartiteDetectsOddCycle()
{
    QList<int> colours;
    QVERIFY(twoColouring(GraphGenerators::cycle(6), &colours));      // even cycle
    QVERIFY(twoColouring(GraphGenerators::grid(4, 4), &colours));    // lattices are bipartite
    QVERIFY(twoColouring(GraphGenerators::completeBipartite(3, 4), &colours));

    int badEdge = -1;
    QVERIFY(!twoColouring(GraphGenerators::cycle(5), &colours, &badEdge));   // odd cycle
    QVERIFY(badEdge >= 0);
    QVERIFY(!twoColouring(GraphGenerators::complete(3), &colours));
}

void TestGraphCore::forceLayoutSeparatesNodes()
{
    // Two nodes dumped on top of each other must not stay there.
    Graph g;
    g.addNode(QStringLiteral("a"), QPointF(0, 0));
    g.addNode(QStringLiteral("b"), QPointF(0, 0));
    g.addEdge(0, 1);

    ForceLayout layout;
    for (int i = 0; i < 400; ++i)
        layout.step(g);

    const QPointF d = g.node(0).pos - g.node(1).pos;
    const double distance = std::sqrt(QPointF::dotProduct(d, d));
    QVERIFY2(distance > 10.0, qPrintable(QStringLiteral("distance = %1").arg(distance)));
    QVERIFY(layout.isCool());
    QVERIFY(std::isfinite(g.node(0).pos.x()));
}

QTEST_MAIN(TestGraphCore)
#include "tst_graphcore.moc"
