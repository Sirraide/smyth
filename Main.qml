import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 800
    height: 600
    minimumWidth: 500
    minimumHeight: 200
    visible: true
    color: "#2D2A2E"
    title: "Smyth"

    menuBar: MenuBar {
        Menu {
            title: "&File"

            Action {
                text: "&Quit"
                onTriggered: Qt.quit()
            }

            Action {
                text: "&Save"
                shortcut: StandardKey.Save
                onTriggered: SmythContext.save()
            }
        }
    }

    GridLayout {
        id: mainGrid
        anchors {
            fill: parent
            margins: 20
        }

        rows: 2
        columns: 3

        Editor {
            id: input
            Layout.minimumWidth: 100
            Layout.preferredWidth: parent.width * .25
            Layout.fillHeight: true
        }

        Editor {
            id: changes
            Layout.minimumWidth: 200
            Layout.fillWidth: true
            Layout.fillHeight: true
            fontFamily: "Fira Code"
        }

        Editor {
            id: output
            Layout.minimumWidth: 100
            Layout.preferredWidth: parent.width * .25
            Layout.fillHeight: true
            readOnly: true
        }

        Button {
            text: "Apply"
            onClicked: output.text = SmythContext.applySoundChanges(input.text, changes.text)
        }
    }
}
