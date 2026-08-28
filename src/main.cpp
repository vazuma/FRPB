// ---------------------------------------------------------------------------
// GraphLab - entry point.
//
// The interesting part of a Qt Quick main() is the ordering: a few decisions
// have to be made *before* the first window exists, because they select the
// graphics stack the whole scene graph will run on.
// ---------------------------------------------------------------------------

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSurfaceFormat>

int main(int argc, char *argv[])
{
    // 1. Request an OpenGL 3.3 core profile context.  Our shaders say
    //    "#version 330 core", so anything older would refuse to compile them.
    //    setDefaultFormat() must be called before any window is created.
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);                       // window-level multisampling
    QSurfaceFormat::setDefaultFormat(format);

    // 2. Qt 6 draws Qt Quick through the RHI, an abstraction that may pick
    //    Vulkan, Metal, D3D or OpenGL depending on the platform.  A
    //    QQuickFramebufferObject only works on the OpenGL backend, so ask for
    //    it explicitly instead of hoping for the default.
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("GraphLab"));
    QGuiApplication::setOrganizationName(QStringLiteral("GraphLab"));
    QGuiApplication::setApplicationVersion(QStringLiteral("1.0"));

    QQmlApplicationEngine engine;

    // qt_add_qml_module put the compiled QML under this resource prefix; adding
    // it as an import path is what lets `import GraphLab` resolve.
    engine.addImportPath(QStringLiteral("qrc:/qt/qml"));

    const QUrl url(QStringLiteral("qrc:/qt/qml/GraphLab/qml/Main.qml"));

    // If the root object fails to instantiate (a QML syntax error, a missing
    // import) we get a null object here - exit loudly rather than hanging with
    // no window.
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [url](QObject *object, const QUrl &objectUrl) {
            if (!object && url == objectUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
