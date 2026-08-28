#include "graphcontroller.h"
#include "graphalgorithms.h"
#include "graphgenerators.h"

#include <QFile>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtMath>

using namespace GraphAlgorithms;

namespace {

// A colourblind-friendly qualitative palette, cycled for component/colour ids.
const QColor kPalette[] = {
    QColor("#4c9aff"), QColor("#ff8f5e"), QColor("#5ed3a0"), QColor("#c48fff"),
    QColor("#ffd166"), QColor("#ff6b8a"), QColor("#4fd6e0"), QColor("#9db4ff"),
};
constexpr int kPaletteSize = int(std::size(kPalette));

const QColor kNeutralFill   = QColor("#6d8ba8");
const QColor kNeutralRing   = QColor("#0f1720");
const QColor kNeutralEdge   = QColor(150, 168, 190, 150);
const QColor kAccent        = QColor("#ffd166");
const QColor kDimFill       = QColor(70, 84, 100);
const QColor kDimEdge       = QColor(90, 102, 118, 70);

constexpr float kBaseRadius     = 13.0f;
constexpr float kBaseEdgeWidth  = 1.6f;
constexpr float kThickEdgeWidth = 4.0f;

QColor paletteColour(int i)
{
    return kPalette[((i % kPaletteSize) + kPaletteSize) % kPaletteSize];
}

// Blend between two colours; used for the centrality heat map.
QColor lerpColour(const QColor &a, const QColor &b, double t)
{
    t = qBound(0.0, t, 1.0);
    return QColor::fromRgbF(a.redF()   + (b.redF()   - a.redF())   * t,
                            a.greenF() + (b.greenF() - a.greenF()) * t,
                            a.blueF()  + (b.blueF()  - a.blueF())  * t);
}

} // namespace

GraphController::GraphController(QObject *parent)
    : QObject(parent)
    , m_nodeModel(new NodeListModel(this))
{
    // The layout runs on the GUI thread from a timer.  Each tick advances the
    // simulation by one iteration and tells the world the positions moved -
    // that is all the viewport needs to redraw.
    m_layoutTimer.setInterval(16);          // ~60 frames per second
    connect(&m_layoutTimer, &QTimer::timeout, this, &GraphController::stepLayout);

    onGraphReplaced(GraphGenerators::petersen(), tr("Petersen graph"));
}

GraphController::~GraphController() = default;

// ---------------------------------------------------------------------------
// Bookkeeping
// ---------------------------------------------------------------------------

void GraphController::setStatus(const QString &text)
{
    if (m_statusText == text)
        return;
    m_statusText = text;
    emit statusTextChanged();
}

void GraphController::resetStyling()
{
    const int n = m_graph.nodeCount();
    const int e = m_graph.edgeCount();

    m_nodeFill.fill(kNeutralFill, n);
    m_nodeRing.fill(kNeutralRing, n);
    m_nodeRadius.fill(kBaseRadius, n);
    m_edgeColour.fill(kNeutralEdge, e);
    m_edgeWidth.fill(kBaseEdgeWidth, e);

    // Hubs are drawn slightly larger - a cheap but very readable cue.
    const int maxDeg = qMax(1, m_graph.maxDegree());
    for (int i = 0; i < n; ++i)
        m_nodeRadius[i] = kBaseRadius * float(1.0 + 0.45 * m_graph.degree(i) / maxDeg);
}

void GraphController::onGraphReplaced(Graph &&g, const QString &description)
{
    m_nodeModel->beginRebuild();
    m_graph = std::move(g);
    m_selectedNode = -1;
    m_algorithmName.clear();
    resetStyling();
    m_layout.reheat();
    m_nodeModel->endRebuild();

    emit graphChanged();
    emit highlightsChanged();
    emit selectedNodeChanged();
    setStatus(tr("%1 - %2 nodes, %3 edges")
                  .arg(description)
                  .arg(m_graph.nodeCount())
                  .arg(m_graph.edgeCount()));
}

void GraphController::stepLayout()
{
    const double movement = m_layout.step(m_graph);
    emit positionsChanged();
    m_nodeModel->notifyPositionsChanged();

    // Stop by ourselves once the graph has settled, so an idle window is not
    // burning a CPU core forever.
    if (m_layout.isCool() && movement < 0.4 * qMax(1, m_graph.nodeCount()))
        setLayoutRunning(false);
}

