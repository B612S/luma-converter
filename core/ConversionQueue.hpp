#pragma once
#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include "ConversionJob.hpp"

class ConfigManager;

// The queue that QML sees: a list model of files plus the batch controls.
// Conversion runs one ffmpeg process per job, at most `parallelJobs` at a time.
class ConversionQueue : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int activeCount READ activeCount NOTIFY countersChanged)
    Q_PROPERTY(int finishedCount READ finishedCount NOTIFY countersChanged)
    Q_PROPERTY(int failedCount READ failedCount NOTIFY countersChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY pausedChanged)
    Q_PROPERTY(double overallProgress READ overallProgress NOTIFY progressChanged)
    Q_PROPERTY(QVariantList formats READ formats CONSTANT)
    Q_PROPERTY(bool ffmpegAvailable READ ffmpegAvailable NOTIFY toolsChanged)
    Q_PROPERTY(QString ffmpegVersion READ ffmpegVersion NOTIFY toolsChanged)
    Q_PROPERTY(QString ffmpegPath READ ffmpegPath NOTIFY toolsChanged)
    Q_PROPERTY(QString ffprobePath READ ffprobePath NOTIFY toolsChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        PathRole,
        StatusRole,
        StatusKeyRole,
        ProgressRole,
        DurationRole,
        SizeRole,
        CodecRole,
        SampleRateRole,
        ChannelsRole,
        CoverRole,
        FormatRole,
        OutputRole,
        ErrorRole,
        ErrorKeyRole,
        SpeedRole,
    };

    explicit ConversionQueue(ConfigManager *config, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return m_jobs.size(); }
    int activeCount() const { return m_active; }
    int finishedCount() const;
    int failedCount() const;
    bool running() const { return m_running; }
    bool paused() const { return m_paused; }
    double overallProgress() const;
    QVariantList formats() const;
    bool ffmpegAvailable() const { return !m_ffmpegPath.isEmpty(); }
    QString ffmpegVersion() const { return m_ffmpegVersion; }
    QString ffmpegPath() const { return m_ffmpegPath; }
    QString ffprobePath() const { return m_ffprobePath; }

    Q_INVOKABLE void addFiles(const QStringList &paths);
    Q_INVOKABLE void addUrls(const QVariantList &urls);
    Q_INVOKABLE void addFolder(const QString &path);
    Q_INVOKABLE void removeAt(int row);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void clearFinished();
    Q_INVOKABLE void start(const QVariantMap &options);
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    Q_INVOKABLE void stopAll();
    Q_INVOKABLE void refreshTools();

signals:
    void countChanged();
    void countersChanged();
    void runningChanged();
    void pausedChanged();
    void progressChanged();
    void toolsChanged();
    void allDone(int done, int failed);
    void errorMessage(const QString &key);

private:
    void onJobChanged(ConversionJob *job);
    void onJobFinished(ConversionJob *job);
    void pump();
    ConversionJob *nextQueued() const;
    void resolveToolPaths();
    ConvertOptions optionsFromMap(const QVariantMap &map) const;
    int rowOf(const ConversionJob *job) const;

    ConfigManager *m_config = nullptr;
    QList<ConversionJob *> m_jobs;
    ConvertOptions m_options;
    int m_active = 0;
    bool m_running = false;
    bool m_paused = false;
    QString m_ffmpegPath;
    QString m_ffprobePath;
    QString m_ffmpegVersion;
};
