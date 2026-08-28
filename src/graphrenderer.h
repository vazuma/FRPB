#pragma once

// ---------------------------------------------------------------------------
// GraphRenderer - the render-thread half of the OpenGL viewport.
//
// Qt Quick renders on its own thread.  QQuickFramebufferObject splits an item
// into two objects to make that safe:
//
//   GraphView      lives on the GUI thread, holds the properties QML binds to
//   GraphRenderer  lives on the render thread, owns the GL objects
//
// They only ever meet inside synchronize(), which Qt calls with the GUI thread
// *blocked*.  That is the one and only place where it is legal to read the
// item's state; everything render() needs must be copied across there.
// ---------------------------------------------------------------------------

#include <QColor>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QQuickFramebufferObject>

class GraphRenderer : public QQuickFramebufferObject::Renderer,
                      protected QOpenGLExtraFunctions
{
public:
    GraphRenderer();
    ~GraphRenderer() override;

    // Called on the render thread, GUI thread blocked: copy state here.
    void synchronize(QQuickFramebufferObject *item) override;

    // Called on the render thread with our FBO bound.
    void render() override;

    // Lets us ask for multisampling and a matching size.
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override;

private:
    void initialiseGL();
    void uploadBuffers();

    bool m_initialised = false;

    QOpenGLShaderProgram m_edgeProgram;
    QOpenGLShaderProgram m_nodeProgram;

    QOpenGLVertexArrayObject m_edgeVao;
    QOpenGLBuffer            m_edgeVbo{QOpenGLBuffer::VertexBuffer};

    QOpenGLVertexArrayObject m_nodeVao;
    QOpenGLBuffer            m_quadVbo{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer            m_instanceVbo{QOpenGLBuffer::VertexBuffer};

    // --- the snapshot filled in by synchronize() ---------------------------
    QList<float> m_edgeVertices;    // 9 floats per vertex, 6 vertices per edge
    QList<float> m_nodeInstances;   // 12 floats per node
    int          m_edgeVertexCount = 0;
    int          m_nodeInstanceCount = 0;
    bool         m_geometryDirty = true;

    QMatrix4x4 m_mvp;
    QColor     m_background = Qt::black;
    float      m_pixelsPerUnit = 1.0f;
    bool       m_showEdges = true;
};
