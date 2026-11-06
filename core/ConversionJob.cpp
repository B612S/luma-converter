#include "ConversionJob.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QUuid>

#include "Logger.hpp"

namespace {

constexpr int kMaxStderrBytes = 128 * 1024;

QString errorTail(const QByteArray &data) {
    QString text = QString::fromLocal8Bit(data).trimmed();
    if (text.isEmpty())
        return QString();
    const QStringList lines = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    // ffmpeg puts the actual reason in the last couple of lines.
    const int first = qMax(0, lines.size() - 3);
    return lines.mid(first).join(QLatin1Char('\n')).trimmed();
}

} // namespace

ConversionJob::ConversionJob(const QString &inputPath, QObject *parent)
    : QObject(parent), m_inputPath(inputPath) {
}

ConversionJob::~ConversionJob() {
    if (m_process) {
        m_process->disconnect(this);
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
            m_process->waitForFinished(1000);
        }
        delete m_process;
    }
    removeTemporaryFile();
}

QString ConversionJob::fileName() const {
    return QFileInfo(m_inputPath).fileName();
}

QString ConversionJob::statusKey() const {
    switch (m_status) {
    case Status::Queued:
        return QStringLiteral("queued");
    case Status::Running:
        return QStringLiteral("running");
    case Status::Done:
        return QStringLiteral("done");
    case Status::Failed:
        return QStringLiteral("failed");
    case Status::Cancelled:
        return QStringLiteral("cancelled");
    }
    return QStringLiteral("queued");
}

QString ConversionJob::labelFor(const ConvertOptions &options) {
    const FormatPreset format = Formats::byId(options.formatId);
    switch (format.quality) {
    case QualityKind::Bitrate: {
        const int bitrate = options.bitrate > 0 ? options.bitrate : format.defaultBitrate;
        return bitrate > 0
            ? QStringLiteral("%1 · %2 kbps").arg(format.label, QString::number(bitrate))
            : format.label;
    }
    case QualityKind::CompressionLevel:
        if (options.compressionLevel >= 0)
            return QStringLiteral("%1 · -%2").arg(format.label, QString::number(options.compressionLevel));
        return format.label;
    case QualityKind::None:
        return format.label;
    }
    return format.label;
}

void ConversionJob::reset() {
    if (m_status == Status::Running)
        return;
    m_status = Status::Queued;
    m_progress = 0.0;
    m_speed = 0.0;
    m_outTimeUs = 0;
    m_errorKey.clear();
    m_errorText.clear();
    m_outputPath.clear();
    m_temporaryPath.clear();
    m_stdout.clear();
    m_stderr.clear();
    m_cancelled = false;
    m_reported = false;
    emit changed();
}

void ConversionJob::setStatus(Status status) {
    if (m_status == status)
        return;
    m_status = status;
    emit changed();
}

QStringList ConversionJob::buildArguments(const FormatPreset &format, const ConvertOptions &options,
                                          const ProbeResult &probe, const QString &input,
                                          const QString &output) {
    QStringList args;
    args << QStringLiteral("-hide_banner")
         << QStringLiteral("-nostdin")
         << QStringLiteral("-loglevel") << QStringLiteral("error")
         << QStringLiteral("-progress") << QStringLiteral("pipe:1")
         << QStringLiteral("-nostats")
         << QStringLiteral("-y")
         << QStringLiteral("-i") << input;

    // Only the first audio stream is converted; chapters do not survive.
    args << QStringLiteral("-map") << QStringLiteral("0:a:0")
         << QStringLiteral("-map_chapters") << QStringLiteral("-1");

    if (options.keepMetadata && format.supportsMetadata)
        args << QStringLiteral("-map_metadata") << QStringLiteral("0");
    else
        args << QStringLiteral("-map_metadata") << QStringLiteral("-1");

    // Cover art lives in a video stream with the attached_pic disposition.
    if (options.keepCover && format.supportsCover && probe.hasCover) {
        args << QStringLiteral("-map") << QStringLiteral("0:v:0")
             << QStringLiteral("-c:v") << QStringLiteral("copy")
             << QStringLiteral("-disposition:v") << QStringLiteral("attached_pic");
    }

    args << QStringLiteral("-c:a") << format.encoder;

    switch (format.quality) {
    case QualityKind::Bitrate: {
        const int bitrate = options.bitrate > 0 ? options.bitrate : format.defaultBitrate;
        if (bitrate > 0)
            args << QStringLiteral("-b:a") << QString::number(bitrate) + QStringLiteral("k");
        break;
    }
    case QualityKind::CompressionLevel:
        if (options.compressionLevel >= 0)
            args << QStringLiteral("-compression_level") << QString::number(options.compressionLevel);
        break;
    case QualityKind::None:
        break;
    }

    if (options.sampleRate > 0)
        args << QStringLiteral("-ar") << QString::number(options.sampleRate);
    if (options.channels > 0)
        args << QStringLiteral("-ac") << QString::number(options.channels);
    if (options.normalize)
        args << QStringLiteral("-af") << QStringLiteral("loudnorm=I=-16:TP=-1.5:LRA=11");

    args << format.extraArgs;
    args << output;
    return args;
}

