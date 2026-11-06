#include "ConversionQueue.hpp"

#include <QCoreApplication>
#include <QDirIterator>
#include <QFileInfo>
#include <QProcess>
#include <QSet>
#include <QStandardPaths>
#include <QUrl>

#include "ConfigManager.hpp"
#include "Logger.hpp"
#include "Notifier.hpp"

namespace {

// Files picked up when a whole folder is dropped. ffprobe still has the final
// say: anything without an audio stream ends up as a failed job.
const QStringList &inputSuffixes() {
    static const QStringList suffixes = {
        QStringLiteral("mp3"), QStringLiteral("flac"), QStringLiteral("wav"),
        QStringLiteral("ogg"), QStringLiteral("opus"), QStringLiteral("m4a"),
        QStringLiteral("aac"), QStringLiteral("wma"), QStringLiteral("aiff"),
        QStringLiteral("ape"), QStringLiteral("wv"), QStringLiteral("mp4"),
        QStringLiteral("mkv"), QStringLiteral("webm"), QStringLiteral("mov"),
        QStringLiteral("avi"), QStringLiteral("m4v"), QStringLiteral("ts"),
        QStringLiteral("m2ts"), QStringLiteral("3gp"), QStringLiteral("oga"),
    };
    return suffixes;
}

} // namespace

ConversionQueue::ConversionQueue(ConfigManager *config, QObject *parent)
    : QAbstractListModel(parent), m_config(config) {
    refreshTools();
    m_options.formatId = m_config->outputFormat();
    m_options.bitrate = m_config->bitrate();
    m_options.compressionLevel = m_config->compressionLevel();
    m_options.parallelJobs = m_config->parallelJobs();
}

int ConversionQueue::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : int(m_jobs.size());
}

QHash<int, QByteArray> ConversionQueue::roleNames() const {
    return {
        {NameRole, "fileName"},
        {PathRole, "path"},
        {StatusRole, "status"},
        {StatusKeyRole, "statusKey"},
        {ProgressRole, "progress"},
        {DurationRole, "duration"},
        {SizeRole, "size"},
        {CodecRole, "codec"},
        {SampleRateRole, "sampleRate"},
        {ChannelsRole, "channels"},
        {CoverRole, "hasCover"},
        {FormatRole, "formatLabel"},
        {OutputRole, "outputPath"},
        {ErrorRole, "errorText"},
        {ErrorKeyRole, "errorKey"},
        {SpeedRole, "speed"},
    };
}

int ConversionQueue::finishedCount() const {
    int done = 0;
    for (const ConversionJob *job : m_jobs) {
        if (job->status() == ConversionJob::Status::Done)
            ++done;
    }
    return done;
}

int ConversionQueue::failedCount() const {
    int failed = 0;
    for (const ConversionJob *job : m_jobs) {
        if (job->status() == ConversionJob::Status::Failed)
            ++failed;
    }
    return failed;
}

double ConversionQueue::overallProgress() const {
    if (m_jobs.isEmpty())
        return 0.0;
    double total = 0.0;
    for (const ConversionJob *job : m_jobs)
        total += job->progress();
    return total / double(m_jobs.size());
}

QVariantList ConversionQueue::formats() const {
    return Formats::toVariantList();
}

int ConversionQueue::rowOf(const ConversionJob *job) const {
    for (int row = 0; row < m_jobs.size(); ++row) {
        if (m_jobs.at(row) == job)
            return row;
    }
    return -1;
}

QVariant ConversionQueue::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_jobs.size())
        return QVariant();
    const ConversionJob *job = m_jobs.at(index.row());
    const ProbeResult &probe = job->probe();

    switch (role) {
    case NameRole:
        return job->fileName();
    case PathRole:
        return job->inputPath();
    case StatusRole:
        return int(job->status());
    case StatusKeyRole:
        return job->statusKey();
    case ProgressRole:
        return job->progress();
    case DurationRole:
        return probe.durationMs;
    case SizeRole:
        return probe.size;
    case CodecRole:
        return probe.codec;
    case SampleRateRole:
        return probe.sampleRate;
    case ChannelsRole:
        return probe.channels;
    case CoverRole:
        return probe.hasCover;
    case FormatRole:
        return job->status() == ConversionJob::Status::Queued
            ? ConversionJob::labelFor(m_options)
            : job->formatLabel();
    case OutputRole:
        return job->outputPath();
    case ErrorRole:
        return job->errorText();
    case ErrorKeyRole:
        return job->errorKey();
    case SpeedRole:
        return job->speed();
    default:
        return QVariant();
    }
}

