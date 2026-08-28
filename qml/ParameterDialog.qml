// ---------------------------------------------------------------------------
// A reusable dialog that builds its own form from a list of field descriptions.
//
// Two QML ideas carry the whole file:
//   * a Repeater instantiates one row per entry in `fields`, so the dialog does
//     not need to know in advance what it is asking for;
//   * `accepted` carries the collected values back to the caller, which keeps
//     the dialog free of any knowledge about the controller.
// ---------------------------------------------------------------------------
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GraphLab            // for the Theme singleton

Dialog {
    id: root

    // [{ label: "Nodes", value: 12, from: 1, to: 500, suffix: "" }, ...]
    property var fields: []
    property string description: ""

    // Emitted with the numbers the user chose, in the same order as `fields`.
    signal applied(var values)

    modal: true
    anchors.centerIn: parent
    standardButtons: Dialog.Ok | Dialog.Cancel
    width: 380
    padding: 16

    // Qt Quick Controls' default "Basic" style is light.  Every control exposes
    // its chrome as a replaceable item - `background`, `header`, `footer` - so
    // re-skinning one is a matter of assigning a new Item, with no need to write
    // a whole custom style.
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

    onAccepted: {
        let values = []
        for (let i = 0; i < repeater.count; ++i)
            values.push(repeater.itemAt(i).spinValue)
        root.applied(values)
    }

    ColumnLayout {
        width: parent.width
        spacing: 10

        Text {
            Layout.fillWidth: true
            visible: root.description.length > 0
            text: root.description
            color: Theme.textMuted
            font.pixelSize: 12
            wrapMode: Text.WordWrap
        }

        Repeater {
            id: repeater
            model: root.fields

            // The delegate is a RowLayout that publishes its current value, so
            // onAccepted above can read it back through repeater.itemAt(i).
            delegate: RowLayout {
                id: field
                required property var modelData
                property alias spinValue: spin.value

                Layout.fillWidth: true
                spacing: 10

                Label {
                    text: field.modelData.label
                    color: Theme.text
                    Layout.preferredWidth: 130
                }
                SpinBox {
                    id: spin
                    Layout.fillWidth: true
                    from: field.modelData.from !== undefined ? field.modelData.from : 0
                    to: field.modelData.to !== undefined ? field.modelData.to : 999
                    value: field.modelData.value !== undefined ? field.modelData.value : from
                    editable: true
                    textFromValue: function(value) {
                        return value + (field.modelData.suffix !== undefined ? field.modelData.suffix : "")
                    }
                    // The matching parser, otherwise typing into the editable
                    // field would fail as soon as a suffix is shown.
                    valueFromText: function(text) {
                        return parseInt(text.replace(/[^0-9-]/g, ""), 10) || 0
                    }
                }
            }
        }
    }
}
