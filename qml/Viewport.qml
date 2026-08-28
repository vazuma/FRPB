// ---------------------------------------------------------------------------
// The viewport: a GraphView (OpenGL, C++) with a layer of Qt Quick on top.
//
// The split is worth studying.  Everything that has to be fast and per-frame -
// thousands of discs and lines - is drawn by the C++ renderer.  Everything that
// is fiddly in raw OpenGL - text labels, the crosshair, the hover readout - is
// ordinary Qt Quick, positioned by bindings against the camera properties.
//
// Input is handled with Qt Quick *Input Handlers* (TapHandler, DragHandler,
// WheelHandler, HoverHandler) rather than nested MouseAreas.  Handlers compose:
// several can watch the same item and negotiate who wins a gesture.
// ---------------------------------------------------------------------------
import QtQuick
import QtQuick.Controls
import GraphLab

Item {
    id: root

    required property GraphController controller
    property alias  view: graphView
    property bool   showLabels: true
    property bool   showEdges: true

    // Which node the pointer is currently over, or -1.
    property int hoveredNode: -1
    // The node currently being dragged, or -1.
    property int draggedNode: -1

    clip: true

    GraphView {
        id: graphView
        anchors.fill: parent

        controller: root.controller
        background: Theme.viewport
        showEdges: root.showEdges

        // Auto-framing is trickier than it looks: at Component.onCompleted an
        // item inside a Layout still has width == height == 0, so fitting there
        // would divide by nothing and clamp the zoom to its minimum.  Instead we
        // record that a fit is *pending* and perform it as soon as the item has
        // both a real size and a graph to frame.
        property bool fitPending: true

        function fitWhenReady() {
            if (fitPending && width > 0 && height > 0 && root.controller.nodeCount > 0) {
                fitToContents()
                fitPending = false
            }
        }

        Component.onCompleted: fitWhenReady()
        onWidthChanged: fitWhenReady()
        onHeightChanged: fitWhenReady()

        Connections {
            target: root.controller
            // A new topology deserves a fresh camera; panning after that is the
            // user's business, so we do not fight them on every position tick.
            function onGraphChanged() {
                graphView.fitPending = true
                graphView.fitWhenReady()
            }
        }

        // --- zooming ------------------------------------------------------
        WheelHandler {
            id: wheelHandler
            // Both mice and trackpads end up here; angleDelta is in 1/8 degree.
            onWheel: function(event) {
                const factor = Math.pow(1.0015, event.angleDelta.y)
                graphView.zoomAt(Qt.point(event.x, event.y), factor)
                event.accepted = true
            }
        }

        // --- hover readout -------------------------------------------------
        HoverHandler {
            id: hoverHandler
            onPointChanged: root.hoveredNode = graphView.nodeAt(point.position)
            onActiveChanged: if (!active) root.hoveredNode = -1
        }

        // --- select / toggle edge / pin -------------------------------------
        TapHandler {
            acceptedButtons: Qt.LeftButton
            onSingleTapped: function(eventPoint, button) {
                const hit = graphView.nodeAt(eventPoint.position)
                if (hit < 0) {
                    root.controller.selectedNode = -1
                } else if (eventPoint.modifiers & Qt.ShiftModifier) {
                    // Shift-click a second node to add or remove the edge.
                    root.controller.connectSelectedTo(hit)
                } else {
                    root.controller.selectedNode = hit
                }
            }
            onDoubleTapped: function(eventPoint, button) {
                if (graphView.nodeAt(eventPoint.position) < 0) {
                    const world = graphView.toWorld(eventPoint.position)
                    root.controller.selectedNode = root.controller.addNodeAt(world.x, world.y)
                }
            }
        }

        TapHandler {
            acceptedButtons: Qt.RightButton
            onSingleTapped: function(eventPoint, button) {
                const hit = graphView.nodeAt(eventPoint.position)
                if (hit >= 0) {
                    root.controller.selectedNode = hit
                    nodeMenu.popup()
                }
            }
        }

        // --- dragging: a node if one is grabbed, otherwise the camera -------
        DragHandler {
            id: dragHandler
            target: null                 // we move things ourselves, not the item
            acceptedButtons: Qt.LeftButton

            property point grabItemPos
            property point grabCentre

            onActiveChanged: {
                if (active) {
                    grabItemPos = centroid.pressPosition
                    grabCentre = graphView.centre
                    root.draggedNode = graphView.nodeAt(centroid.pressPosition)
                    if (root.draggedNode >= 0)
                        root.controller.selectedNode = root.draggedNode
                } else {
                    root.draggedNode = -1
                }
            }

            onCentroidChanged: {
                if (!active)
                    return
                if (root.draggedNode >= 0) {
                    const world = graphView.toWorld(centroid.position)
                    root.controller.setNodePosition(root.draggedNode, world.x, world.y)
                } else {
                    // Pan: shift the camera centre by the *world-space* delta.
                    const dx = (centroid.position.x - grabItemPos.x) / graphView.zoom
                    const dy = (centroid.position.y - grabItemPos.y) / graphView.zoom
                    graphView.centre = Qt.point(grabCentre.x - dx, grabCentre.y - dy)
                }
            }
        }
    }

    Menu {
        id: nodeMenu
        MenuItem {
            text: root.controller.nodeIsPinned(root.controller.selectedNode)
                  ? qsTr("Unpin node") : qsTr("Pin node in place")
            onTriggered: root.controller.toggleNodePinned(root.controller.selectedNode)
        }
        MenuItem {
            text: qsTr("Delete node")
            onTriggered: root.controller.removeSelectedNode()
        }
        MenuItem {
            text: qsTr("BFS from here")
            onTriggered: root.controller.runBreadthFirst()
        }
        MenuItem {
            text: qsTr("Radial layout from here")
            onTriggered: root.controller.applyRadialLayout()
        }
    }

    // -----------------------------------------------------------------------
    // Node labels, drawn as real Qt Quick text on top of the OpenGL surface.
    //
    // The Repeater consumes the C++ NodeListModel, so `label`, `worldX` and
    // `worldY` below are model *roles*, not properties we had to invent here.
    // The x/y bindings depend on graphView.centre and graphView.zoom, which is
    // why panning and zooming keeps the labels glued to their nodes for free.
    // -----------------------------------------------------------------------
    Repeater {
        id: labelLayer
        model: root.showLabels && root.controller.nodeCount <= 250
               ? root.controller.nodeModel : null

        delegate: Text {
            required property string label
            required property real worldX
            required property real worldY
            required property int nodeIndex

            // A binding tracks *properties*, not function calls.  Writing
            //     x: graphView.toItem(Qt.point(worldX, worldY)).x
            // looks tidier but silently breaks: QML would record a dependency
            // on worldX/worldY only, so the labels would stay frozen wherever
            // the camera happened to be when they were created.  Spelling the
            // transform out means zoom, centre, width and height are all real
            // dependencies, and the labels track the camera for free.
            x: (worldX - graphView.centre.x) * graphView.zoom
               + graphView.width / 2 - width / 2
            y: (worldY - graphView.centre.y) * graphView.zoom
               + graphView.height / 2 - height / 2

            visible: x > -width && y > -height && x < root.width && y < root.height

            text: label
            // An outline keeps the label readable on top of any node colour,
            // in either theme, without needing a background rectangle per node.
            color: "#ffffff"
            style: Text.Outline
            styleColor: "#000000"
            font.pixelSize: Math.max(8, Math.min(15, 11 * graphView.zoom))
            font.bold: true
            renderType: Text.NativeRendering
        }
    }

    // --- hover tooltip ------------------------------------------------------
    Rectangle {
        id: tooltip
        visible: root.hoveredNode >= 0 && root.draggedNode < 0
        color: Theme.surface
        border.color: Theme.border
        radius: Theme.radius
        opacity: 0.96

        width: tooltipText.implicitWidth + 16
        height: tooltipText.implicitHeight + 10
        x: Math.min(root.width - width - 8, hoverHandler.point.position.x + 16)
        y: Math.max(8, hoverHandler.point.position.y - height - 12)

        Text {
            id: tooltipText
            anchors.centerIn: parent
            color: Theme.text
            font.pixelSize: 12
            text: root.hoveredNode < 0 ? "" :
                  qsTr("node %1  ·  \"%2\"  ·  degree %3")
                      .arg(root.hoveredNode)
                      .arg(root.controller.nodeLabel(root.hoveredNode))
                      .arg(root.controller.nodeDegree(root.hoveredNode))
        }
    }

    // --- empty state --------------------------------------------------------
    Text {
        anchors.centerIn: parent
        visible: root.controller.nodeCount === 0
        color: Theme.textMuted
        font.pixelSize: 14
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("No graph loaded.\nPick something from the Generate menu, " +
                   "or double-click here to add a node.")
    }
}
