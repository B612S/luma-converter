import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

// The per-batch output choices shown at the bottom of the window: container
// and codec, quality, sample rate and channel layout. Everything that applies
// to the files themselves (folder, tags, overwrite, parallel jobs) lives in
// the settings window.
RowLayout {
    id: root
    spacing: Kirigami.Units.smallSpacing

    readonly property var formatList: Queue.formats
    readonly property int formatIndex: {
        for (let i = 0; i < formatList.length; ++i) {
            if (formatList[i].id === Config.outputFormat)
                return i;
        }
        return 0;
    }
    readonly property var currentFormat: formatList[Math.max(0, Math.min(formatIndex, formatList.length - 1))]
    readonly property bool isBitrate: currentFormat && currentFormat.quality === "bitrate"
    readonly property bool isCompression: currentFormat && currentFormat.quality === "compression"

    readonly property var bitrateLabels: {
        const labels = [];
        if (!currentFormat)
            return labels;
        for (let i = 0; i < currentFormat.bitrates.length; ++i)
            labels.push(qsTr("bitrate_value").arg(currentFormat.bitrates[i]));
        return labels;
    }
    readonly property int bitrateIndex: {
        if (!currentFormat)
            return 0;
        const wanted = Config.bitrate > 0 ? Config.bitrate : currentFormat.defaultBitrate;
        const found = currentFormat.bitrates.indexOf(wanted);
        return found >= 0 ? found : 0;
    }

    readonly property var sampleRateModel: [
        { value: 0, label: qsTr("auto") },
        { value: 44100, label: "44.1 kHz" },
        { value: 48000, label: "48 kHz" },
        { value: 96000, label: "96 kHz" }
    ]
    readonly property var channelModel: [
        { value: 0, label: qsTr("auto") },
        { value: 1, label: qsTr("channels_mono") },
        { value: 2, label: qsTr("channels_stereo") }
    ]

    function indexOfValue(model, value) {
        for (let i = 0; i < model.length; ++i) {
            if (model[i].value === value)
                return i;
        }
        return 0;
    }

    Controls.Label {
        text: qsTr("label_format")
    }
    Controls.ComboBox {
        id: formatBox
        Layout.preferredWidth: Kirigami.Units.gridUnit * 8
        model: root.formatList
        textRole: "label"
        currentIndex: root.formatIndex
        onActivated: {
            const format = root.formatList[currentIndex];
            Config.outputFormat = format.id;
            Config.bitrate = format.defaultBitrate;
            Config.compressionLevel = -1;
        }
    }

    Controls.Label {
        text: qsTr("label_quality")
        visible: root.isBitrate
    }
    Controls.ComboBox {
        id: bitrateBox
        Layout.preferredWidth: Kirigami.Units.gridUnit * 6
        visible: root.isBitrate
        model: root.bitrateLabels
        currentIndex: root.bitrateIndex
        onActivated: Config.bitrate = root.currentFormat.bitrates[currentIndex]
    }

    Controls.Label {
        text: qsTr("label_compression")
        visible: root.isCompression
    }
    Controls.SpinBox {
        visible: root.isCompression
        from: 0
        to: 12
        value: Config.compressionLevel >= 0 ? Config.compressionLevel : 5
        onValueModified: Config.compressionLevel = value
    }

    Controls.Label {
        text: qsTr("label_samplerate")
    }
    Controls.ComboBox {
        id: sampleRateBox
        Layout.preferredWidth: Kirigami.Units.gridUnit * 5
        model: root.sampleRateModel
        textRole: "label"
        currentIndex: root.indexOfValue(root.sampleRateModel, Config.sampleRate)
        onActivated: Config.sampleRate = root.sampleRateModel[currentIndex].value
    }

    Controls.Label {
        text: qsTr("label_channels")
    }
    Controls.ComboBox {
        id: channelsBox
        Layout.preferredWidth: Kirigami.Units.gridUnit * 5
        model: root.channelModel
        textRole: "label"
        currentIndex: root.indexOfValue(root.channelModel, Config.channels)
        onActivated: Config.channels = root.channelModel[currentIndex].value
    }

    Item {
        Layout.fillWidth: true
    }

    Connections {
        target: Config
        function onOutputFormatChanged() {
            formatBox.currentIndex = root.formatIndex;
        }
        function onBitrateChanged() {
            bitrateBox.currentIndex = root.bitrateIndex;
        }
        function onSampleRateChanged() {
            sampleRateBox.currentIndex = root.indexOfValue(root.sampleRateModel, Config.sampleRate);
        }
        function onChannelsChanged() {
            channelsBox.currentIndex = root.indexOfValue(root.channelModel, Config.channels);
        }
    }
}
