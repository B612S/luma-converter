#pragma once
#include <QObject>
#include <QSettings>
#include <QString>

// Persistent settings in ~/.config/luma-converter/luma-converter.conf.
// The class is exposed to QML as the `Config` context property.
class ConfigManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString outputFormat READ outputFormat WRITE setOutputFormat NOTIFY outputFormatChanged)
    Q_PROPERTY(int bitrate READ bitrate WRITE setBitrate NOTIFY bitrateChanged)
    Q_PROPERTY(int compressionLevel READ compressionLevel WRITE setCompressionLevel NOTIFY compressionLevelChanged)
    Q_PROPERTY(int sampleRate READ sampleRate WRITE setSampleRate NOTIFY sampleRateChanged)
    Q_PROPERTY(int channels READ channels WRITE setChannels NOTIFY channelsChanged)
    Q_PROPERTY(bool normalize READ normalize WRITE setNormalize NOTIFY normalizeChanged)
    Q_PROPERTY(bool keepMetadata READ keepMetadata WRITE setKeepMetadata NOTIFY keepMetadataChanged)
    Q_PROPERTY(bool keepCover READ keepCover WRITE setKeepCover NOTIFY keepCoverChanged)
    Q_PROPERTY(bool overwrite READ overwrite WRITE setOverwrite NOTIFY overwriteChanged)
    Q_PROPERTY(QString outputDir READ outputDir WRITE setOutputDir NOTIFY outputDirChanged)
    Q_PROPERTY(int parallelJobs READ parallelJobs WRITE setParallelJobs NOTIFY parallelJobsChanged)
    Q_PROPERTY(bool notifications READ notifications WRITE setNotifications NOTIFY notificationsChanged)
    Q_PROPERTY(QString logLevel READ logLevel WRITE setLogLevel NOTIFY logLevelChanged)
    Q_PROPERTY(int windowWidth READ windowWidth WRITE setWindowWidth NOTIFY windowWidthChanged)
    Q_PROPERTY(int windowHeight READ windowHeight WRITE setWindowHeight NOTIFY windowHeightChanged)

public:
    explicit ConfigManager(QObject *parent = nullptr);

    QString language() const;
    void setLanguage(const QString &value);

    QString outputFormat() const;
    void setOutputFormat(const QString &value);

    int bitrate() const;
    void setBitrate(int value);

    int compressionLevel() const;
    void setCompressionLevel(int value);

    int sampleRate() const;
    void setSampleRate(int value);

    int channels() const;
    void setChannels(int value);

    bool normalize() const;
    void setNormalize(bool value);

    bool keepMetadata() const;
    void setKeepMetadata(bool value);

    bool keepCover() const;
    void setKeepCover(bool value);

    bool overwrite() const;
    void setOverwrite(bool value);

    QString outputDir() const;
    void setOutputDir(const QString &value);

    int parallelJobs() const;
    void setParallelJobs(int value);

    bool notifications() const;
    void setNotifications(bool value);

    QString logLevel() const;
    void setLogLevel(const QString &value);

    int windowWidth() const;
    void setWindowWidth(int value);

    int windowHeight() const;
    void setWindowHeight(int value);

    Q_INVOKABLE void saveWindowSize(int width, int height);
    Q_INVOKABLE void resetDefaults();

    QString filePath() const { return m_settings.fileName(); }

signals:
    void languageChanged();
    void outputFormatChanged();
    void bitrateChanged();
    void compressionLevelChanged();
    void sampleRateChanged();
    void channelsChanged();
    void normalizeChanged();
    void keepMetadataChanged();
    void keepCoverChanged();
    void overwriteChanged();
    void outputDirChanged();
    void parallelJobsChanged();
    void notificationsChanged();
    void logLevelChanged();
    void windowWidthChanged();
    void windowHeightChanged();

private:
    QSettings m_settings;
};