QString ConversionJob::resolveOutputPath(const QString &directory, const QString &baseName,
                                         const QString &extension, bool overwrite) {
    const QString suffix = QStringLiteral(".") + extension;
    const QString candidate = directory + QLatin1Char('/') + baseName + suffix;
    if (overwrite || !QFileInfo::exists(candidate))
        return candidate;
    for (int index = 2; index < 1000; ++index) {
        const QString numbered = directory + QLatin1Char('/') + baseName
                                 + QStringLiteral(" (") + QString::number(index) + QLatin1Char(')')
                                 + suffix;
        if (!QFileInfo::exists(numbered))
            return numbered;
    }
    return candidate;
}

void ConversionJob::start() {
    if (m_status == Status::Running)
        return;

    m_reported = false;
    m_cancelled = false;
    m_progress = 0.0;
    m_speed = 0.0;
    m_outTimeUs = 0;
    m_errorKey.clear();
    m_errorText.clear();
    m_stdout.clear();
    m_stderr.clear();

    if (!m_probe.ok) {
        scheduleFailure(m_probe.errorKey);
        return;
    }
    if (m_options.ffmpegPath.isEmpty()) {
        scheduleFailure(QStringLiteral("error_ffmpeg_missing"));
        return;
    }

    const FormatPreset format = Formats::byId(m_options.formatId);
    const QString directory = m_options.outputDir.isEmpty()
        ? QFileInfo(m_inputPath).absolutePath()
        : m_options.outputDir;
    if (!QDir().mkpath(directory)) {
        scheduleFailure(QStringLiteral("error_output_dir"));
        return;
    }

    m_outputPath = resolveOutputPath(directory,
                                     QFileInfo(m_inputPath).completeBaseName(),
                                     format.extension, m_options.overwrite);
    m_temporaryPath = directory + QStringLiteral("/.luma-converter-")
                      + QUuid::createUuid().toString(QUuid::WithoutBraces)
                      + QStringLiteral(".") + format.extension;

    // A retried job replaces its process; the previous one is already finished.
    if (m_process) {
        m_process->disconnect(this);
        m_process->deleteLater();
        m_process = nullptr;
    }

    const QStringList args = buildArguments(format, m_options, m_probe, m_inputPath, m_temporaryPath);
    Logger::instance().info(QStringLiteral("ffmpeg %1").arg(args.join(QLatin1Char(' '))));

    m_process = new QProcess(this);
    m_process->setProgram(m_options.ffmpegPath);
    m_process->setArguments(args);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &ConversionJob::readProgress);
    connect(m_process, &QProcess::readyReadStandardError, this, &ConversionJob::readErrors);
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            fail(QStringLiteral("error_ffmpeg_start"),
                 m_process ? m_process->errorString() : QString());
        }
    });
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ConversionJob::onProcessFinished);

    setStatus(Status::Running);
    m_process->start();
}

void ConversionJob::markFailedEarly(const QString &errorKey) {
    if (m_status == Status::Running || m_status == Status::Done)
        return;
    m_errorKey = errorKey;
    setStatus(Status::Failed);
}

void ConversionJob::cancel() {
    if (m_status != Status::Running || !m_process)
        return;
    m_cancelled = true;
    m_process->terminate();
    if (!m_process->waitForFinished(3000)) {
        m_process->kill();
        m_process->waitForFinished(2000);
    }
}

