import QtQuick

Rectangle {
    id: root
    color: "#1E1E1E"

    property alias text: edit.text
    property alias readOnly: edit.readOnly

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
            pointSize: 16
        }
    }
}