void ConversionQueue::refreshTools() {
    const QString ffmpeg = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
    const QString ffprobe = QStandardPaths::findExecutable(QStringLiteral("ffprobe"));
    const bool changed = ffmpeg != m_ffmpegPath || ffprobe != m_ffprobePath;
    m_ffmpegPath = ffmpeg;
    m_ffprobePath = ffprobe;

    if (changed || m_ffmpegVersion.isEmpty()) {
        m_ffmpegVersion.clear();
        if (!m_ffmpegPath.isEmpty()) {
            QProcess process;
            process.start(m_ffmpegPath, {QStringLiteral("-version")});
            if (process.waitForFinished(3000)) {
                const QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
                m_ffmpegVersion = output.split(QLatin1Char('\n')).value(0).trimmed();
            }
        }
        emit toolsChanged();
    }
}

void ConversionQueue::addFiles(const QStringList &paths) {
    QSet<QString> known;
    known.reserve(m_jobs.size());
    for (const ConversionJob *job : m_jobs)
        known.insert(job->inputPath());

    QList<ConversionJob *> created;
    for (const QString &raw : paths) {
        const QFileInfo info(raw);
        if (!info.exists() || info.isDir())
            continue;
        const QString path = info.absoluteFilePath();
        if (known.contains(path))
            continue;

        auto *job = new ConversionJob(path, this);
        job->setProbe(probeFile(path, m_ffprobePath));
        if (!job->probe().ok)
            job->markFailedEarly(job->probe().errorKey);
        connect(job, &ConversionJob::changed, this, [this, job] { onJobChanged(job); });
        connect(job, &ConversionJob::finished, this, &ConversionQueue::onJobFinished);
        created.append(job);
        known.insert(path);
    }

    if (created.isEmpty())
        return;

    beginInsertRows(QModelIndex(), int(m_jobs.size()), int(m_jobs.size() + created.size() - 1));
    for (ConversionJob *job : created)
        m_jobs.append(job);
    endInsertRows();

    Logger::instance().info(QStringLiteral("added %1 file(s)").arg(created.size()));
    emit countChanged();
    emit countersChanged();
    emit progressChanged();
}

void ConversionQueue::addUrls(const QVariantList &urls) {
    QStringList files;
    QStringList folders;
    for (const QVariant &value : urls) {
        const QUrl url = value.toUrl();
        if (!url.isLocalFile())
            continue;
        const QString path = url.toLocalFile();
        if (QFileInfo(path).isDir())
            folders.append(path);
        else
            files.append(path);
    }
    for (const QString &folder : folders)
        addFolder(folder);
    addFiles(files);
}

void ConversionQueue::addFolder(const QString &path) {
    QStringList filters;
    for (const QString &suffix : inputSuffixes())
        filters.append(QStringLiteral("*.") + suffix);

    QStringList files;
    QDirIterator iterator(path, filters, QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext())
        files.append(iterator.next());

    if (!files.isEmpty())
        addFiles(files);
}

void ConversionQueue::removeAt(int row) {
    if (row < 0 || row >= m_jobs.size())
        return;
    ConversionJob *job = m_jobs.at(row);
    if (job->status() == ConversionJob::Status::Running)
        job->cancel();

    beginRemoveRows(QModelIndex(), row, row);
    m_jobs.removeAt(row);
    job->deleteLater();
    endRemoveRows();

    emit countChanged();
    emit countersChanged();
    emit progressChanged();
}

void ConversionQueue::clear() {
    stopAll();
    beginResetModel();
    qDeleteAll(m_jobs);
    m_jobs.clear();
    endResetModel();
    emit countChanged();
    emit countersChanged();
    emit progressChanged();
}

void ConversionQueue::clearFinished() {
    for (int row = int(m_jobs.size()) - 1; row >= 0; --row) {
        const ConversionJob::Status status = m_jobs.at(row)->status();
        if (status == ConversionJob::Status::Done || status == ConversionJob::Status::Cancelled) {
            beginRemoveRows(QModelIndex(), row, row);
            ConversionJob *job = m_jobs.takeAt(row);
            job->deleteLater();
            endRemoveRows();
        }
    }
    emit countChanged();
    emit countersChanged();
    emit progressChanged();
}

