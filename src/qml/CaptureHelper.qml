import QtQuick

Item {
    anchors.fill: parent
    required property CaptureManager manager
    Repeater {
        model: manager.contextModel
        CaptureSourceSelector {
            anchors.fill: parent
            onSelectedSourceChanged: {
                // Source selected.
                contextModel.context.source = selectedSource
            }
        }
    }
}
