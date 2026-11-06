#pragma once
#include <QByteArray>
#include <QObject>
#include <QProcess>
#include <QStringList>

#include "FormatPreset.hpp"
#include "MediaProbe.hpp"

// Everything the ffmpeg command line needs, captured when a batch is started.
// The queue fills this in from the UI/config and hands it to every job.
struct ConvertOptions {
    QString formatId = QStringLiteral("mp3");
    int bitrate = 0;              // kbps; 0 = format default
    int compressionLevel = -1;    // flac; -1 = encoder default
    int sampleRate = 0;           // Hz; 0 = keep the source rate
    int channels = 0;             // 0 = keep, 1 = mono, 2 = stereo
    bool normalize = false;       // loudnorm to -16 LUFS
    bool keepMetadata = true;
    bool keepCover = true;
    bool overwrite = false;
    int parallelJobs = 2;
    QString outputDir;            // empty = next to the source file
    QString ffmpegPath;
    QString ffprobePath;
};

// One file to convert: owns the ffmpeg process and turns its progress stream
// into a 0..1 value. The queue keeps the objects and re-emits their state to QML.
class ConversionJob : public QObject {
    Q_OBJECT
public:
    enum class Status { Queued, Running, Done, Failed, Cancelled };

    explicit ConversionJob(const QString &inputPath, QObject *parent = nullptr);
    ~ConversionJob() override;

    QString inputPath() const { return m_inputPath; }
    QString fileName() const;

    void setProbe(const ProbeResult &probe) { m_probe = probe; }
    const ProbeResult &probe() const { return m_probe; }

    void setOptions(const ConvertOptions &options) { m_options = options; }
    const ConvertOptions &options() const { return m_options; }

    Status status() const { return m_status; }
    QString statusKey() const;
    void reset();

    double progress() const { return m_progress; }
    double speed() const { return m_speed; }
    QString errorKey() const { return m_errorKey; }
    QString errorText() const { return m_errorText; }
    QString outputPath() const { return m_outputPath; }
    QString formatLabel() const { return labelFor(m_options); }
    // "MP3 · 192 kbps" / "FLAC · -5" — the same text for queued and running jobs.
    static QString labelFor(const ConvertOptions &options);

    void start();
    void cancel();

    // Marks a file that cannot be converted (e.g. ffprobe found no audio stream)
    // so the list shows the problem instead of a silently queued entry.
    void markFailedEarly(const QString &errorKey);

    // Pure helper, unit-tested without ffmpeg: builds the full argv.
    static QStringList buildArguments(const FormatPreset &format, const ConvertOptions &options,
                                      const ProbeResult &probe, const QString &input,
                                      const QString &output);
    // "song.mp3" -> "song (2).mp3" when the target already exists (and no overwrite).
    static QString resolveOutputPath(const QString &directory, const QString &baseName,
                                     const QString &extension, bool overwrite);

signals:
    void changed();
    void finished(ConversionJob *job);

private:
    void setStatus(Status status);
    void scheduleFailure(const QString &errorKey, const QString &errorText = QString());
    void readProgress();
    void readErrors();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void finish(Status status);
    void fail(const QString &errorKey, const QString &errorText = QString());
    void removeTemporaryFile();

    QString m_inputPath;
    ProbeResult m_probe;
    ConvertOptions m_options;
    Status m_status = Status::Queued;
    QProcess *m_process = nullptr;
    QString m_temporaryPath;
    QString m_outputPath;
    QString m_errorKey;
    QString m_errorText;
    QByteArray m_stdout;
    QByteArray m_stderr;
    double m_progress = 0.0;
    double m_speed = 0.0;
    qint64 m_outTimeUs = 0;
    bool m_cancelled = false;
    bool m_reported = false;
};
