// ---------------------------------------------------------------------------
// The right-hand panel: live statistics, the selected node, and a scrollable
// list of every node.
//
// Note how little imperative code there is.  Nothing here is ever "refreshed";
// each Text simply *binds* to a controller property, and the C++ NOTIFY signal
// on that property is what makes the UI update itself.
// ---------------------------------------------------------------------------
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GraphLab

Rectangle {
    id: root

    required property GraphController controller

    color: Theme.surface

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        // --- statistics -----------------------------------------------------
        Text {
            text: qsTr("GRAPH")
            color: Theme.textMuted
            font.pixelSize: 10
            font.letterSpacing: 1.4
            font.bold: true
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 3

            StatRow { label: qsTr("Vertices |V|"); value: root.controller.nodeCount }
            StatRow { label: qsTr("Edges |E|");    value: root.controller.edgeCount }
            StatRow { label: qsTr("Components");   value: root.controller.componentCount }
            StatRow {
                label: qsTr("Density")
                value: root.controller.density.toFixed(3)
            }
            StatRow {
                label: qsTr("Mean degree")
                value: root.controller.averageDegree.toFixed(2)
            }
            StatRow { label: qsTr("Max degree");   value: root.controller.maxDegree }
            StatRow {
                label: qsTr("Acyclic?")
                // A forest has exactly |V| - c edges.  One binding, no code.
                value: root.controller.edgeCount ===
                       root.controller.nodeCount - root.controller.componentCount
                       ? qsTr("tree / forest") : qsTr("has a cycle")
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

        // --- last algorithm -------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            visible: root.controller.algorithmName.length > 0

            Rectangle {
                Layout.preferredWidth: 6
                Layout.preferredHeight: 6
                Layout.alignment: Qt.AlignVCenter
                radius: 3
                color: Theme.accentWarm
            }
            Text {
                Layout.fillWidth: true
                text: root.controller.algorithmName
                color: Theme.accentWarm
                font.pixelSize: 12
                elide: Text.ElideRight
            }
        }

        // --- selection ------------------------------------------------------
        Text {
            text: qsTr("SELECTION")
            color: Theme.textMuted
            font.pixelSize: 10
            font.letterSpacing: 1.4
            font.bold: true
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: selectionColumn.implicitHeight + 16
            color: Theme.surfaceAlt
            radius: Theme.radius
            border.color: Theme.border

            ColumnLayout {
                id: selectionColumn
                anchors.fill: parent
                anchors.margins: 8
                spacing: 4

                Text {
                    visible: root.controller.selectedNode < 0
                    text: qsTr("Nothing selected.\nClick a node in the viewport.")
                    color: Theme.textMuted
                    font.pixelSize: 12
                }
                StatRow {
                    visible: root.controller.selectedNode >= 0
                    label: qsTr("Index"); value: root.controller.selectedNode
                }
                StatRow {
                    visible: root.controller.selectedNode >= 0
                    label: qsTr("Label")
                    value: root.controller.nodeLabel(root.controller.selectedNode)
                }
                StatRow {
                    visible: root.controller.selectedNode >= 0
                    label: qsTr("Degree")
                    value: root.controller.nodeDegree(root.controller.selectedNode)
                }
                Text {
                    Layout.fillWidth: true
                    visible: root.controller.selectedNode >= 0
                    wrapMode: Text.WordWrap
                    color: Theme.textMuted
                    font.pixelSize: 11
                    font.family: "monospace"
                    text: {
                        // Recomputed whenever the selection or the topology
                        // changes, because both are properties this reads.
                        root.controller.edgeCount     // force a dependency
                        const n = root.controller.neighbours(root.controller.selectedNode)
                        return n.length === 0 ? qsTr("isolated vertex")
                                              : qsTr("adjacent to: ") + n.join(", ")
                    }
                }
            }
        }

        // --- node list ------------------------------------------------------
        Text {
            text: qsTr("VERTICES")
            color: Theme.textMuted
            font.pixelSize: 10
            font.letterSpacing: 1.4
            font.bold: true
        }

        ListView {
            id: nodeList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            // The very same C++ model the label Repeater uses in the viewport.
            model: root.controller.nodeModel

            // A delegate lives in its own component scope, so reaching outward
            // for `root` works but cannot be checked by qmllint.  Republishing
            // the controller here lets the delegate get at it through the
            // ListView.view attached property instead - fully resolvable.
            property GraphController controller: root.controller
            currentIndex: root.controller.selectedNode
            ScrollBar.vertical: ScrollBar {}

            delegate: ItemDelegate {
                id: delegateRoot

                required property int index
                required property string label
                required property int degree
                required property color fillColour
                required property bool pinned

                width: ListView.view.width
                height: 26
                readonly property GraphController controller: delegateRoot.ListView.view.controller

                highlighted: index === controller.selectedNode
                onClicked: controller.selectedNode = index

                // Qt Quick Controls' default (Basic) style is light; overriding
                // `background` is how you re-skin a single control without
                // writing a whole custom style.
                background: Rectangle {
                    color: delegateRoot.highlighted ? Theme.accent
                         : delegateRoot.hovered     ? Theme.surfaceAlt
                                                    : "transparent"
                    opacity: delegateRoot.highlighted ? 0.28 : 1.0
                    radius: 3
                }

                contentItem: RowLayout {
                    spacing: 8
                    Rectangle {
                        Layout.preferredWidth: 10
                        Layout.preferredHeight: 10
                        radius: 5
                        color: delegateRoot.fillColour
                        border.color: delegateRoot.pinned ? "#ff8f5e" : "transparent"
                        border.width: 2
                    }
                    Text {
                        text: delegateRoot.label
                        color: Theme.text
                        font.pixelSize: 12
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Text {
                        text: qsTr("deg %1").arg(delegateRoot.degree)
                        color: Theme.textMuted
                        font.pixelSize: 11
                        font.family: "monospace"
                    }
                }
            }
        }
    }
}