void ConversionJob::readProgress() {
    if (!m_process)
        return;
    m_stdout += m_process->readAllStandardOutput();

    int newline = -1;
    while ((newline = m_stdout.indexOf('\n')) >= 0) {
        const QByteArray line = m_stdout.left(newline).trimmed();
        m_stdout.remove(0, newline + 1);
        if (line.isEmpty())
            continue;

        const int separator = line.indexOf('=');
        if (separator <= 0)
            continue;
        const QByteArray key = line.left(separator);
        const QByteArray value = line.mid(separator + 1);

        if (key == "out_time_us") {
            bool ok = false;
            const qint64 micros = value.toLongLong(&ok);
            if (ok)
                m_outTimeUs = micros;
        } else if (key == "speed") {
            QByteArray number = value;
            if (number.endsWith('x'))
                number.chop(1);
            bool ok = false;
            const double speed = number.toDouble(&ok);
            if (ok && speed > 0)
                m_speed = speed;
        }
    }

    if (m_probe.durationMs > 0) {
        const double fraction = (double(m_outTimeUs) / 1000.0) / m_probe.durationMs;
        m_progress = qBound(0.0, fraction, 0.999);
    }
    emit changed();
}

void ConversionJob::readErrors() {
    if (!m_process)
        return;
    m_stderr += m_process->readAllStandardError();
    if (m_stderr.size() > kMaxStderrBytes)
        m_stderr = m_stderr.right(kMaxStderrBytes);
}

void ConversionJob::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (!m_process)
        return;
    readErrors();
    readProgress();

    if (m_cancelled) {
        removeTemporaryFile();
        finish(Status::Cancelled);
        return;
    }

    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        fail(QStringLiteral("error_ffmpeg_failed"), errorTail(m_stderr));
        return;
    }
    if (m_temporaryPath.isEmpty() || !QFileInfo::exists(m_temporaryPath)) {
        fail(QStringLiteral("error_no_output"), errorTail(m_stderr));
        return;
    }

    // The final name was reserved when the job started; another program may have
    // created it in the meantime.
    if (!m_options.overwrite && QFileInfo::exists(m_outputPath)) {
        m_outputPath = resolveOutputPath(QFileInfo(m_outputPath).absolutePath(),
                                         QFileInfo(m_inputPath).completeBaseName(),
                                         QFileInfo(m_outputPath).suffix(), false);
    }
    if (QFileInfo::exists(m_outputPath) && m_options.overwrite)
        QFile::remove(m_outputPath);

    if (!QFile::rename(m_temporaryPath, m_outputPath)) {
        fail(QStringLiteral("error_rename"), m_outputPath);
        return;
    }

    m_temporaryPath.clear();
    m_progress = 1.0;
    finish(Status::Done);
}

void ConversionJob::finish(Status status) {
    setStatus(status);
    if (m_reported)
        return;
    m_reported = true;
    emit finished(this);
}

void ConversionJob::fail(const QString &errorKey, const QString &errorText) {
    if (m_status == Status::Failed && m_reported)
        return;
    m_errorKey = errorKey;
    m_errorText = errorText.trimmed();
    Logger::instance().warning(QStringLiteral("%1: %2 %3")
                                   .arg(fileName(), errorKey, m_errorText));
    removeTemporaryFile();
    finish(Status::Failed);
}

void ConversionJob::scheduleFailure(const QString &errorKey, const QString &errorText) {
    // Fail synchronously so the queue stops handing this job out, but report the
    // result on the next event loop turn: start() is called from the queue's
    // pump loop, which is not ready for a re-entrant finished() signal.
    m_errorKey = errorKey;
    m_errorText = errorText;
    setStatus(Status::Failed);
    QTimer::singleShot(0, this, [this] {
        if (m_reported)
            return;
        m_reported = true;
        removeTemporaryFile();
        emit finished(this);
    });
}

void ConversionJob::removeTemporaryFile() {
    if (!m_temporaryPath.isEmpty() && QFileInfo::exists(m_temporaryPath)) {
        QFile::remove(m_temporaryPath);
        m_temporaryPath.clear();
    }
}