void GraphController::setSelectedNode(int index)
{
    const int clamped = m_graph.isValidIndex(index) ? index : -1;
    if (m_selectedNode == clamped)
        return;
    m_selectedNode = clamped;
    emit selectedNodeChanged();
    emit highlightsChanged();
    if (clamped >= 0) {
        setStatus(tr("Node %1 (%2) - degree %3")
                      .arg(clamped)
                      .arg(m_graph.node(clamped).label)
                      .arg(m_graph.degree(clamped)));
    }
}

void GraphController::setLayoutRunning(bool running)
{
    if (running == m_layoutTimer.isActive())
        return;
    if (running) {
        if (m_layout.isCool())
            m_layout.reheat();
        m_layoutTimer.start();
    } else {
        m_layoutTimer.stop();
    }
    emit layoutRunningChanged();
}

int GraphController::componentCount() const
{
    if (m_graph.nodeCount() == 0)
        return 0;
    int count = 0;
    connectedComponents(m_graph, &count);
    return count;
}

// ---------------------------------------------------------------------------
// Editing
// ---------------------------------------------------------------------------

void GraphController::clear()
{
    onGraphReplaced(Graph(), tr("Empty graph"));
}

int GraphController::addNodeAt(qreal x, qreal y)
{
    m_nodeModel->beginRebuild();
    const int index = m_graph.addNode(QString(), QPointF(x, y));
    resetStyling();
    m_nodeModel->endRebuild();

    emit graphChanged();
    emit highlightsChanged();
    setStatus(tr("Added node %1").arg(index));
    return index;
}

void GraphController::removeSelectedNode()
{
    if (!m_graph.isValidIndex(m_selectedNode))
        return;
    const int removed = m_selectedNode;

    m_nodeModel->beginRebuild();
    m_graph.removeNode(removed);
    m_selectedNode = -1;
    resetStyling();
    m_nodeModel->endRebuild();

    emit graphChanged();
    emit highlightsChanged();
    emit selectedNodeChanged();
    setStatus(tr("Removed node %1").arg(removed));
}

void GraphController::toggleEdge(int a, int b)
{
    if (!m_graph.isValidIndex(a) || !m_graph.isValidIndex(b) || a == b)
        return;

    const bool existed = m_graph.hasEdge(a, b);
    if (existed)
        m_graph.removeEdge(a, b);
    else
        m_graph.addEdge(a, b);

    resetStyling();
    emit graphChanged();
    emit highlightsChanged();
    m_nodeModel->notifyStylingChanged();
    setStatus(existed ? tr("Removed edge %1-%2").arg(a).arg(b)
                      : tr("Added edge %1-%2").arg(a).arg(b));
}

void GraphController::connectSelectedTo(int other)
{
    if (m_graph.isValidIndex(m_selectedNode))
        toggleEdge(m_selectedNode, other);
}

void GraphController::setNodePosition(int index, qreal x, qreal y)
{
    if (!m_graph.isValidIndex(index))
        return;
    m_graph.node(index).pos = QPointF(x, y);
    m_graph.node(index).velocity = QPointF();
    emit positionsChanged();
    m_nodeModel->notifyPositionsChanged();
}

void GraphController::setNodePinned(int index, bool pinned)
{
    if (!m_graph.isValidIndex(index))
        return;
    m_graph.node(index).pinned = pinned;
    m_nodeModel->notifyStylingChanged();
    setStatus(pinned ? tr("Pinned node %1").arg(index) : tr("Released node %1").arg(index));
}

void GraphController::toggleNodePinned(int index)
{
    if (m_graph.isValidIndex(index))
        setNodePinned(index, !m_graph.node(index).pinned);
}

void GraphController::nudgeLayout()
{
    m_layout.reheat();
    setLayoutRunning(true);
}

// ---------------------------------------------------------------------------
// Generators
// ---------------------------------------------------------------------------

void GraphController::generatePath(int n)
{ onGraphReplaced(GraphGenerators::path(qBound(1, n, 2000)), tr("Path P%1").arg(n)); }

