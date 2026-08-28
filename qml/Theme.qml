// ---------------------------------------------------------------------------
// A QML singleton holding the palette.
//
// `pragma Singleton` plus the QT_QML_SINGLETON_TYPE source-file property in
// CMakeLists.txt means exactly one instance exists, and any file in the module
// can reach it as `Theme.something` with no import statement and no plumbing.
// Flip `dark` and every binding that reads these colours re-evaluates.
// ---------------------------------------------------------------------------
pragma Singleton
import QtQuick

QtObject {
    id: theme

    property bool dark: true

    readonly property color background:  dark ? "#11161d" : "#f4f6f9"
    readonly property color surface:     dark ? "#182029" : "#ffffff"
    readonly property color surfaceAlt:  dark ? "#1f2935" : "#eaeef4"
    readonly property color border:      dark ? "#2b3644" : "#d0d8e2"
    readonly property color text:        dark ? "#e6edf5" : "#1b2430"
    readonly property color textMuted:   dark ? "#8fa2b7" : "#5c6b7d"
    readonly property color accent:      "#4c9aff"
    readonly property color accentWarm:  "#ffd166"
    readonly property color viewport:    dark ? "#0d1218" : "#fbfcfe"

    readonly property int  spacing: 8
    readonly property int  radius: 6
}
