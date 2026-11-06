#include "MediaProbe.hpp"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

namespace {

ProbeResult failure(const QString &key) {
    ProbeResult result;
    result.errorKey = key;
    return result;
}

} // namespace

ProbeResult probeFile(const QString &path, const QString &ffprobePath) {
    if (ffprobePath.isEmpty() || !QFileInfo::exists(path))
        return failure(QStringLiteral("error_probe_missing"));

    QProcess process;
    process.start(ffprobePath,
                  {QStringLiteral("-v"), QStringLiteral("error"),
                   QStringLiteral("-show_format"), QStringLiteral("-show_streams"),
                   QStringLiteral("-of"), QStringLiteral("json"), path});
    if (!process.waitForFinished(15000)) {
        process.kill();
        process.waitForFinished(1000);
        return failure(QStringLiteral("error_probe_timeout"));
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        return failure(QStringLiteral("error_probe_failed"));

    const QJsonDocument document = QJsonDocument::fromJson(process.readAllStandardOutput());
    if (!document.isObject())
        return failure(QStringLiteral("error_probe_failed"));

    const QJsonObject root = document.object();
    ProbeResult result;
    result.ok = true;
    result.size = QFileInfo(path).size();

    const QJsonObject format = root.value(QStringLiteral("format")).toObject();
    if (format.contains(QStringLiteral("duration"))) {
        result.durationMs = format.value(QStringLiteral("duration")).toString().toDouble() * 1000.0;
    }
    if (format.contains(QStringLiteral("size"))) {
        bool ok = false;
        const qint64 size = format.value(QStringLiteral("size")).toString().toLongLong(&ok);
        if (ok && size > 0)
            result.size = size;
    }
    if (format.contains(QStringLiteral("bit_rate")))
        result.bitRate = format.value(QStringLiteral("bit_rate")).toString().toLongLong();

    const QJsonArray streams = root.value(QStringLiteral("streams")).toArray();
    for (const QJsonValue &value : streams) {
        const QJsonObject stream = value.toObject();
        const QString type = stream.value(QStringLiteral("codec_type")).toString();
        if (type == QLatin1String("audio") && !result.hasAudio) {
            result.hasAudio = true;
            result.codec = stream.value(QStringLiteral("codec_name")).toString();
            result.channels = stream.value(QStringLiteral("channels")).toInt();
            result.sampleRate = stream.value(QStringLiteral("sample_rate")).toString().toInt();
            if (result.durationMs <= 0 && stream.contains(QStringLiteral("duration")))
                result.durationMs = stream.value(QStringLiteral("duration")).toString().toDouble() * 1000.0;
        } else if (type == QLatin1String("video")) {
            const QJsonObject disposition = stream.value(QStringLiteral("disposition")).toObject();
            if (disposition.value(QStringLiteral("attached_pic")).toInt() == 1)
                result.hasCover = true;
        }
    }

    if (!result.hasAudio) {
        result.ok = false;
        result.errorKey = QStringLiteral("error_no_audio");
    }
    return result;
}