void GraphController::generateCycle(int n)
{ onGraphReplaced(GraphGenerators::cycle(qBound(3, n, 2000)), tr("Cycle C%1").arg(n)); }

void GraphController::generateComplete(int n)
{ onGraphReplaced(GraphGenerators::complete(qBound(1, n, 200)), tr("Complete K%1").arg(n)); }

void GraphController::generateCompleteBipartite(int m, int n)
{
    onGraphReplaced(GraphGenerators::completeBipartite(qBound(1, m, 100), qBound(1, n, 100)),
                    tr("Complete bipartite K%1,%2").arg(m).arg(n));
}

void GraphController::generateGrid(int cols, int rows)
{
    onGraphReplaced(GraphGenerators::grid(qBound(1, cols, 80), qBound(1, rows, 80)),
                    tr("%1x%2 grid").arg(cols).arg(rows));
}

void GraphController::generateStar(int leaves)
{ onGraphReplaced(GraphGenerators::star(qBound(1, leaves, 500)), tr("Star K1,%1").arg(leaves)); }

void GraphController::generateWheel(int rim)
{ onGraphReplaced(GraphGenerators::wheel(qBound(3, rim, 500)), tr("Wheel W%1").arg(rim)); }

void GraphController::generateTree(int depth, int branching)
{
    onGraphReplaced(GraphGenerators::tree(qBound(0, depth, 12), qBound(1, branching, 8)),
                    tr("Tree, depth %1, branching %2").arg(depth).arg(branching));
}

void GraphController::generatePetersen()
{ onGraphReplaced(GraphGenerators::petersen(), tr("Petersen graph")); }

void GraphController::generateHypercube(int dimension)
{
    const int d = qBound(1, dimension, 9);
    onGraphReplaced(GraphGenerators::hypercube(d), tr("Hypercube Q%1").arg(d));
}

void GraphController::generateRandom(int n, int percent)
{
    const double p = qBound(0, percent, 100) / 100.0;
    onGraphReplaced(GraphGenerators::erdosRenyi(qBound(1, n, 1500), p,
                                                QRandomGenerator::global()->generate()),
                    tr("Random G(%1, %2)").arg(n).arg(p, 0, 'g', 2));
}

// ---------------------------------------------------------------------------
// Layouts
// ---------------------------------------------------------------------------

void GraphController::applyCircularLayout()
{
    setLayoutRunning(false);
    Layouts::circular(m_graph);
    emit positionsChanged();
    m_nodeModel->notifyPositionsChanged();
    setStatus(tr("Circular layout"));
}

void GraphController::applyGridLayout()
{
    setLayoutRunning(false);
    Layouts::grid(m_graph);
    emit positionsChanged();
    m_nodeModel->notifyPositionsChanged();
    setStatus(tr("Grid layout"));
}

void GraphController::applyRandomLayout()
{
    setLayoutRunning(false);
    Layouts::random(m_graph);
    emit positionsChanged();
    m_nodeModel->notifyPositionsChanged();
    setStatus(tr("Random layout"));
}

void GraphController::applyRadialLayout()
{
    if (m_graph.nodeCount() == 0)
        return;
    setLayoutRunning(false);
    const int root = m_graph.isValidIndex(m_selectedNode) ? m_selectedNode : 0;
    Layouts::radialTree(m_graph, root);
    emit positionsChanged();
    m_nodeModel->notifyPositionsChanged();
    setStatus(tr("Radial layout around node %1").arg(root));
}

// ---------------------------------------------------------------------------
// Algorithms.  Each one writes its result into the styling arrays; the renderer
// picks the change up on the next frame without knowing what ran.
// ---------------------------------------------------------------------------

void GraphController::clearHighlights()
{
    resetStyling();
    m_algorithmName.clear();
    emit highlightsChanged();
    m_nodeModel->notifyStylingChanged();
    setStatus(tr("Cleared highlights"));
}

