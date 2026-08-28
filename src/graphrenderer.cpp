#include "graphrenderer.h"
#include "graphcontroller.h"
#include "graphview.h"
#include "shaders.h"

#include <QOpenGLFramebufferObjectFormat>
#include <QtMath>

namespace {

// Layout of one edge vertex, in floats:  x y  nx ny  halfWidth  r g b a
constexpr int kEdgeFloatsPerVertex = 9;
constexpr int kEdgeVerticesPerEdge = 6;      // two triangles

// Layout of one node instance, in floats: cx cy  radius  fill(4)  ring(4)  ringWidth
constexpr int kNodeFloatsPerInstance = 12;

void appendEdgeVertex(QList<float> &out, QPointF pos, QPointF normal,
                      float halfWidth, const QColor &c)
{
    out << float(pos.x()) << float(pos.y())
        << float(normal.x()) << float(normal.y())
        << halfWidth
        << float(c.redF()) << float(c.greenF()) << float(c.blueF()) << float(c.alphaF());
}

} // namespace

GraphRenderer::GraphRenderer() = default;

GraphRenderer::~GraphRenderer()
{
    // The GL context is still current here, so destroying the buffers is safe.
    m_edgeVao.destroy();
    m_nodeVao.destroy();
    m_edgeVbo.destroy();
    m_quadVbo.destroy();
    m_instanceVbo.destroy();
}

QOpenGLFramebufferObject *GraphRenderer::createFramebufferObject(const QSize &size)
{
    // 4x multisampling: the cheapest way to make the disc rims and the thin
    // edges look clean.  Qt resolves the multisampled FBO for us.
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);
    return new QOpenGLFramebufferObject(size, format);
}

// ---------------------------------------------------------------------------
// synchronize(): the GUI thread is blocked, so reading the controller's graph
// is safe.  We turn it into flat float arrays and touch nothing else afterwards.
// ---------------------------------------------------------------------------
void GraphRenderer::synchronize(QQuickFramebufferObject *item)
{
    auto *view = static_cast<GraphView *>(item);

    m_background    = view->background();
    m_pixelsPerUnit = float(view->zoom());
    m_showEdges     = view->showEdges();

    const qreal w = qMax<qreal>(1.0, view->width());
    const qreal h = qMax<qreal>(1.0, view->height());
    const QPointF c = view->centre();
    const qreal halfW = w / (2.0 * view->zoom());
    const qreal halfH = h / (2.0 * view->zoom());

    // An orthographic projection with `bottom` > `top` flips the y axis, so
    // world y grows downwards and matches Qt's item coordinates.
    m_mvp.setToIdentity();
    m_mvp.ortho(float(c.x() - halfW), float(c.x() + halfW),
                float(c.y() + halfH), float(c.y() - halfH),
                -1.0f, 1.0f);

    const GraphController *controller = view->controller();
    if (!controller) {
        m_edgeVertices.clear();
        m_nodeInstances.clear();
        m_edgeVertexCount = 0;
        m_nodeInstanceCount = 0;
        m_geometryDirty = true;
        return;
    }

    const Graph &g = controller->graph();
    const QList<QColor> &fills  = controller->nodeFillColours();
    const QList<QColor> &rings  = controller->nodeRingColours();
    const QList<float>  &radii  = controller->nodeRadii();
    const QList<QColor> &eCols  = controller->edgeColours();
    const QList<float>  &eWidth = controller->edgeWidths();
    const int selected = controller->selectedNode();

    // --- edges: expand each into a quad ------------------------------------
    m_edgeVertices.clear();
    m_edgeVertices.reserve(g.edgeCount() * kEdgeVerticesPerEdge * kEdgeFloatsPerVertex);
    for (int i = 0; i < g.edgeCount(); ++i) {
        const Edge &e = g.edge(i);
        const QPointF a = g.node(e.a).pos;
        const QPointF b = g.node(e.b).pos;

        QPointF dir = b - a;
        const qreal len = std::hypot(dir.x(), dir.y());
        if (len < 1e-6)
            continue;                       // degenerate; nothing to draw
        dir /= len;
        const QPointF normal(-dir.y(), dir.x());

        const QColor colour = i < eCols.size() ? eCols.at(i) : QColor(Qt::gray);
        const float half = (i < eWidth.size() ? eWidth.at(i) : 1.5f) * 0.5f;

        // Two triangles: (a-, a+, b-) and (b-, a+, b+)
        appendEdgeVertex(m_edgeVertices, a, -normal, half, colour);
        appendEdgeVertex(m_edgeVertices, a,  normal, half, colour);
        appendEdgeVertex(m_edgeVertices, b, -normal, half, colour);

        appendEdgeVertex(m_edgeVertices, b, -normal, half, colour);
        appendEdgeVertex(m_edgeVertices, a,  normal, half, colour);
        appendEdgeVertex(m_edgeVertices, b,  normal, half, colour);
    }
    m_edgeVertexCount = m_edgeVertices.size() / kEdgeFloatsPerVertex;

    // --- nodes: one instance each ------------------------------------------
    m_nodeInstances.clear();
    m_nodeInstances.reserve(g.nodeCount() * kNodeFloatsPerInstance);
    for (int i = 0; i < g.nodeCount(); ++i) {
        const Node &n = g.node(i);
        const QColor fill = i < fills.size() ? fills.at(i) : QColor("#6d8ba8");
        QColor ring       = i < rings.size() ? rings.at(i) : QColor("#0f1720");
        float ringWidth   = 0.16f;

        if (i == selected) {                  // the selection cue
            ring = QColor("#ffffff");
            ringWidth = 0.30f;
        } else if (n.pinned) {
            ring = QColor("#ff8f5e");
            ringWidth = 0.26f;
        }

        m_nodeInstances << float(n.pos.x()) << float(n.pos.y())
                        << (i < radii.size() ? radii.at(i) : 13.0f)
                        << float(fill.redF()) << float(fill.greenF())
                        << float(fill.blueF()) << float(fill.alphaF())
                        << float(ring.redF()) << float(ring.greenF())
                        << float(ring.blueF()) << float(ring.alphaF())
                        << ringWidth;
    }
    m_nodeInstanceCount = m_nodeInstances.size() / kNodeFloatsPerInstance;

    m_geometryDirty = true;
}

