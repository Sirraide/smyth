import QtQuick

Rectangle {
    id: root
    color: "#1E1E1E"

    property alias text: edit.text
    property alias readOnly: edit.readOnly
    property alias fontFamily: edit.font.family

    TextEdit {
        id: edit
        color: "#FCFCFC"
        width: parent.width
        wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere

        anchors {
            fill: parent
            margins: 5
        }

        font {
            family: "Charis SIL"
            features: { "liga": 1 }
            pointSize: 16
        }
    }
}