void GraphController::runBreadthFirst()
{
    if (m_graph.nodeCount() == 0)
        return;
    const int source = m_graph.isValidIndex(m_selectedNode) ? m_selectedNode : 0;
    const Traversal t = breadthFirst(m_graph, source);

    resetStyling();
    int maxDistance = 0;
    for (int d : t.distance)
        maxDistance = qMax(maxDistance, d);

    // Colour every node by its distance from the source: a BFS "heat map".
    for (int i = 0; i < m_graph.nodeCount(); ++i) {
        if (t.distance[i] < 0) {
            m_nodeFill[i] = kDimFill;               // unreachable
            continue;
        }
        const double f = maxDistance > 0 ? double(t.distance[i]) / maxDistance : 0.0;
        m_nodeFill[i] = lerpColour(QColor("#4cc9f0"), QColor("#7209b7"), f);
    }
    for (int i = 0; i < m_graph.edgeCount(); ++i)
        m_edgeColour[i] = kDimEdge;
    for (int ei : t.treeEdges) {
        if (ei >= 0) {
            m_edgeColour[ei] = kAccent;
            m_edgeWidth[ei]  = kThickEdgeWidth;     // the BFS tree
        }
    }
    m_nodeRing[source] = kAccent;
    m_nodeRadius[source] *= 1.35f;

    m_algorithmName = tr("BFS from %1").arg(source);
    emit highlightsChanged();
    m_nodeModel->notifyStylingChanged();
    setStatus(tr("BFS from node %1: visited %2 of %3 nodes, eccentricity %4")
                  .arg(source).arg(t.order.size()).arg(m_graph.nodeCount()).arg(maxDistance));
}

void GraphController::runDepthFirst()
{
    if (m_graph.nodeCount() == 0)
        return;
    const int source = m_graph.isValidIndex(m_selectedNode) ? m_selectedNode : 0;
    const Traversal t = depthFirst(m_graph, source);

    resetStyling();
    for (int i = 0; i < m_graph.nodeCount(); ++i)
        m_nodeFill[i] = kDimFill;
    // Shade by visit order rather than by depth, which is what makes DFS look
    // so different from BFS on the same graph.
    for (int k = 0; k < t.order.size(); ++k) {
        const double f = t.order.size() > 1 ? double(k) / (t.order.size() - 1) : 0.0;
        m_nodeFill[t.order[k]] = lerpColour(QColor("#f72585"), QColor("#4cc9f0"), f);
    }
    for (int i = 0; i < m_graph.edgeCount(); ++i)
        m_edgeColour[i] = kDimEdge;
    for (int ei : t.treeEdges) {
        if (ei >= 0) {
            m_edgeColour[ei] = kAccent;
            m_edgeWidth[ei]  = kThickEdgeWidth;
        }
    }
    m_nodeRing[source] = kAccent;
    m_nodeRadius[source] *= 1.35f;

    m_algorithmName = tr("DFS from %1").arg(source);
    emit highlightsChanged();
    m_nodeModel->notifyStylingChanged();
    setStatus(tr("DFS from node %1: visit order %2")
                  .arg(source)
                  .arg(t.order.size() <= 12
                           ? QStringLiteral("[%1]").arg([&t] {
                                 QStringList s;
                                 for (int v : t.order) s << QString::number(v);
                                 return s.join(QStringLiteral(", "));
                             }())
                           : tr("%1 nodes").arg(t.order.size())));
}

void GraphController::runConnectedComponents()
{
    int count = 0;
    const QList<int> labels = connectedComponents(m_graph, &count);

    resetStyling();
    for (int i = 0; i < m_graph.nodeCount(); ++i)
        m_nodeFill[i] = paletteColour(labels[i]);
    for (int i = 0; i < m_graph.edgeCount(); ++i)
        m_edgeColour[i] = paletteColour(labels[m_graph.edge(i).a]).darker(140);

    m_algorithmName = tr("Connected components");
    emit highlightsChanged();
    m_nodeModel->notifyStylingChanged();
    setStatus(tr("%n connected component(s)", nullptr, count));
}

