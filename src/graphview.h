#pragma once

// ---------------------------------------------------------------------------
// GraphView - the QML-visible item.  Everything it exposes is either a property
// QML binds to (camera, colours) or an invokable helper QML calls from its
// input handlers (hit testing, coordinate conversion).
//
// It draws nothing itself: createRenderer() hands Qt a GraphRenderer and Qt
// takes care of scheduling it on the render thread.
// ---------------------------------------------------------------------------

#include <QColor>
#include <QPointF>
#include <QQmlEngine>
#include <QQuickFramebufferObject>

class GraphController;

class GraphView : public QQuickFramebufferObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(GraphController *controller READ controller WRITE setController NOTIFY controllerChanged)
    Q_PROPERTY(qreal zoom       READ zoom       WRITE setZoom       NOTIFY cameraChanged)
    Q_PROPERTY(QPointF centre   READ centre     WRITE setCentre     NOTIFY cameraChanged)
    Q_PROPERTY(QColor background READ background WRITE setBackground NOTIFY backgroundChanged)
    Q_PROPERTY(bool showEdges   READ showEdges  WRITE setShowEdges  NOTIFY showEdgesChanged)

public:
    explicit GraphView(QQuickItem *parent = nullptr);

    GraphController *controller() const { return m_controller; }
    void setController(GraphController *controller);

    qreal zoom() const { return m_zoom; }
    void  setZoom(qreal zoom);

    QPointF centre() const { return m_centre; }
    void    setCentre(QPointF centre);

    QColor background() const { return m_background; }
    void   setBackground(const QColor &colour);

    bool showEdges() const { return m_showEdges; }
    void setShowEdges(bool show);

    Renderer *createRenderer() const override;

    // --- helpers QML calls from its pointer handlers -----------------------

    // Item (pixel) coordinates <-> world coordinates.
    Q_INVOKABLE QPointF toWorld(QPointF itemPos) const;
    Q_INVOKABLE QPointF toItem(QPointF worldPos) const;

    // Index of the node under the given item position, or -1.
    Q_INVOKABLE int nodeAt(QPointF itemPos, qreal slackPixels = 6.0) const;

    // Zoom keeping the world point under `itemPos` fixed - the behaviour every
    // map application has trained users to expect from the scroll wheel.
    Q_INVOKABLE void zoomAt(QPointF itemPos, qreal factor);

    // Frame the whole graph.
    Q_INVOKABLE void fitToContents(qreal marginPixels = 60.0);
    Q_INVOKABLE void resetCamera();

signals:
    void controllerChanged();
    void cameraChanged();
    void backgroundChanged();
    void showEdgesChanged();

private:
    GraphController *m_controller = nullptr;
    qreal   m_zoom = 1.0;
    QPointF m_centre;               // world point shown at the item's centre
    QColor  m_background = QColor("#11161d");
    bool    m_showEdges = true;
};
