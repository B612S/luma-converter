import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

Controls.ItemDelegate {
    id: delegate
    width: ListView.view ? ListView.view.width : implicitWidth

    readonly property bool isRunning: statusKey === "running"
    readonly property bool isDone: statusKey === "done"
    readonly property bool isFailed: statusKey === "failed"
    readonly property bool isCancelled: statusKey === "cancelled"

    readonly property string statusIcon: {
        switch (statusKey) {
        case "running":
            return "media-playback-start";
        case "done":
            return "dialog-ok-apply";
        case "failed":
            return "dialog-error";
        case "cancelled":
            return "dialog-cancel";
        default:
            return "chronometer";
        }
    }

    readonly property color statusColor: {
        if (isDone)
            return Kirigami.Theme.positiveTextColor;
        if (isFailed)
            return Kirigami.Theme.negativeTextColor;
        if (isRunning)
            return Kirigami.Theme.highlightColor;
        return Kirigami.Theme.disabledTextColor;
    }

    function formatDuration(milliseconds) {
        if (!(milliseconds > 0))
            return "";
        const totalSeconds = Math.round(milliseconds / 1000);
        const minutes = Math.floor(totalSeconds / 60);
        const seconds = totalSeconds % 60;
        const hours = Math.floor(minutes / 60);
        if (hours > 0)
            return hours + ":" + String(minutes % 60).padStart(2, "0") + ":" + String(seconds).padStart(2, "0");
        return minutes + ":" + String(seconds).padStart(2, "0");
    }

    function formatSize(bytes) {
        if (!(bytes > 0))
            return "";
        const units = ["B", "KiB", "MiB", "GiB"];
        let value = bytes;
        let unit = 0;
        while (value >= 1024 && unit < units.length - 1) {
            value /= 1024;
            ++unit;
        }
        return (unit === 0 ? value : value.toFixed(1)) + " " + units[unit];
    }

    function detailText() {
        const parts = [];
        if (codec)
            parts.push(String(codec).toUpperCase());
        if (sampleRate > 0)
            parts.push((sampleRate / 1000).toFixed(1) + " kHz");
        if (channels === 1)
            parts.push(qsTr("channels_mono"));
        else if (channels === 2)
            parts.push(qsTr("channels_stereo"));
        else if (channels > 2)
            parts.push(qsTr("channels_count").arg(channels));
        const durationText = formatDuration(duration);
        if (durationText.length > 0)
            parts.push(durationText);
        const sizeText = formatSize(size);
        if (sizeText.length > 0)
            parts.push(sizeText);
        let text = parts.join(" · ");
        if (formatLabel)
            text += "  →  " + formatLabel;
        return text;
    }

    function errorMessage() {
        switch (errorKey) {
        case "error_no_audio":
            return qsTr("error_no_audio");
        case "error_probe_missing":
            return qsTr("error_probe_missing");
        case "error_probe_failed":
        case "error_probe_timeout":
            return qsTr("error_probe_failed");
        case "error_ffmpeg_missing":
            return qsTr("error_ffmpeg_missing");
        case "error_ffmpeg_start":
            return qsTr("error_ffmpeg_start");
        case "error_output_dir":
            return qsTr("error_output_dir");
        case "error_rename":
            return qsTr("error_rename");
        case "error_no_output":
            return qsTr("error_no_output");
        default:
            return errorText.length > 0 ? errorText : qsTr("error_ffmpeg_failed");
        }
    }

    function statusText() {
        switch (statusKey) {
        case "running":
            return speed > 0
                ? qsTr("status_running_speed").arg(speed.toFixed(1))
                : qsTr("status_running");
        case "done":
            return qsTr("status_done");
        case "failed":
            return qsTr("status_failed");
        case "cancelled":
            return qsTr("status_cancelled");
        default:
            return qsTr("status_queued");
        }
    }

    contentItem: RowLayout {
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Icon {
            Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
            Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
            Layout.alignment: Qt.AlignVCenter
            source: delegate.statusIcon
            color: delegate.statusColor
            isMask: true
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Controls.Label {
                Layout.fillWidth: true
                text: fileName
                elide: Text.ElideMiddle
                font.weight: Font.Medium
            }

            Controls.Label {
                Layout.fillWidth: true
                text: delegate.detailText()
                elide: Text.ElideRight
                color: Kirigami.Theme.disabledTextColor
                font: Kirigami.Theme.smallFont
            }

            Controls.Label {
                Layout.fillWidth: true
                visible: delegate.isFailed
                text: delegate.errorMessage()
                elide: Text.ElideRight
                color: Kirigami.Theme.negativeTextColor
                font: Kirigami.Theme.smallFont
            }

            Controls.ProgressBar {
                Layout.fillWidth: true
                Layout.topMargin: Kirigami.Units.smallSpacing
                from: 0
                to: 1
                value: progress
                visible: delegate.isRunning
            }
        }

        Controls.Label {
            Layout.alignment: Qt.AlignVCenter
            text: delegate.statusText()
            color: delegate.statusColor
            font: Kirigami.Theme.smallFont
        }

        Controls.ToolButton {
            Layout.alignment: Qt.AlignVCenter
            icon.name: "edit-delete"
            enabled: !delegate.isRunning
            onClicked: Queue.removeAt(index)
            Controls.ToolTip.visible: hovered
            Controls.ToolTip.text: qsTr("remove_file")
        }
    }
}