ConvertOptions ConversionQueue::optionsFromMap(const QVariantMap &map) const {
    ConvertOptions options;
    options.formatId = map.value(QStringLiteral("format"), m_config->outputFormat()).toString();
    if (!Formats::isValidId(options.formatId))
        options.formatId = QStringLiteral("mp3");
    options.bitrate = map.value(QStringLiteral("bitrate"), m_config->bitrate()).toInt();
    options.compressionLevel = map.value(QStringLiteral("compressionLevel"),
                                         m_config->compressionLevel()).toInt();
    options.sampleRate = map.value(QStringLiteral("sampleRate"), m_config->sampleRate()).toInt();
    options.channels = map.value(QStringLiteral("channels"), m_config->channels()).toInt();
    options.normalize = map.value(QStringLiteral("normalize"), m_config->normalize()).toBool();
    options.keepMetadata = map.value(QStringLiteral("keepMetadata"), m_config->keepMetadata()).toBool();
    options.keepCover = map.value(QStringLiteral("keepCover"), m_config->keepCover()).toBool();
    options.overwrite = map.value(QStringLiteral("overwrite"), m_config->overwrite()).toBool();
    options.parallelJobs = qBound(1, map.value(QStringLiteral("parallelJobs"),
                                               m_config->parallelJobs()).toInt(), 16);
    options.outputDir = map.value(QStringLiteral("outputDir"), m_config->outputDir()).toString();
    options.ffmpegPath = m_ffmpegPath;
    options.ffprobePath = m_ffprobePath;
    return options;
}

void ConversionQueue::start(const QVariantMap &options) {
    if (m_running) {
        resume();
        return;
    }
    refreshTools();
    m_options = optionsFromMap(options);

    if (m_ffmpegPath.isEmpty()) {
        emit errorMessage(QStringLiteral("error_ffmpeg_missing"));
        return;
    }

    int pending = 0;
    for (ConversionJob *job : m_jobs) {
        if (job->status() == ConversionJob::Status::Done)
            continue;
        job->reset();
        ++pending;
    }
    if (pending == 0)
        return;

    m_running = true;
    m_paused = false;
    emit runningChanged();
    emit pausedChanged();
    pump();
}

void ConversionQueue::pause() {
    if (!m_running || m_paused)
        return;
    m_paused = true;
    emit pausedChanged();
}

void ConversionQueue::resume() {
    if (!m_running || !m_paused)
        return;
    m_paused = false;
    emit pausedChanged();
    pump();
}

void ConversionQueue::stopAll() {
    if (!m_running && m_active == 0)
        return;
    m_running = false;
    m_paused = false;
    emit runningChanged();
    emit pausedChanged();

    const QList<ConversionJob *> jobs = m_jobs;
    for (ConversionJob *job : jobs) {
        if (job->status() == ConversionJob::Status::Running)
            job->cancel();
    }
}

ConversionJob *ConversionQueue::nextQueued() const {
    for (ConversionJob *job : m_jobs) {
        if (job->status() == ConversionJob::Status::Queued)
            return job;
    }
    return nullptr;
}

void ConversionQueue::pump() {
    const int limit = qMax(1, m_options.parallelJobs);
    while (m_running && !m_paused && m_active < limit) {
        ConversionJob *job = nextQueued();
        if (!job)
            break;
        ++m_active;
        job->setOptions(m_options);
        job->start();
        emit countersChanged();
    }
    emit progressChanged();
}

void ConversionQueue::onJobChanged(ConversionJob *job) {
    const int row = rowOf(job);
    if (row >= 0) {
        const QModelIndex index = this->index(row, 0);
        emit dataChanged(index, index);
    }
    emit progressChanged();
}

void ConversionQueue::onJobFinished(ConversionJob *job) {
    if (m_active > 0)
        --m_active;
    const int row = rowOf(job);
    if (row >= 0) {
        const QModelIndex index = this->index(row, 0);
        emit dataChanged(index, index);
    }
    emit countersChanged();
    emit progressChanged();

    if (!m_running)
        return;

    pump();

    if (m_active == 0 && !nextQueued()) {
        m_running = false;
        m_paused = false;
        emit runningChanged();
        emit pausedChanged();

        const int done = finishedCount();
        const int failed = failedCount();
        emit allDone(done, failed);

        if (m_config->notifications()) {
            if (failed > 0) {
                Notifier::send(tr("notify_failed_title"),
                               tr("notify_failed_body").arg(failed).arg(done));
            } else {
                Notifier::send(tr("notify_done_title"),
                               tr("notify_done_body").arg(done));
            }
        }
    }
}