// ---------------------------------------------------------------------------
// GL setup, done once on the render thread
// ---------------------------------------------------------------------------
void GraphRenderer::initialiseGL()
{
    initializeOpenGLFunctions();

    m_edgeProgram.addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, Shaders::edgeVertex);
    m_edgeProgram.addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, Shaders::edgeFragment);
    if (!m_edgeProgram.link())
        qWarning("GraphRenderer: edge program failed to link: %s",
                 qPrintable(m_edgeProgram.log()));

    m_nodeProgram.addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, Shaders::nodeVertex);
    m_nodeProgram.addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, Shaders::nodeFragment);
    if (!m_nodeProgram.link())
        qWarning("GraphRenderer: node program failed to link: %s",
                 qPrintable(m_nodeProgram.log()));

    // --- edge VAO ----------------------------------------------------------
    m_edgeVao.create();
    m_edgeVao.bind();
    m_edgeVbo.create();
    m_edgeVbo.bind();
    m_edgeVbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    {
        const int stride = kEdgeFloatsPerVertex * int(sizeof(float));
        auto attrib = [&](int loc, int size, int offsetFloats) {
            glEnableVertexAttribArray(loc);
            glVertexAttribPointer(loc, size, GL_FLOAT, GL_FALSE, stride,
                                  reinterpret_cast<void *>(quintptr(offsetFloats * sizeof(float))));
        };
        attrib(0, 2, 0);    // aPos
        attrib(1, 2, 2);    // aNormal
        attrib(2, 1, 4);    // aHalfWidth
        attrib(3, 4, 5);    // aColour
    }
    m_edgeVao.release();

    // --- node VAO ----------------------------------------------------------
    m_nodeVao.create();
    m_nodeVao.bind();

    // The unit quad never changes, so it is uploaded once as a static buffer.
    static const float quad[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f,
    };
    m_quadVbo.create();
    m_quadVbo.bind();
    m_quadVbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    m_quadVbo.allocate(quad, sizeof(quad));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

    m_instanceVbo.create();
    m_instanceVbo.bind();
    m_instanceVbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    {
        const int stride = kNodeFloatsPerInstance * int(sizeof(float));
        auto instanceAttrib = [&](int loc, int size, int offsetFloats) {
            glEnableVertexAttribArray(loc);
            glVertexAttribPointer(loc, size, GL_FLOAT, GL_FALSE, stride,
                                  reinterpret_cast<void *>(quintptr(offsetFloats * sizeof(float))));
            // Divisor 1 = advance once per instance instead of once per vertex.
            // This single call is what turns the quad into "one quad per node".
            glVertexAttribDivisor(loc, 1);
        };
        instanceAttrib(1, 2, 0);    // aCentre
        instanceAttrib(2, 1, 2);    // aRadius
        instanceAttrib(3, 4, 3);    // aFill
        instanceAttrib(4, 4, 7);    // aRing
        instanceAttrib(5, 1, 11);   // aRingWidth
    }
    m_nodeVao.release();

    m_initialised = true;
}

void GraphRenderer::uploadBuffers()
{
    if (!m_geometryDirty)
        return;

    m_edgeVbo.bind();
    m_edgeVbo.allocate(m_edgeVertices.constData(),
                       int(m_edgeVertices.size() * sizeof(float)));
    m_edgeVbo.release();

    m_instanceVbo.bind();
    m_instanceVbo.allocate(m_nodeInstances.constData(),
                           int(m_nodeInstances.size() * sizeof(float)));
    m_instanceVbo.release();

    m_geometryDirty = false;
}

// ---------------------------------------------------------------------------
// render(): our FBO is already bound and sized by Qt
// ---------------------------------------------------------------------------
void GraphRenderer::render()
{
    if (!m_initialised)
        initialiseGL();

    uploadBuffers();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    // Premultiplied alpha, which is what both fragment shaders output and what
    // the Qt Quick scene graph expects from an FBO item's texture.
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(float(m_background.redF()), float(m_background.greenF()),
                 float(m_background.blueF()), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    if (m_showEdges && m_edgeVertexCount > 0) {
        m_edgeProgram.bind();
        m_edgeProgram.setUniformValue("uMvp", m_mvp);
        m_edgeProgram.setUniformValue("uPixelsPerUnit", m_pixelsPerUnit);
        m_edgeVao.bind();
        glDrawArrays(GL_TRIANGLES, 0, m_edgeVertexCount);
        m_edgeVao.release();
        m_edgeProgram.release();
    }

    if (m_nodeInstanceCount > 0) {
        m_nodeProgram.bind();
        m_nodeProgram.setUniformValue("uMvp", m_mvp);
        m_nodeProgram.setUniformValue("uPixelsPerUnit", m_pixelsPerUnit);
        m_nodeVao.bind();
        // 4 vertices of the shared quad, m_nodeInstanceCount instances.
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, m_nodeInstanceCount);
        m_nodeVao.release();
        m_nodeProgram.release();
    }

    // NB: we deliberately do *not* call update() here.  Doing so would request
    // another frame immediately and spin the GPU at full speed forever.  New
    // frames are requested by GraphView, which calls update() when the
    // controller says something actually changed.
}
