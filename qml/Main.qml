import QtQuick

import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root

    width: Config.windowWidth
    height: Config.windowHeight
    // The settings window is 800x600 (the Luma size), so the main window has
    // to stay large enough to show it without clipping its edges.
    minimumWidth: 860
    minimumHeight: 640
    title: qsTr("app_title")

    pageStack.initialPage: ConverterPage {}

    // Wayland ignores client-side geometry, but the size is worth remembering.
    onClosing: Config.saveWindowSize(root.width, root.height)

    Component.onCompleted: {
        root.width = Math.max(root.width, root.minimumWidth);
        root.height = Math.max(root.height, root.minimumHeight);
    }
}
