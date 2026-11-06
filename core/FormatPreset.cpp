#include "FormatPreset.hpp"

#include <QVariantMap>

namespace Formats {

QList<FormatPreset> all() {
    // Order matters: the first entry is the default format in a fresh config.
    return {
        {QStringLiteral("mp3"), QStringLiteral("MP3"), QStringLiteral("mp3"),
         QStringLiteral("libmp3lame"), QualityKind::Bitrate, 192, {128, 192, 256, 320},
         true, true, {QStringLiteral("-id3v2_version"), QStringLiteral("3")}},

        {QStringLiteral("m4a"), QStringLiteral("M4A (AAC)"), QStringLiteral("m4a"),
         QStringLiteral("aac"), QualityKind::Bitrate, 192, {128, 192, 256, 320},
         true, true, {}},

        {QStringLiteral("opus"), QStringLiteral("Opus"), QStringLiteral("opus"),
         QStringLiteral("libopus"), QualityKind::Bitrate, 128, {64, 96, 128, 192, 256},
         false, true, {}},

        {QStringLiteral("ogg"), QStringLiteral("OGG Vorbis"), QStringLiteral("ogg"),
         QStringLiteral("libvorbis"), QualityKind::Bitrate, 192, {128, 192, 256, 320},
         false, true, {}},

        {QStringLiteral("flac"), QStringLiteral("FLAC"), QStringLiteral("flac"),
         QStringLiteral("flac"), QualityKind::CompressionLevel, 0, {},
         true, true, {}},

        {QStringLiteral("alac"), QStringLiteral("ALAC (M4A)"), QStringLiteral("m4a"),
         QStringLiteral("alac"), QualityKind::None, 0, {},
         true, true, {}},

        {QStringLiteral("wav"), QStringLiteral("WAV"), QStringLiteral("wav"),
         QStringLiteral("pcm_s16le"), QualityKind::None, 0, {},
         false, false, {}},
    };
}

FormatPreset byId(const QString &id) {
    const QList<FormatPreset> presets = all();
    for (const FormatPreset &preset : presets) {
        if (preset.id == id)
            return preset;
    }
    return presets.first();
}

bool isValidId(const QString &id) {
    const QList<FormatPreset> presets = all();
    for (const FormatPreset &preset : presets) {
        if (preset.id == id)
            return true;
    }
    return false;
}

QVariantList toVariantList() {
    QVariantList list;
    const QList<FormatPreset> presets = all();
    for (const FormatPreset &preset : presets) {
        QVariantMap map;
        map.insert(QStringLiteral("id"), preset.id);
        map.insert(QStringLiteral("label"), preset.label);
        map.insert(QStringLiteral("extension"), preset.extension);
        map.insert(QStringLiteral("supportsCover"), preset.supportsCover);
        map.insert(QStringLiteral("supportsMetadata"), preset.supportsMetadata);
        map.insert(QStringLiteral("defaultBitrate"), preset.defaultBitrate);
        switch (preset.quality) {
        case QualityKind::Bitrate:
            map.insert(QStringLiteral("quality"), QStringLiteral("bitrate"));
            break;
        case QualityKind::CompressionLevel:
            map.insert(QStringLiteral("quality"), QStringLiteral("compression"));
            break;
        case QualityKind::None:
            map.insert(QStringLiteral("quality"), QStringLiteral("none"));
            break;
        }
        QVariantList bitrates;
        for (int bitrate : preset.bitrates)
            bitrates.append(bitrate);
        map.insert(QStringLiteral("bitrates"), bitrates);
        list.append(map);
    }
    return list;
}

} // namespace Formats