void GraphController::runShortestPath(int from, int to)
{
    if (!m_graph.isValidIndex(from) || !m_graph.isValidIndex(to)) {
        setStatus(tr("Shortest path: invalid endpoints"));
        return;
    }
    const ShortestPath sp = shortestPath(m_graph, from, to);

    resetStyling();
    for (int i = 0; i < m_graph.nodeCount(); ++i)
        m_nodeFill[i] = kDimFill;
    for (int i = 0; i < m_graph.edgeCount(); ++i)
        m_edgeColour[i] = kDimEdge;

    if (!sp.found) {
        m_nodeFill[from] = QColor("#ff6b8a");
        m_nodeFill[to]   = QColor("#ff6b8a");
        m_algorithmName  = tr("Shortest path %1 -> %2").arg(from).arg(to);
        emit highlightsChanged();
        m_nodeModel->notifyStylingChanged();
        setStatus(tr("No path between %1 and %2 - they are in different components")
                      .arg(from).arg(to));
        return;
    }

    for (int v : sp.nodes)
        m_nodeFill[v] = QColor("#5ed3a0");
    for (int ei : sp.edges) {
        m_edgeColour[ei] = kAccent;
        m_edgeWidth[ei]  = kThickEdgeWidth + 1.0f;
    }
    m_nodeFill[from] = QColor("#4cc9f0");
    m_nodeFill[to]   = QColor("#f72585");
    m_nodeRing[from] = kAccent;
    m_nodeRing[to]   = kAccent;
    m_nodeRadius[from] *= 1.35f;
    m_nodeRadius[to]   *= 1.35f;

    m_algorithmName = tr("Shortest path %1 -> %2").arg(from).arg(to);
    emit highlightsChanged();
    m_nodeModel->notifyStylingChanged();
    setStatus(tr("Shortest path %1 -> %2: %3 hops, total weight %4")
                  .arg(from).arg(to).arg(sp.nodes.size() - 1).arg(sp.length, 0, 'g', 4));
}

void GraphController::runMinimumSpanningTree()
{
    const SpanningForest f = minimumSpanningForest(m_graph);

    resetStyling();
    for (int i = 0; i < m_graph.edgeCount(); ++i)
        m_edgeColour[i] = kDimEdge;
    for (int ei : f.edges) {
        m_edgeColour[ei] = QColor("#5ed3a0");
        m_edgeWidth[ei]  = kThickEdgeWidth;
    }

    m_algorithmName = tr("Minimum spanning forest");
    emit highlightsChanged();
    m_nodeModel->notifyStylingChanged();
    setStatus(tr("Spanning forest: %1 edges, total weight %2, %n component(s)",
                 nullptr, f.components)
                  .arg(f.edges.size()).arg(f.weight, 0, 'g', 4));
}

void GraphController::runBipartiteCheck()
{
    QList<int> colours;
    int oddEdge = -1;
    const bool bipartite = twoColouring(m_graph, &colours, &oddEdge);

    resetStyling();
    for (int i = 0; i < m_graph.nodeCount(); ++i) {
        m_nodeFill[i] = colours[i] < 0 ? kDimFill
                                       : (colours[i] == 0 ? QColor("#4cc9f0") : QColor("#ffd166"));
    }
    if (!bipartite && oddEdge >= 0) {
        m_edgeColour[oddEdge] = QColor("#f72585");
        m_edgeWidth[oddEdge]  = kThickEdgeWidth + 1.0f;
    }

    m_algorithmName = tr("2-colouring");
    emit highlightsChanged();
    m_nodeModel->notifyStylingChanged();
    setStatus(bipartite ? tr("The graph is bipartite - a valid 2-colouring exists")
                        : tr("Not bipartite: edge %1-%2 closes an odd cycle")
                              .arg(m_graph.edge(oddEdge).a).arg(m_graph.edge(oddEdge).b));
}

void GraphController::runDegreeCentrality()
{
    const QList<double> centrality = degreeCentrality(m_graph);

    resetStyling();
    double best = 0.0;
    int bestNode = -1;
    for (int i = 0; i < centrality.size(); ++i) {
        if (centrality[i] > best) {
            best = centrality[i];
            bestNode = i;
        }
    }
    for (int i = 0; i < m_graph.nodeCount(); ++i) {
        const double f = best > 0.0 ? centrality[i] / best : 0.0;
        m_nodeFill[i]   = lerpColour(QColor("#22303f"), QColor("#ff8f5e"), f);
        m_nodeRadius[i] = kBaseRadius * float(0.75 + 0.9 * f);
    }
    if (bestNode >= 0)
        m_nodeRing[bestNode] = kAccent;

    m_algorithmName = tr("Degree centrality");
    emit highlightsChanged();
    m_nodeModel->notifyStylingChanged();
    setStatus(bestNode >= 0
                  ? tr("Most central node: %1 (degree %2, centrality %3)")
                        .arg(bestNode).arg(m_graph.degree(bestNode)).arg(best, 0, 'f', 3)
                  : tr("Degree centrality: empty graph"));
}

