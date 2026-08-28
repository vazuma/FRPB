# ---------------------------------------------------------------------------
# GraphLab - a Qt Quick + OpenGL playground for graph theory
# ---------------------------------------------------------------------------
# Qt 6 projects are built with CMake.  Qt ships its own CMake commands
# (qt_add_executable, qt_add_qml_module, ...) that wrap the plain CMake ones
# and take care of moc / rcc / uic / qmlcachegen / qmltyperegistrar for us.
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.21)

project(GraphLab VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# AUTOMOC runs Qt's meta-object compiler over every header that contains
# Q_OBJECT.  Without it, signals/slots/properties simply do not exist.
set(CMAKE_AUTOMOC ON)

# Core     - QObject, containers, QJsonDocument, QTimer
# Gui      - QColor, QMatrix4x4, QOpenGL* (Qt 6 keeps OpenGL helpers in Gui/OpenGL)
# Qml/Quick- the QML engine and the Qt Quick scene graph
# QuickControls2 - MenuBar, Dialog, SplitView, ToolBar and friends
# OpenGL   - QOpenGLShaderProgram, QOpenGLBuffer, QOpenGLVertexArrayObject
# Test     - only needed by the unit tests below
find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 OpenGL)
find_package(Qt6 6.2 QUIET COMPONENTS Test)

# Generates the code that turns QML_ELEMENT annotations into registered types.
qt_standard_project_setup()

# ---------------------------------------------------------------------------
# The core graph library: deliberately free of any GUI dependency so that it
# can be unit tested head-less.  It only uses QtCore types.
# ---------------------------------------------------------------------------
add_library(graphcore STATIC
    src/graph.h                 src/graph.cpp
    src/graphalgorithms.h       src/graphalgorithms.cpp
    src/graphgenerators.h       src/graphgenerators.cpp
    src/forcelayout.h           src/forcelayout.cpp
)
target_link_libraries(graphcore PUBLIC Qt6::Core Qt6::Gui)
target_include_directories(graphcore PUBLIC src)

# ---------------------------------------------------------------------------
# The application
# ---------------------------------------------------------------------------
qt_add_executable(graphlab
    src/main.cpp
    src/graphcontroller.h       src/graphcontroller.cpp
    src/nodelistmodel.h         src/nodelistmodel.cpp
    src/graphview.h             src/graphview.cpp
    src/graphrenderer.h         src/graphrenderer.cpp
    src/shaders.h
)

# Theme.qml is a singleton (`pragma Singleton`).  CMake has to be told, so that
# the generated qmldir contains the matching `singleton` keyword.
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

# Declares a QML module named "GraphLab".  Every .qml file listed here is
# compiled to byte code and baked into the binary as a Qt resource, and every
# C++ class in `graphlab` marked with QML_ELEMENT is registered into the module.
qt_add_qml_module(graphlab
    URI GraphLab
    VERSION 1.0
    RESOURCE_PREFIX "/qt/qml"
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/Viewport.qml
        qml/InspectorPanel.qml
        qml/ParameterDialog.qml
        qml/AboutDialog.qml
        qml/StatRow.qml
)

target_link_libraries(graphlab PRIVATE
    graphcore
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::OpenGL
)

# ---------------------------------------------------------------------------
# Unit tests for the pure C++ core (ctest)
# ---------------------------------------------------------------------------
if (TARGET Qt6::Test)
    enable_testing()
    qt_add_executable(tst_graphcore tests/tst_graphcore.cpp)
    target_link_libraries(tst_graphcore PRIVATE graphcore Qt6::Test)
    add_test(NAME graphcore COMMAND tst_graphcore)
endif()
