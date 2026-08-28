#pragma once

// ---------------------------------------------------------------------------
// GraphController - the single bridge between the plain C++ core and QML.
//
// This is where the Qt-specific vocabulary lives:
//
//   Q_OBJECT     enables the meta-object system (signals, slots, properties)
//   Q_PROPERTY   exposes a value to QML *with a change notification*, which is
//                what makes declarative bindings update automatically
//   Q_INVOKABLE  makes a plain method callable from QML
//   QML_ELEMENT  registers the class as a QML type named after the class,
//                in the QML module declared by qt_add_qml_module in CMake
// ---------------------------------------------------------------------------

#include <QColor>
#include <QObject>
#include <QQmlEngine>          // for QML_ELEMENT
#include <QTimer>
#include <QUrl>

#include "forcelayout.h"
#include "graph.h"
// The Q_PROPERTY below returns a NodeListModel*, and the meta-object system
// needs the complete type - a forward declaration is not enough.
#include "nodelistmodel.h"

class GraphController : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    // --- read-only statistics, all refreshed by graphChanged() -------------
    Q_PROPERTY(int nodeCount      READ nodeCount                      NOTIFY graphChanged)
    Q_PROPERTY(int edgeCount      READ edgeCount                      NOTIFY graphChanged)
    Q_PROPERTY(int componentCount READ componentCount                 NOTIFY graphChanged)
    Q_PROPERTY(qreal density      READ density                        NOTIFY graphChanged)
    Q_PROPERTY(qreal averageDegree READ averageDegree                 NOTIFY graphChanged)
    Q_PROPERTY(int maxDegree      READ maxDegree                      NOTIFY graphChanged)

    // --- editable state ----------------------------------------------------
    Q_PROPERTY(int selectedNode   READ selectedNode  WRITE setSelectedNode  NOTIFY selectedNodeChanged)
    Q_PROPERTY(bool layoutRunning READ layoutRunning WRITE setLayoutRunning NOTIFY layoutRunningChanged)
    Q_PROPERTY(QString statusText READ statusText                     NOTIFY statusTextChanged)
    Q_PROPERTY(QString algorithmName READ algorithmName               NOTIFY highlightsChanged)

    // A QAbstractListModel handed straight to a QML Repeater / ListView.
    Q_PROPERTY(NodeListModel *nodeModel READ nodeModel CONSTANT)

public:
    explicit GraphController(QObject *parent = nullptr);
    ~GraphController() override;

    // Not exposed to QML: used by GraphView / GraphRenderer on the C++ side.
    const Graph &graph() const { return m_graph; }
    const QList<QColor> &nodeFillColours() const { return m_nodeFill; }
    const QList<QColor> &nodeRingColours() const { return m_nodeRing; }
    const QList<QColor> &edgeColours() const { return m_edgeColour; }
    const QList<float>  &edgeWidths() const { return m_edgeWidth; }
    const QList<float>  &nodeRadii() const { return m_nodeRadius; }

    int    nodeCount() const { return m_graph.nodeCount(); }
    int    edgeCount() const { return m_graph.edgeCount(); }
    int    componentCount() const;
    qreal  density() const { return m_graph.density(); }
    qreal  averageDegree() const { return m_graph.averageDegree(); }
    int    maxDegree() const { return m_graph.maxDegree(); }

    int  selectedNode() const { return m_selectedNode; }
    void setSelectedNode(int index);

    bool layoutRunning() const { return m_layoutTimer.isActive(); }
    void setLayoutRunning(bool running);

    QString statusText() const { return m_statusText; }
    QString algorithmName() const { return m_algorithmName; }
    NodeListModel *nodeModel() const { return m_nodeModel; }

public slots:
    // --- editing -----------------------------------------------------------
    void clear();
    int  addNodeAt(qreal x, qreal y);
    void removeSelectedNode();
    void toggleEdge(int a, int b);
    void connectSelectedTo(int other);
    void setNodePosition(int index, qreal x, qreal y);
    void setNodePinned(int index, bool pinned);
    void toggleNodePinned(int index);
    void nudgeLayout();                      // reheat the simulation

    // --- generators --------------------------------------------------------
    void generatePath(int n);
    void generateCycle(int n);
    void generateComplete(int n);
    void generateCompleteBipartite(int m, int n);
    void generateGrid(int cols, int rows);
    void generateStar(int leaves);
    void generateWheel(int rim);
    void generateTree(int depth, int branching);
    void generatePetersen();
    void generateHypercube(int dimension);
    void generateRandom(int n, int percent);   // percent = 100 * p, keeps QML simple

    // --- layouts -----------------------------------------------------------
    void applyCircularLayout();
    void applyGridLayout();
    void applyRandomLayout();
    void applyRadialLayout();

    // --- algorithms (results are shown as colours in the viewport) ---------
    void runBreadthFirst();
    void runDepthFirst();
    void runConnectedComponents();
    void runShortestPath(int from, int to);
    void runMinimumSpanningTree();
    void runBipartiteCheck();
    void runDegreeCentrality();
    void clearHighlights();

    // --- persistence -------------------------------------------------------
    bool saveToFile(const QUrl &fileUrl);
    bool loadFromFile(const QUrl &fileUrl);

    // --- helpers for QML ---------------------------------------------------
    QString nodeLabel(int index) const;
    int     nodeDegree(int index) const;
    bool    nodeIsPinned(int index) const;
    QList<int> neighbours(int index) const;

signals:
    void graphChanged();        // topology changed: node/edge counts differ
    void positionsChanged();    // only coordinates moved (cheap, fires ~60 Hz)
    void highlightsChanged();   // per-node / per-edge colours changed
    void selectedNodeChanged();
    void layoutRunningChanged();
    void statusTextChanged();

private:
    void setStatus(const QString &text);
    void resetStyling();                 // back to the neutral palette
    void onGraphReplaced(Graph &&g, const QString &description);
    void stepLayout();

    Graph        m_graph;
    ForceLayout  m_layout;
    QTimer       m_layoutTimer;
    NodeListModel *m_nodeModel = nullptr;

    int     m_selectedNode = -1;
    QString m_statusText;
    QString m_algorithmName;

    // Per-element styling, sized to match the graph.  The renderer copies these
    // verbatim, so algorithms "publish" their results simply by writing colours.
    QList<QColor> m_nodeFill;
    QList<QColor> m_nodeRing;
    QList<float>  m_nodeRadius;
    QList<QColor> m_edgeColour;
    QList<float>  m_edgeWidth;
};
