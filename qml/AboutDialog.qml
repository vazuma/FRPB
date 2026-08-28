import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GraphLab            // for the Theme singleton

Dialog {
    id: root

    title: qsTr("About GraphLab")
    modal: true
    anchors.centerIn: parent
    standardButtons: Dialog.Close
    width: 460
    padding: 16

    background: Rectangle {
        color: Theme.surface
        border.color: Theme.border
        radius: Theme.radius
    }

    header: Label {
        text: root.title
        color: Theme.text
        font.bold: true
        font.pixelSize: 14
        padding: 14
        bottomPadding: 4
    }

    // Replacing the footer keeps the button row from showing the Basic style's
    // light strip.  A DialogButtonBox used as a Dialog's footer is wired to
    // accepted()/rejected() automatically, so the buttons keep working.
    footer: DialogButtonBox {
        standardButtons: root.standardButtons
        background: Rectangle { color: "transparent" }
    }

    ColumnLayout {
        width: parent.width
        spacing: 10

        Text {
            text: qsTr("GraphLab")
            color: Theme.text
            font.pixelSize: 22
            font.bold: true
        }
        Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: Theme.textMuted
            font.pixelSize: 13
            text: qsTr("A small Qt Quick application for exploring graph theory.\n\n" +
                       "The viewport is a QQuickFramebufferObject rendering with raw " +
                       "OpenGL on Qt Quick's render thread; everything around it - the " +
                       "menus, the inspector, the dialogs - is Qt Quick Controls driven " +
                       "by property bindings.")
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

        GridLayout {
            columns: 2
            columnSpacing: 18
            rowSpacing: 4

            Text { text: qsTr("Pan");           color: Theme.textMuted; font.pixelSize: 12 }
            Text { text: qsTr("drag the background"); color: Theme.text; font.pixelSize: 12 }
            Text { text: qsTr("Zoom");          color: Theme.textMuted; font.pixelSize: 12 }
            Text { text: qsTr("mouse wheel");   color: Theme.text; font.pixelSize: 12 }
            Text { text: qsTr("Select node");   color: Theme.textMuted; font.pixelSize: 12 }
            Text { text: qsTr("click");         color: Theme.text; font.pixelSize: 12 }
            Text { text: qsTr("Move node");     color: Theme.textMuted; font.pixelSize: 12 }
            Text { text: qsTr("drag it");       color: Theme.text; font.pixelSize: 12 }
            Text { text: qsTr("Toggle edge");   color: Theme.textMuted; font.pixelSize: 12 }
            Text { text: qsTr("Shift + click another node"); color: Theme.text; font.pixelSize: 12 }
            Text { text: qsTr("Add node");      color: Theme.textMuted; font.pixelSize: 12 }
            Text { text: qsTr("double-click empty space"); color: Theme.text; font.pixelSize: 12 }
            Text { text: qsTr("Pin / unpin");   color: Theme.textMuted; font.pixelSize: 12 }
            Text { text: qsTr("right-click a node"); color: Theme.text; font.pixelSize: 12 }
        }
    }
}
