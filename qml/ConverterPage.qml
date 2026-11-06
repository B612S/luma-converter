import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: page
    title: qsTr("page_title")
    padding: 0

    readonly property var formatList: Queue.formats

    actions: [
        Kirigami.Action {
            text: qsTr("action_add_files")
            icon.name: "list-add"
            shortcut: "Ctrl+O"
            onTriggered: fileDialog.open()
        },
        Kirigami.Action {
            text: qsTr("action_add_folder")
            icon.name: "folder-open"
            onTriggered: folderDialog.open()
        },
        Kirigami.Action {
            text: qsTr("action_clear_finished")
            icon.name: "edit-clear-history"
            enabled: Queue.finishedCount > 0
            onTriggered: Queue.clearFinished()
        },
        Kirigami.Action {
            text: qsTr("action_settings")
            icon.name: "settings-configure"
            onTriggered: settingsDialog.open()
        }
    ]

    function startOptions() {
        return {
            "format": Config.outputFormat,
            "bitrate": Config.bitrate,
            "compressionLevel": Config.compressionLevel,
            "sampleRate": Config.sampleRate,
            "channels": Config.channels,
            "normalize": Config.normalize,
            "keepMetadata": Config.keepMetadata,
            "keepCover": Config.keepCover,
            "overwrite": Config.overwrite,
            "parallelJobs": Config.parallelJobs,
            "outputDir": Config.outputDir
        };
    }

    function summaryText() {
        if (Queue.count === 0)
            return qsTr("summary_empty");
        if (!Queue.ffmpegAvailable)
            return qsTr("summary_no_ffmpeg");
        if (Queue.running)
            return qsTr("summary_running").arg(Queue.finishedCount).arg(Queue.count);
        if (Queue.failedCount > 0)
            return qsTr("summary_failed").arg(Queue.failedCount);
        if (Queue.finishedCount > 0)
            return qsTr("summary_done").arg(Queue.finishedCount);
        return qsTr("summary_ready").arg(Queue.count);
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ListView {
            id: fileList
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: Queue
            clip: true
            reuseItems: true
            delegate: FileDelegate {}
        }

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        // Everything that applies to the current batch sits at the bottom:
        // the output format first (it is what the user comes to change most
        // often), then the queue status and the conversion controls.
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: bottomLayout.implicitHeight + Kirigami.Units.largeSpacing * 2
            color: Kirigami.Theme.alternateBackgroundColor

            ColumnLayout {
                id: bottomLayout
                x: Kirigami.Units.largeSpacing
                y: Kirigami.Units.largeSpacing
                width: parent.width - Kirigami.Units.largeSpacing * 2
                spacing: Kirigami.Units.smallSpacing

                FormatBar {
                    Layout.fillWidth: true
                }

                Kirigami.Separator {
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.largeSpacing

                    Controls.Label {
                        text: page.summaryText()
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Controls.ProgressBar {
                        Layout.preferredWidth: 180
                        from: 0
                        to: 1
                        value: Queue.overallProgress
                        visible: Queue.running
                    }

                    Controls.Button {
                        text: qsTr("convert")
                        icon.name: "media-playback-start"
                        highlighted: true
                        enabled: Queue.count > 0 && !Queue.running && Queue.ffmpegAvailable
                        onClicked: Queue.start(page.startOptions())
                    }

                    Controls.Button {
                        text: Queue.paused ? qsTr("resume") : qsTr("pause")
                        icon.name: Queue.paused ? "media-playback-start" : "media-playback-pause"
                        visible: Queue.running
                        onClicked: Queue.paused ? Queue.resume() : Queue.pause()
                    }

                    Controls.Button {
                        text: qsTr("stop")
                        icon.name: "media-playback-stop"
                        visible: Queue.running
                        onClicked: Queue.stopAll()
                    }
                }
            }
        }
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 10
        visible: fileList.count === 0
        icon.name: "audio-x-generic"
        text: qsTr("empty_title")
        explanation: qsTr("empty_hint")

        helpfulAction: Kirigami.Action {
            text: qsTr("action_add_files")
            icon.name: "list-add"
            onTriggered: fileDialog.open()
        }
    }

    DropArea {
        anchors.fill: parent
        onEntered: (drag) => {
            if (drag.hasUrls)
                drag.acceptProposedAction();
        }
        onDropped: (drop) => {
            if (drop.hasUrls) {
                Queue.addUrls(drop.urls);
                drop.acceptProposedAction();
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: qsTr("dialog_open_files")
        fileMode: FileDialog.OpenFiles
        // The dialogs return URLs; addFiles() expects local paths.
        onAccepted: Queue.addUrls(selectedFiles)
    }

    FolderDialog {
        id: folderDialog
        title: qsTr("dialog_open_folder")
        // A URL as well: addUrls() extracts the local path and scans it.
        onAccepted: Queue.addUrls([selectedFolder])
    }

    SettingsDialog {
        id: settingsDialog
    }

    Connections {
        target: Queue
        function onAllDone(done, failed) {
            if (failed > 0)
                page.showPassiveNotification(qsTr("toast_done_failed").arg(done).arg(failed), "long");
            else
                page.showPassiveNotification(qsTr("toast_done").arg(done));
        }
        function onErrorMessage(key) {
            const text = key === "error_ffmpeg_missing"
                ? qsTr("error_ffmpeg_missing")
                : key;
            page.showPassiveNotification(text, "long");
        }
    }
}
