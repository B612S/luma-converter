import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

// Settings window in the style of Luma: a category sidebar on the left, a
// scrollable page on the right and About as the last category. The same
// spacing, heading and control sizes are used there.
Controls.Dialog {
    id: settingsDialog
    title: qsTr("settings_title")
    modal: true
    focus: true
    width: 800
    height: 600
    padding: 0

    anchors.centerIn: Controls.Overlay.overlay
    closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutside

    // Luma's dialog is translucent over the dark video window, which makes the
    // sidebar read as a lighter panel. The queue behind this dialog is light, so
    // use the two theme surfaces directly instead: the alternate background for
    // the page, the normal one for the sidebar. This keeps the same contrast
    // without letting the queue show through the dialog.
    background: Rectangle {
        color: Kirigami.Theme.alternateBackgroundColor
        radius: 8
        border.color: "transparent"
    }

    readonly property int controlWidth: 200
    property string currentCategory: "general"

    readonly property var formatList: Queue.formats
    readonly property var currentFormat: {
        for (let i = 0; i < formatList.length; ++i) {
            if (formatList[i].id === Config.outputFormat)
                return formatList[i];
        }
        return null;
    }

    readonly property var languageModel: [
        { value: "auto", label: qsTr("language_auto") },
        { value: "en", label: "English" },
        { value: "ru", label: "Русский" }
    ]
    readonly property var logLevelModel: [
        { value: "off", label: qsTr("log_off") },
        { value: "error", label: qsTr("log_error") },
        { value: "info", label: qsTr("log_info") },
        { value: "debug", label: qsTr("log_debug") }
    ]

    function indexOfValue(model, value) {
        for (let i = 0; i < model.length; ++i) {
            if (model[i].value === value)
                return i;
        }
        return 0;
    }

    onOpened: {
        // The custom content item is not focusable by default; without active
        // focus Escape would not reach the popup.
        settingsContent.forceActiveFocus();
        Queue.refreshTools();
    }

    contentItem: RowLayout {
        id: settingsContent
        spacing: 0
        focus: true
        // Escape must close the dialog even when a child control has focus.
        Keys.onEscapePressed: (event) => {
            settingsDialog.close();
            event.accepted = true;
        }

        Rectangle {
            Layout.preferredWidth: 200
            Layout.fillHeight: true
            color: Kirigami.Theme.backgroundColor
            radius: 8

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 4

                Item { Layout.preferredHeight: 8 }

                component CategoryButton: Rectangle {
                    id: btn
                    property string text: ""
                    property bool isActive: false
                    signal clicked()

                    Layout.fillWidth: true
                    height: 36
                    color: isActive ? Qt.rgba(Kirigami.Theme.highlightColor.r, Kirigami.Theme.highlightColor.g, Kirigami.Theme.highlightColor.b, 0.15)
                    : (catHover.containsMouse ? Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.08) : "transparent")
                    radius: 4

                    Controls.Label {
                        text: btn.text
                        color: Kirigami.Theme.textColor
                        font.pixelSize: 14
                        font.weight: btn.isActive ? Font.Bold : Font.Normal
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 12
                    }

                    MouseArea {
                        id: catHover
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: btn.clicked()
                    }
                }

                CategoryButton {
                    text: qsTr("settings_general")
                    isActive: settingsDialog.currentCategory === "general"
                    onClicked: settingsDialog.currentCategory = "general"
                }
                CategoryButton {
                    text: qsTr("settings_output")
                    isActive: settingsDialog.currentCategory === "output"
                    onClicked: settingsDialog.currentCategory = "output"
                }
                CategoryButton {
                    text: qsTr("settings_debug")
                    isActive: settingsDialog.currentCategory === "debug"
                    onClicked: settingsDialog.currentCategory = "debug"
                }
                CategoryButton {
                    text: qsTr("settings_about")
                    isActive: settingsDialog.currentCategory === "about"
                    onClicked: settingsDialog.currentCategory = "about"
                }

                Item { Layout.fillHeight: true }
            }
        }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 24
            clip: true
            contentWidth: width
            contentHeight: contentLayout.implicitHeight
            boundsBehavior: Flickable.StopAtBounds

            // Set the bar width explicitly: some styles reserve a wide
            // transparent area that would overlap the controls on the right.
            Controls.ScrollBar.vertical: Controls.ScrollBar {
                policy: Controls.ScrollBar.AsNeeded
                width: 10
            }

            Item {
                // Leave room on the right for the scrollbar.
                width: parent.width - 22
                implicitHeight: contentLayout.implicitHeight

                ColumnLayout {
                    id: contentLayout
                    width: parent.width
                    spacing: 24

                    // ---- General ----
                    ColumnLayout {
                        visible: settingsDialog.currentCategory === "general"
                        Layout.fillWidth: true
                        spacing: 22

                        Controls.Label {
                            text: qsTr("settings_general")
                            font.pixelSize: 20
                            font.weight: Font.Bold
                            color: Kirigami.Theme.textColor
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.2)
                        }

                        GridLayout {
                            columns: 2
                            columnSpacing: 16
                            rowSpacing: 16
                            Layout.fillWidth: true

                            Controls.Label {
                                text: qsTr("settings_ffmpeg")
                                color: Kirigami.Theme.textColor
                                font.pixelSize: 14
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                            }
                            Controls.TextField {
                                Layout.fillWidth: true
                                font.pixelSize: 14
                                placeholderText: qsTr("settings_auto")
                                text: Config.ffmpegPath
                                onEditingFinished: {
                                    Config.ffmpegPath = text;
                                    Queue.refreshTools();
                                }
                            }

                            Controls.Label {
                                text: qsTr("settings_ffprobe")
                                color: Kirigami.Theme.textColor
                                font.pixelSize: 14
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                            }
                            Controls.TextField {
                                Layout.fillWidth: true
                                font.pixelSize: 14
                                placeholderText: qsTr("settings_auto")
                                text: Config.ffprobePath
                                onEditingFinished: {
                                    Config.ffprobePath = text;
                                    Queue.refreshTools();
                                }
                            }

                            Controls.Label {
                                text: qsTr("settings_status")
                                color: Kirigami.Theme.textColor
                                font.pixelSize: 14
                                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                            }
                            Controls.Label {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                font.pixelSize: 14
                                color: Queue.ffmpegAvailable ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.negativeTextColor
                                text: Queue.ffmpegAvailable
                                    ? (Queue.ffmpegVersion.length > 0 ? Queue.ffmpegVersion : qsTr("settings_ffmpeg_found"))
                                    : qsTr("settings_ffmpeg_missing")
                            }

                            Controls.Label {
                                text: qsTr("settings_language")
                                color: Kirigami.Theme.textColor
                                font.pixelSize: 14
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                            }
                            Item {
                                Layout.preferredWidth: settingsDialog.controlWidth
                                Layout.preferredHeight: 40
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                Controls.ComboBox {
                                    anchors.fill: parent
                                    font.pixelSize: 14
                                    model: settingsDialog.languageModel
                                    textRole: "label"
                                    currentIndex: settingsDialog.indexOfValue(settingsDialog.languageModel, Config.language)
                                    onActivated: Config.language = settingsDialog.languageModel[currentIndex].value
                                }
                            }

                            Controls.Label {
                                Layout.columnSpan: 2
                                Layout.fillWidth: true
                                text: qsTr("settings_language_restart")
                                color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.65)
                                font.pixelSize: 12
                                wrapMode: Text.WordWrap
                            }

                            Controls.Label {
                                text: qsTr("settings_notifications")
                                color: Kirigami.Theme.textColor
                                font.pixelSize: 14
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                            }
                            Item {
                                Layout.preferredWidth: settingsDialog.controlWidth
                                Layout.preferredHeight: 40
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                Controls.Switch {
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    checked: Config.notifications
                                    onToggled: Config.notifications = checked
                                }
                            }
                        }
                    }

                    // ---- Output ----
                    ColumnLayout {
                        visible: settingsDialog.currentCategory === "output"
                        Layout.fillWidth: true
                        spacing: 22

                        Controls.Label {
                            text: qsTr("settings_output")
                            font.pixelSize: 20
                            font.weight: Font.Bold
                            color: Kirigami.Theme.textColor
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.2)
                        }

                        GridLayout {
                            columns: 2
                            columnSpacing: 16
                            rowSpacing: 16
                            Layout.fillWidth: true

                            Controls.Label {
                                text: qsTr("label_output")
                                color: Kirigami.Theme.textColor
                                font.pixelSize: 14
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Controls.TextField {
                                    Layout.fillWidth: true
                                    font.pixelSize: 14
                                    readOnly: true
                                    text: Config.outputDir.length > 0 ? Config.outputDir : qsTr("same_folder")
                                }
                                Controls.Button {
                                    text: qsTr("browse")
                                    icon.name: "folder-open"
                                    onClicked: outputDialog.open()
                                }
                                Controls.ToolButton {
                                    icon.name: "edit-undo"
                                    visible: Config.outputDir.length > 0
                                    onClicked: Config.outputDir = ""
                                    Controls.ToolTip.visible: hovered
                                    Controls.ToolTip.text: qsTr("reset_output_folder")
                                }
                            }

                            Controls.Label {
                                text: qsTr("normalize")
                                color: Kirigami.Theme.textColor
                                font.pixelSize: 14
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                            }
                            Item {
                                Layout.preferredWidth: settingsDialog.controlWidth
                                Layout.preferredHeight: 40
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                Controls.Switch {
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    checked: Config.normalize
                                    onToggled: Config.normalize = checked
                                }
                            }

                            Controls.Label {
                                text: qsTr("keep_metadata")
                                color: Kirigami.Theme.textColor
                                font.pixelSize: 14
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                            }
                            Item {
                                Layout.preferredWidth: settingsDialog.controlWidth
                                Layout.preferredHeight: 40
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                Controls.Switch {
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    checked: Config.keepMetadata
                                    onToggled: Config.keepMetadata = checked
                                }
                            }

                            Controls.Label {
                                text: qsTr("keep_cover")
                                color: Kirigami.Theme.textColor
                                font.pixelSize: 14
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                            }
                            Item {
                                Layout.preferredWidth: settingsDialog.controlWidth
                                Layout.preferredHeight: 40
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                Controls.Switch {
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    checked: Config.keepCover
                                    enabled: settingsDialog.currentFormat !== null && settingsDialog.currentFormat.supportsCover
                                    onToggled: Config.keepCover = checked
                                }
                            }

                            Controls.Label {
                                text: qsTr("overwrite")
                                color: Kirigami.Theme.textColor
                                font.pixelSize: 14
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                            }
                            Item {
                                Layout.preferredWidth: settingsDialog.controlWidth
                                Layout.preferredHeight: 40
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                Controls.Switch {
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    checked: Config.overwrite
                                    onToggled: Config.overwrite = checked
                                }
                            }

                            Controls.Label {
                                Layout.columnSpan: 2
                                Layout.fillWidth: true
                                text: qsTr("settings_overwrite_hint")
                                color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.65)
                                font.pixelSize: 12
                                wrapMode: Text.WordWrap
                            }

                            Controls.Label {
                                text: qsTr("parallel_jobs")
                                color: Kirigami.Theme.textColor
                                font.pixelSize: 14
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                            }
                            Item {
                                Layout.preferredWidth: settingsDialog.controlWidth
                                Layout.preferredHeight: 40
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                Controls.SpinBox {
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    font.pixelSize: 14
                                    from: 1
                                    to: 16
                                    value: Config.parallelJobs
                                    onValueModified: Config.parallelJobs = value
                                }
                            }
                        }
                    }

                    // ---- Debug ----
                    ColumnLayout {
                        visible: settingsDialog.currentCategory === "debug"
                        Layout.fillWidth: true
                        spacing: 22

                        Controls.Label {
                            text: qsTr("settings_debug")
                            font.pixelSize: 20
                            font.weight: Font.Bold
                            color: Kirigami.Theme.textColor
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.2)
                        }

                        GridLayout {
                            columns: 2
                            columnSpacing: 16
                            rowSpacing: 16
                            Layout.fillWidth: true

                            Controls.Label {
                                text: qsTr("settings_log_level")
                                color: Kirigami.Theme.textColor
                                font.pixelSize: 14
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                            }
                            Item {
                                Layout.preferredWidth: settingsDialog.controlWidth
                                Layout.preferredHeight: 40
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                Controls.ComboBox {
                                    anchors.fill: parent
                                    font.pixelSize: 14
                                    model: settingsDialog.logLevelModel
                                    textRole: "label"
                                    currentIndex: settingsDialog.indexOfValue(settingsDialog.logLevelModel, Config.logLevel)
                                    onActivated: {
                                        Config.logLevel = settingsDialog.logLevelModel[currentIndex].value;
                                        Queue.refreshTools();
                                    }
                                }
                            }

                            Controls.Label {
                                Layout.columnSpan: 2
                                Layout.fillWidth: true
                                text: qsTr("settings_log_hint")
                                color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.65)
                                font.pixelSize: 12
                                wrapMode: Text.WordWrap
                            }
                        }
                    }

                    // ---- About ----
                    ColumnLayout {
                        visible: settingsDialog.currentCategory === "about"
                        Layout.fillWidth: true
                        spacing: 22

                        Controls.Label {
                            text: qsTr("settings_about")
                            font.pixelSize: 20
                            font.weight: Font.Bold
                            color: Kirigami.Theme.textColor
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.2)
                        }

                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 16
                            Layout.topMargin: 16

                            Image {
                                source: "qrc:/assets/luma-converter.png"
                                sourceSize.width: 64
                                sourceSize.height: 64
                                Layout.preferredWidth: 64
                                Layout.preferredHeight: 64
                                fillMode: Image.PreserveAspectFit
                            }

                            ColumnLayout {
                                spacing: 2

                                Controls.Label {
                                    text: qsTr("app_title")
                                    font.pixelSize: 28
                                    font.weight: Font.Bold
                                    color: Kirigami.Theme.textColor
                                }
                                Controls.Label {
                                    text: qsTr("about_version").arg(AppVersion)
                                    font.pixelSize: 14
                                    color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.7)
                                }
                            }
                        }

                        Controls.Label {
                            Layout.fillWidth: true
                            text: qsTr("about_description")
                            color: Kirigami.Theme.textColor
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Layout.topMargin: 16

                            Controls.Label {
                                text: qsTr("about_links")
                                font.weight: Font.Bold
                                color: Kirigami.Theme.textColor
                            }

                            Controls.Label {
                                text: '<a href="https://github.com/B612S/luma-converter">' + qsTr("about_github") + '</a>'
                                textFormat: Text.RichText
                                color: Kirigami.Theme.highlightColor
                                onLinkActivated: Qt.openUrlExternally(link)
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    acceptedButtons: Qt.NoButton
                                }
                            }

                            Controls.Label {
                                text: '<a href="https://ffmpeg.org/">' + qsTr("about_ffmpeg_link") + '</a>'
                                textFormat: Text.RichText
                                color: Kirigami.Theme.highlightColor
                                onLinkActivated: Qt.openUrlExternally(link)
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    acceptedButtons: Qt.NoButton
                                }
                            }

                            Controls.Label {
                                text: '<a href="https://invent.kde.org/frameworks/kirigami">' + qsTr("about_kirigami_link") + '</a>'
                                textFormat: Text.RichText
                                color: Kirigami.Theme.highlightColor
                                onLinkActivated: Qt.openUrlExternally(link)
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    acceptedButtons: Qt.NoButton
                                }
                            }

                            Controls.Label {
                                text: '<a href="https://www.qt.io/">' + qsTr("about_qt_link") + '</a>'
                                textFormat: Text.RichText
                                color: Kirigami.Theme.highlightColor
                                onLinkActivated: Qt.openUrlExternally(link)
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    acceptedButtons: Qt.NoButton
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: 16
                            spacing: 4

                            Controls.Label {
                                Layout.fillWidth: true
                                text: qsTr("about_ffmpeg_version").arg(Queue.ffmpegVersion.length > 0
                                                                      ? Queue.ffmpegVersion
                                                                      : qsTr("about_unknown"))
                                color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.55)
                                font.pixelSize: 12
                                wrapMode: Text.WordWrap
                            }
                            Controls.Label {
                                Layout.fillWidth: true
                                text: qsTr("about_qt_version").arg(QtVersion)
                                color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.55)
                                font.pixelSize: 12
                            }
                            Controls.Label {
                                Layout.fillWidth: true
                                text: qsTr("about_license")
                                color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.55)
                                font.pixelSize: 12
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }
            }
        }
    }

    footer: Controls.DialogButtonBox {
        alignment: Qt.AlignRight

        Controls.Button {
            text: qsTr("settings_close")
            Controls.DialogButtonBox.buttonRole: Controls.DialogButtonBox.AcceptRole
            onClicked: settingsDialog.close()
        }
        Item { implicitWidth: 16; implicitHeight: 1 }
    }

    FolderDialog {
        id: outputDialog
        title: qsTr("dialog_output_folder")
        onAccepted: Config.outputDir = decodeURIComponent(selectedFolder.toString().replace("file://", ""))
    }
}
