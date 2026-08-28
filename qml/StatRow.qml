// A label/value pair.  Trivial, but factoring it out keeps InspectorPanel.qml
// readable and shows the basic pattern for a reusable QML component: declare
// properties at the top, and the file name becomes the type name.
import QtQuick
import QtQuick.Layouts
import GraphLab            // for the Theme singleton

RowLayout {
    id: root

    property string label: ""
    property string value: ""
    property color valueColour: Theme.text

    Layout.fillWidth: true
    spacing: 6

    Text {
        text: root.label
        color: Theme.textMuted
        font.pixelSize: 12
    }
    Item { Layout.fillWidth: true }          // a spacer: pushes the value right
    Text {
        text: root.value
        color: root.valueColour
        font.pixelSize: 12
        font.family: "monospace"
    }
}