// ---------------------------------------------------------------------------
// Persistence - a tiny JSON format, using Qt's JSON classes.
// ---------------------------------------------------------------------------

bool GraphController::saveToFile(const QUrl &fileUrl)
{
    // QML file dialogs hand back a URL; QFile needs a local path.
    const QString path = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();

    QJsonArray nodes;
    for (const Node &n : m_graph.nodes()) {
        nodes.append(QJsonObject{ { QStringLiteral("label"),  n.label },
                                  { QStringLiteral("x"),      n.pos.x() },
                                  { QStringLiteral("y"),      n.pos.y() },
                                  { QStringLiteral("pinned"), n.pinned } });
    }
    QJsonArray edges;
    for (const Edge &e : m_graph.edges()) {
        edges.append(QJsonObject{ { QStringLiteral("a"),      e.a },
                                  { QStringLiteral("b"),      e.b },
                                  { QStringLiteral("weight"), e.weight } });
    }

    const QJsonObject root{ { QStringLiteral("format"), QStringLiteral("graphlab-1") },
                            { QStringLiteral("nodes"),  nodes },
                            { QStringLiteral("edges"),  edges } };

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        setStatus(tr("Could not write %1: %2").arg(path, file.errorString()));
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    setStatus(tr("Saved %1 nodes and %2 edges to %3")
                  .arg(m_graph.nodeCount()).arg(m_graph.edgeCount()).arg(path));
    return true;
}

bool GraphController::loadFromFile(const QUrl &fileUrl)
{
    const QString path = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStatus(tr("Could not read %1: %2").arg(path, file.errorString()));
        return false;
    }

    QJsonParseError error{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        setStatus(tr("%1 is not valid JSON: %2").arg(path, error.errorString()));
        return false;
    }

    const QJsonObject root = doc.object();
    Graph loaded;
    for (const QJsonValue &v : root.value(QStringLiteral("nodes")).toArray()) {
        const QJsonObject o = v.toObject();
        const int i = loaded.addNode(o.value(QStringLiteral("label")).toString(),
                                     QPointF(o.value(QStringLiteral("x")).toDouble(),
                                             o.value(QStringLiteral("y")).toDouble()));
        loaded.node(i).pinned = o.value(QStringLiteral("pinned")).toBool();
    }
    int skipped = 0;
    for (const QJsonValue &v : root.value(QStringLiteral("edges")).toArray()) {
        const QJsonObject o = v.toObject();
        if (!loaded.addEdge(o.value(QStringLiteral("a")).toInt(-1),
                            o.value(QStringLiteral("b")).toInt(-1),
                            o.value(QStringLiteral("weight")).toDouble(1.0)))
            ++skipped;
    }

    onGraphReplaced(std::move(loaded), QFileInfo(path).fileName());
    if (skipped > 0)
        setStatus(tr("Loaded %1 - skipped %2 invalid edge(s)").arg(path).arg(skipped));
    return true;
}

// ---------------------------------------------------------------------------
// Small read-only helpers that QML calls directly
// ---------------------------------------------------------------------------

QString GraphController::nodeLabel(int index) const
{
    return m_graph.isValidIndex(index) ? m_graph.node(index).label : QString();
}

int GraphController::nodeDegree(int index) const
{
    return m_graph.isValidIndex(index) ? m_graph.degree(index) : 0;
}

bool GraphController::nodeIsPinned(int index) const
{
    return m_graph.isValidIndex(index) && m_graph.node(index).pinned;
}

QList<int> GraphController::neighbours(int index) const
{
    return m_graph.isValidIndex(index) ? m_graph.neighbours(index) : QList<int>();
}
