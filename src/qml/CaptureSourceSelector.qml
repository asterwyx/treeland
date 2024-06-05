import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import TreeLand.Protocols.Capture

Item {
    id: selector
    property CaptureSource selectedSource: null
    property int currentSelectType: CaptureSource.Region
    required property CaptureManager manager
    property CaptureContext context: manager.contextInSelection
    property int regionX
    property int regionY
    property int regionEndX
    property int regionEndY
    property bool selecting: false

    component SelectorToolbar: RowLayout {
        Button {
            visible: context.hintType(CaptureSource.Output)
            text: qsTr("Capture monitor")
            onClicked: currentSelectType = CaptureSourceType.Output
        }
        Button {
            visible: context.hintType(CaptureSource.Window)
            text: qsTr("Capture window")
            onClicked: currentSelectType = CaptureSource.Window
        }
        Button {
            visible: context.hintType(CaptureSource.Region)
            text: qsTr("Capture region")
            onClicked: currentSelectType = CaptureSource.Region
        }
        Button {
            text: qsTr("End selection")
            onClicked: {
                if (selectedSource) {
                    context.source = selectedSource
                } else {
                    context.sendSourceFailed(CaptureContext.Other)
                }
            }
        }
    }

    // Region mask
    Rectangle {
        visible: currentSelectType === CaptureSource.Region
        color: Qt.rgba(0, 0, 0, 0.2)
        anchors.fill: parent
    }

    // Region rectangle
    Rectangle {
        visible: currentSelectType === CaptureSource.Region
        color: "transparent"
        border.width: 1
        border.color: "red"
        x: { Math.min(regionX, regionEndX) }
        y: { Math.min(regionY, regionEndY) }
        width: { Math.abs(regionEndX - regionX) }
        height: { Math.abs(regionEndY - regionY) }
    }

    MouseArea {
        visible: currentSelectType === CaptureSource.Region
        anchors.fill: parent

        onPressed: {
            selecting = true
            regionX = mouseX
            regionY = mouseY
            regionEndX = regionX
            regionEndY = regionY
        }

        onReleased: {
            selecting = false
        }

        onPositionChanged: function (mouse) {
            if (selecting) {
                regionEndX = mouse.x
                regionEndY = mouse.y
            }
        }
    }

    SelectorToolbar { }

}
