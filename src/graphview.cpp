#include "graphview.h"
#include "graphcontroller.h"
#include "graphrenderer.h"

#include <QtMath>

GraphView::GraphView(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
{
    // An OpenGL framebuffer has its origin in the BOTTOM left, while Qt Quick
    // items count y downwards from the TOP left.  Left alone (the default,
    // mirrorVertically == false) Qt hands the texture to the scene graph as-is,
    // so clip-space y = +1 ends up at the item's *bottom* edge and everything
    // the renderer draws appears upside down.
    //
    // Setting this to true presents the FBO with a top-left origin, which lets
    // the whole rest of the class - toWorld(), toItem(), nodeAt(), and the
    // y-down orthographic projection in GraphRenderer::synchronize() - share one
    // consistent convention with Qt's item coordinates.
    //
    // This is easy to get wrong *and* easy to miss: a graph whose node positions
    // happen to be symmetric about the horizontal axis (a circular layout, a
    // regular grid, the Petersen graph) looks perfectly correct while flipped,
    // because the mirrored discs land back on top of the un-mirrored ones.  Test
    // orientation with something asymmetric, such as Generate > Tree.
    setMirrorVertically(true);
    setTextureFollowsItemSize(true);
}

QQuickFramebufferObject::Renderer *GraphView::createRenderer() const
{
    // Qt calls this on the render thread and takes ownership of the result.
    return new GraphRenderer;
}

void GraphView::setController(GraphController *controller)
{
    if (m_controller == controller)
        return;

    if (m_controller)
        m_controller->disconnect(this);

    m_controller = controller;

    if (m_controller) {
        // Any of these means "the picture is stale" - schedule a new frame.
        // update() is the only correct way to ask for one; painting directly
        // from the GUI thread is not possible here.
        connect(m_controller, &GraphController::graphChanged,     this, [this] { update(); });
        connect(m_controller, &GraphController::positionsChanged, this, [this] { update(); });
        connect(m_controller, &GraphController::highlightsChanged,this, [this] { update(); });
        connect(m_controller, &GraphController::selectedNodeChanged, this, [this] { update(); });
    }

    emit controllerChanged();
    update();
}

void GraphView::setZoom(qreal zoom)
{
    const qreal clamped = qBound(0.02, zoom, 40.0);
    if (qFuzzyCompare(m_zoom, clamped))
        return;
    m_zoom = clamped;
    emit cameraChanged();
    update();
}

void GraphView::setCentre(QPointF centre)
{
    if (m_centre == centre)
        return;
    m_centre = centre;
    emit cameraChanged();
    update();
}

void GraphView::setBackground(const QColor &colour)
{
    if (m_background == colour)
        return;
    m_background = colour;
    emit backgroundChanged();
    update();
}

void GraphView::setShowEdges(bool show)
{
    if (m_showEdges == show)
        return;
    m_showEdges = show;
    emit showEdgesChanged();
    update();
}

// ---------------------------------------------------------------------------
// Coordinate conversion.  The camera is "world point `centre` sits at the
// middle of the item, scaled by `zoom` pixels per world unit".
// ---------------------------------------------------------------------------

QPointF GraphView::toWorld(QPointF itemPos) const
{
    return m_centre + (itemPos - QPointF(width() / 2.0, height() / 2.0)) / m_zoom;
}

QPointF GraphView::toItem(QPointF worldPos) const
{
    return (worldPos - m_centre) * m_zoom + QPointF(width() / 2.0, height() / 2.0);
}

int GraphView::nodeAt(QPointF itemPos, qreal slackPixels) const
{
    if (!m_controller)
        return -1;

    const Graph &g = m_controller->graph();
    const QList<float> &radii = m_controller->nodeRadii();
    const QPointF world = toWorld(itemPos);

    // Walk backwards so that the node drawn last (on top) wins a tie.
    int best = -1;
    qreal bestDistance = 0.0;
    for (int i = g.nodeCount() - 1; i >= 0; --i) {
        const qreal radius = (i < radii.size() ? radii.at(i) : 13.0) + slackPixels / m_zoom;
        const QPointF d = g.node(i).pos - world;
        const qreal distance = std::hypot(d.x(), d.y());
        if (distance <= radius && (best < 0 || distance < bestDistance)) {
            best = i;
            bestDistance = distance;
        }
    }
    return best;
}

void GraphView::zoomAt(QPointF itemPos, qreal factor)
{
    const QPointF anchor = toWorld(itemPos);       // world point under the cursor
    const qreal before = m_zoom;
    setZoom(m_zoom * factor);
    if (qFuzzyCompare(before, m_zoom))
        return;                                    // clamped: leave the centre alone

    // Move the centre so `anchor` lands back under the cursor.
    setCentre(anchor - (itemPos - QPointF(width() / 2.0, height() / 2.0)) / m_zoom);
}

void GraphView::fitToContents(qreal marginPixels)
{
    if (!m_controller || m_controller->graph().nodeCount() == 0)
        return;

    QRectF box = m_controller->graph().boundingBox();
    // A single node has a zero-size box; pad it so the maths below behaves.
    box.adjust(-40, -40, 40, 40);

    const qreal availableW = qMax(1.0, width()  - 2 * marginPixels);
    const qreal availableH = qMax(1.0, height() - 2 * marginPixels);
    const qreal zoom = qMin(availableW / box.width(), availableH / box.height());

    setZoom(zoom);
    setCentre(box.center());
}

void GraphView::resetCamera()
{
    setZoom(1.0);
    setCentre(QPointF(0, 0));
}
