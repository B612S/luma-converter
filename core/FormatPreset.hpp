#pragma once
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariantList>

// How the quality of a format is expressed on the command line.
enum class QualityKind {
    Bitrate,          // -b:a <n>k
    CompressionLevel, // -compression_level <n>
    None,             // lossless / uncompressed: nothing to tune
};

struct FormatPreset {
    QString id;                  // "mp3" — stored in the config
    QString label;               // "MP3" — shown in the UI (brand names, not translated)
    QString extension;           // output file extension, without the dot
    QString encoder;             // ffmpeg audio encoder, e.g. "libmp3lame"
    QualityKind quality = QualityKind::None;
    int defaultBitrate = 0;      // kbps, used when the user did not pick one
    QList<int> bitrates;         // kbps choices offered in the UI
    bool supportsCover = false;  // embedded cover art survives the conversion
    bool supportsMetadata = false;
    QStringList extraArgs;       // format-specific ffmpeg switches
};

namespace Formats {

QList<FormatPreset> all();
FormatPreset byId(const QString &id);
bool isValidId(const QString &id);

// Serialised for QML: a list of maps with id/label/extension/quality/bitrates…
QVariantList toVariantList();

} // namespace Formats
