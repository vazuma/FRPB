// ---------------------------------------------------------------------------
// The application window.
//
// ApplicationWindow (from QtQuick.Controls) is the piece that gives you the
// three standard slots - `menuBar`, `header`, `footer` - around a content area.
//
// The organising idea here is Action.  An Action bundles a title, a keyboard
// shortcut, an enabled state and a handler into one object; a MenuItem and a
// ToolButton can then both point at the *same* Action, so the menu and the
// toolbar can never drift out of sync.  Define behaviour once, show it twice.
// ---------------------------------------------------------------------------
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import GraphLab

ApplicationWindow {
    id: window

    width: 1280
    height: 820
    visible: true
    title: qsTr("GraphLab — Qt Quick + OpenGL graph explorer")
    color: Theme.background

    // The one instance of our C++ facade.  Declaring it here means QML owns it
    // and its lifetime matches the window's.
    GraphController {
        id: controller
    }

    // -----------------------------------------------------------------------
    // Actions
    // -----------------------------------------------------------------------
    Action {
        id: actionNew
        text: qsTr("&New")
        shortcut: StandardKey.New
        onTriggered: controller.clear()
    }
    Action {
        id: actionOpen
        text: qsTr("&Open…")
        shortcut: StandardKey.Open
        onTriggered: openDialog.open()
    }
    Action {
        id: actionSave
        text: qsTr("&Save As…")
        shortcut: StandardKey.SaveAs
        enabled: controller.nodeCount > 0
        onTriggered: saveDialog.open()
    }
    Action {
        id: actionQuit
        text: qsTr("&Quit")
        shortcut: StandardKey.Quit
        onTriggered: Qt.quit()
    }

    Action {
        id: actionDelete
        text: qsTr("&Delete Selected Node")
        shortcut: StandardKey.Delete
        enabled: controller.selectedNode >= 0
        onTriggered: controller.removeSelectedNode()
    }
    Action {
        id: actionPin
        text: qsTr("&Pin / Unpin Selected")
        shortcut: "P"
        enabled: controller.selectedNode >= 0
        onTriggered: controller.toggleNodePinned(controller.selectedNode)
    }
    Action {
        id: actionDeselect
        text: qsTr("Deselect")
        shortcut: "Esc"
        onTriggered: controller.selectedNode = -1
    }

    Action {
        id: actionRunLayout
        // `checkable` + `checked` bound to a C++ property is the standard way to
        // mirror backend state in a menu.  The two stay in sync in both
        // directions because layoutRunning has a NOTIFY signal.
        text: qsTr("&Run Force Layout")
        shortcut: "Space"
        checkable: true
        checked: controller.layoutRunning
        onTriggered: controller.layoutRunning = checked
    }
    Action {
        id: actionReheat
        text: qsTr("Re&heat Simulation")
        shortcut: "R"
        onTriggered: controller.nudgeLayout()
    }
    Action {
        id: actionFit
        text: qsTr("&Fit to Window")
        shortcut: "F"
        onTriggered: viewport.view.fitToContents()
    }

    // -----------------------------------------------------------------------
    // Menu bar
    // -----------------------------------------------------------------------
    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            MenuItem { action: actionNew }
            MenuItem { action: actionOpen }
            MenuItem { action: actionSave }
            MenuSeparator {}
            MenuItem { action: actionQuit }
        }

        Menu {
            title: qsTr("&Edit")
            MenuItem { action: actionDelete }
            MenuItem { action: actionPin }
            MenuItem { action: actionDeselect }
            MenuSeparator {}
            MenuItem {
                text: qsTr("Clear &Highlights")
                onTriggered: controller.clearHighlights()
            }
        }

        Menu {
            title: qsTr("&Generate")
            MenuItem {
                text: qsTr("Complete Kₙ…")
                onTriggered: paramDialog.ask(qsTr("Complete graph"),
                                             qsTr("Every pair of vertices is joined, so Kₙ has n(n-1)/2 edges."),
                                             [{ label: qsTr("n"), value: 8, from: 1, to: 60 }],
                                             function(v) { controller.generateComplete(v[0]) })
            }
            MenuItem {
                text: qsTr("Cycle Cₙ…")
                onTriggered: paramDialog.ask(qsTr("Cycle"), "",
                                             [{ label: qsTr("n"), value: 12, from: 3, to: 500 }],
                                             function(v) { controller.generateCycle(v[0]) })
            }
            MenuItem {
                text: qsTr("Path Pₙ…")
                onTriggered: paramDialog.ask(qsTr("Path"), "",
                                             [{ label: qsTr("n"), value: 10, from: 1, to: 500 }],
                                             function(v) { controller.generatePath(v[0]) })
            }
            MenuItem {
                text: qsTr("Grid…")
                onTriggered: paramDialog.ask(qsTr("Grid lattice"),
                                             qsTr("Lattices are bipartite — try the 2-colouring afterwards."),
                                             [{ label: qsTr("Columns"), value: 8, from: 1, to: 40 },
                                              { label: qsTr("Rows"),    value: 6, from: 1, to: 40 }],
                                             function(v) { controller.generateGrid(v[0], v[1]) })
            }
            MenuItem {
                text: qsTr("Complete Bipartite Kₘ,ₙ…")
                onTriggered: paramDialog.ask(qsTr("Complete bipartite graph"), "",
                                             [{ label: qsTr("m"), value: 4, from: 1, to: 40 },
                                              { label: qsTr("n"), value: 4, from: 1, to: 40 }],
                                             function(v) { controller.generateCompleteBipartite(v[0], v[1]) })
            }
            MenuItem {
                text: qsTr("Tree…")
                onTriggered: paramDialog.ask(qsTr("Rooted tree"), "",
                                             [{ label: qsTr("Depth"),     value: 4, from: 0, to: 8 },
                                              { label: qsTr("Branching"), value: 2, from: 1, to: 5 }],
                                             function(v) { controller.generateTree(v[0], v[1]) })
            }
            MenuItem {
                text: qsTr("Star K₁,ₙ…")
                onTriggered: paramDialog.ask(qsTr("Star"), "",
                                             [{ label: qsTr("Leaves"), value: 10, from: 1, to: 200 }],
                                             function(v) { controller.generateStar(v[0]) })
            }
            MenuItem {
                text: qsTr("Wheel Wₙ…")
                onTriggered: paramDialog.ask(qsTr("Wheel"), "",
                                             [{ label: qsTr("Rim size"), value: 10, from: 3, to: 200 }],
                                             function(v) { controller.generateWheel(v[0]) })
            }
            MenuItem {
                text: qsTr("Hypercube Q𝒹…")
                onTriggered: paramDialog.ask(qsTr("Hypercube"),
                                             qsTr("Vertices are bit strings; two are adjacent when they differ in one bit."),
                                             [{ label: qsTr("Dimension"), value: 4, from: 1, to: 8 }],
                                             function(v) { controller.generateHypercube(v[0]) })
            }
            MenuSeparator {}
            MenuItem {
                text: qsTr("Petersen Graph")
                onTriggered: controller.generatePetersen()
            }
            MenuItem {
                text: qsTr("Random G(n, p)…")
                onTriggered: paramDialog.ask(qsTr("Erdős–Rényi random graph"),
                                             qsTr("Each of the n(n-1)/2 possible edges is included independently with probability p."),
                                             [{ label: qsTr("Vertices n"),   value: 40, from: 1, to: 400 },
                                              { label: qsTr("Probability p"), value: 8, from: 0, to: 100, suffix: "%" }],
                                             function(v) { controller.generateRandom(v[0], v[1]) })
            }
        }

        Menu {
            title: qsTr("&Layout")
            MenuItem { action: actionRunLayout }
            MenuItem { action: actionReheat }
            MenuSeparator {}
            MenuItem {
                text: qsTr("&Circular")
                onTriggered: controller.applyCircularLayout()
            }
            MenuItem {
                text: qsTr("&Grid")
                onTriggered: controller.applyGridLayout()
            }
            MenuItem {
                text: qsTr("&Radial (from selection)")
                onTriggered: controller.applyRadialLayout()
            }
            MenuItem {
                text: qsTr("Ra&ndom")
                onTriggered: controller.applyRandomLayout()
            }
        }

        Menu {
            title: qsTr("&Algorithms")
            MenuItem {
                text: qsTr("&Breadth-First Search")
                onTriggered: controller.runBreadthFirst()
            }
            MenuItem {
                text: qsTr("&Depth-First Search")
                onTriggered: controller.runDepthFirst()
            }
            MenuItem {
                text: qsTr("&Connected Components")
                onTriggered: controller.runConnectedComponents()
            }
            MenuItem {
                text: qsTr("&Shortest Path…")
                enabled: controller.nodeCount > 1
                onTriggered: paramDialog.ask(qsTr("Dijkstra shortest path"),
                                             qsTr("All edges currently have weight 1, so this is the fewest-hops route."),
                                             [{ label: qsTr("From"), value: Math.max(0, controller.selectedNode),
                                                from: 0, to: controller.nodeCount - 1 },
                                              { label: qsTr("To"),   value: controller.nodeCount - 1,
                                                from: 0, to: controller.nodeCount - 1 }],
                                             function(v) { controller.runShortestPath(v[0], v[1]) })
            }
            MenuItem {
                text: qsTr("&Minimum Spanning Forest")
                onTriggered: controller.runMinimumSpanningTree()
            }
            MenuItem {
                text: qsTr("Bipartite / &2-Colouring")
                onTriggered: controller.runBipartiteCheck()
            }
            MenuItem {
                text: qsTr("Degree C&entrality")
                onTriggered: controller.runDegreeCentrality()
            }
        }

        Menu {
            title: qsTr("&View")
            MenuItem { action: actionFit }
            MenuItem {
                text: qsTr("&Reset Camera")
                onTriggered: viewport.view.resetCamera()
            }
            MenuSeparator {}
            MenuItem {
                text: qsTr("Show &Labels")
                checkable: true
                checked: viewport.showLabels
                onTriggered: viewport.showLabels = checked
            }
            MenuItem {
                text: qsTr("Show &Edges")
                checkable: true
                checked: viewport.showEdges
                onTriggered: viewport.showEdges = checked
            }
            MenuItem {
                text: qsTr("Show &Inspector")
                checkable: true
                checked: inspector.visible
                onTriggered: inspector.visible = checked
            }
            MenuSeparator {}
            MenuItem {
                text: qsTr("&Dark Theme")
                checkable: true
                checked: Theme.dark
                onTriggered: Theme.dark = checked
            }
        }

        Menu {
            title: qsTr("&Help")
            MenuItem {
                text: qsTr("&About GraphLab")
                onTriggered: aboutDialog.open()
            }
        }
    }

    // -----------------------------------------------------------------------
    // Tool bar - the same Actions again, so nothing can get out of step
    // -----------------------------------------------------------------------
    header: ToolBar {
        background: Rectangle {
            color: Theme.surface
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width; height: 1
                color: Theme.border
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            spacing: 4

            ToolButton { action: actionRunLayout; text: controller.layoutRunning ? qsTr("⏸ Pause") : qsTr("▶ Layout") }
            ToolButton { action: actionReheat;    text: qsTr("Reheat") }
            ToolSeparator {}
            ToolButton { text: qsTr("Circular"); onClicked: controller.applyCircularLayout() }
            ToolButton { text: qsTr("Grid");     onClicked: controller.applyGridLayout() }
            ToolSeparator {}
            ToolButton { text: qsTr("BFS");        onClicked: controller.runBreadthFirst() }
            ToolButton { text: qsTr("Components"); onClicked: controller.runConnectedComponents() }
            ToolButton { text: qsTr("MST");        onClicked: controller.runMinimumSpanningTree() }
            ToolButton { text: qsTr("Clear");      onClicked: controller.clearHighlights() }

            Item { Layout.fillWidth: true }

            ToolButton { action: actionFit; text: qsTr("Fit") }
        }
    }

    // -----------------------------------------------------------------------
    // Content
    // -----------------------------------------------------------------------
    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        Viewport {
            id: viewport
            controller: controller
            SplitView.fillWidth: true
            SplitView.minimumWidth: 320
        }

        InspectorPanel {
            id: inspector
            controller: controller
            SplitView.preferredWidth: 290
            SplitView.minimumWidth: 220
        }
    }

    // -----------------------------------------------------------------------
    // Status bar
    // -----------------------------------------------------------------------
    footer: Rectangle {
        height: 26
        color: Theme.surface

        Rectangle {
            anchors.top: parent.top
            width: parent.width; height: 1
            color: Theme.border
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            spacing: 14

            Text {
                Layout.fillWidth: true
                text: controller.statusText
                color: Theme.textMuted
                font.pixelSize: 12
                elide: Text.ElideRight
            }
            Text {
                visible: controller.layoutRunning
                text: qsTr("simulating…")
                color: Theme.accent
                font.pixelSize: 12
            }
            Text {
                text: qsTr("zoom %1×").arg(viewport.view.zoom.toFixed(2))
                color: Theme.textMuted
                font.pixelSize: 12
                font.family: "monospace"
            }
        }
    }

    // -----------------------------------------------------------------------
    // Dialogs
    // -----------------------------------------------------------------------
    ParameterDialog {
        id: paramDialog

        // Small imperative helper so the menu items above stay one-liners.
        property var callback: null

        function ask(dialogTitle, blurb, fieldList, handler) {
            title = dialogTitle
            description = blurb
            fields = fieldList
            callback = handler
            open()
        }

        onApplied: function(values) {
            if (callback)
                callback(values)
        }
    }

    FileDialog {
        id: openDialog
        title: qsTr("Open graph")
        nameFilters: [qsTr("GraphLab JSON (*.json)"), qsTr("All files (*)")]
        onAccepted: controller.loadFromFile(selectedFile)
    }

    FileDialog {
        id: saveDialog
        title: qsTr("Save graph")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: [qsTr("GraphLab JSON (*.json)")]
        onAccepted: controller.saveToFile(selectedFile)
    }

    AboutDialog {
        id: aboutDialog
    }
}
