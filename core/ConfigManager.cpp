#include "ConfigManager.hpp"

#include <QDir>
#include <QStandardPaths>

namespace {

// Settings keys live in a few flat INI sections. Do not use a "general"
// section here: QSettings reserves [General] for root-level keys, so such a
// key is written as [%General] and then does not round-trip.
constexpr auto kLanguage = "ui/language";
constexpr auto kOutputFormat = "output/format";
constexpr auto kBitrate = "output/bitrate";
constexpr auto kCompressionLevel = "output/compression_level";
constexpr auto kSampleRate = "output/sample_rate";
constexpr auto kChannels = "output/channels";
constexpr auto kNormalize = "output/normalize";
constexpr auto kKeepMetadata = "output/keep_metadata";
constexpr auto kKeepCover = "output/keep_cover";
constexpr auto kOverwrite = "output/overwrite";
constexpr auto kOutputDir = "output/directory";
constexpr auto kParallelJobs = "output/parallel_jobs";
constexpr auto kNotifications = "ui/notifications";
constexpr auto kLogLevel = "ui/log_level";
constexpr auto kWindowWidth = "window/width";
constexpr auto kWindowHeight = "window/height";
constexpr auto kFfmpegPath = "tools/ffmpeg";
constexpr auto kFfprobePath = "tools/ffprobe";

template <typename T>
T readSetting(const QSettings &settings, const char *key, const T &fallback) {
    return settings.value(QString::fromLatin1(key), QVariant::fromValue(fallback)).template value<T>();
}

template <typename T>
void writeSetting(QSettings &settings, const char *key, const T &value) {
    settings.setValue(QString::fromLatin1(key), QVariant::fromValue(value));
}

} // namespace

ConfigManager::ConfigManager(QObject *parent) : QObject(parent),
    m_settings(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                   + QStringLiteral("/luma-converter/luma-converter.conf"),
               QSettings::IniFormat) {
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                  + QStringLiteral("/luma-converter"));
}

QString ConfigManager::language() const {
    return readSetting(m_settings, kLanguage, QStringLiteral("auto"));
}

void ConfigManager::setLanguage(const QString &value) {
    if (language() == value)
        return;
    writeSetting(m_settings, kLanguage, value);
    emit languageChanged();
}

QString ConfigManager::outputFormat() const {
    return readSetting(m_settings, kOutputFormat, QStringLiteral("mp3"));
}

void ConfigManager::setOutputFormat(const QString &value) {
    if (outputFormat() == value)
        return;
    writeSetting(m_settings, kOutputFormat, value);
    emit outputFormatChanged();
}

int ConfigManager::bitrate() const {
    return readSetting(m_settings, kBitrate, 0);
}

void ConfigManager::setBitrate(int value) {
    if (bitrate() == value)
        return;
    writeSetting(m_settings, kBitrate, value);
    emit bitrateChanged();
}

int ConfigManager::compressionLevel() const {
    return readSetting(m_settings, kCompressionLevel, -1);
}

void ConfigManager::setCompressionLevel(int value) {
    if (compressionLevel() == value)
        return;
    writeSetting(m_settings, kCompressionLevel, value);
    emit compressionLevelChanged();
}

int ConfigManager::sampleRate() const {
    return readSetting(m_settings, kSampleRate, 0);
}

void ConfigManager::setSampleRate(int value) {
    if (sampleRate() == value)
        return;
    writeSetting(m_settings, kSampleRate, value);
    emit sampleRateChanged();
}

int ConfigManager::channels() const {
    return readSetting(m_settings, kChannels, 0);
}

void ConfigManager::setChannels(int value) {
    if (channels() == value)
        return;
    writeSetting(m_settings, kChannels, value);
    emit channelsChanged();
}

bool ConfigManager::normalize() const {
    return readSetting(m_settings, kNormalize, false);
}

void ConfigManager::setNormalize(bool value) {
    if (normalize() == value)
        return;
    writeSetting(m_settings, kNormalize, value);
    emit normalizeChanged();
}

bool ConfigManager::keepMetadata() const {
    return readSetting(m_settings, kKeepMetadata, true);
}

void ConfigManager::setKeepMetadata(bool value) {
    if (keepMetadata() == value)
        return;
    writeSetting(m_settings, kKeepMetadata, value);
    emit keepMetadataChanged();
}

bool ConfigManager::keepCover() const {
    return readSetting(m_settings, kKeepCover, true);
}

void ConfigManager::setKeepCover(bool value) {
    if (keepCover() == value)
        return;
    writeSetting(m_settings, kKeepCover, value);
    emit keepCoverChanged();
}

bool ConfigManager::overwrite() const {
    return readSetting(m_settings, kOverwrite, false);
}

void ConfigManager::setOverwrite(bool value) {
    if (overwrite() == value)
        return;
    writeSetting(m_settings, kOverwrite, value);
    emit overwriteChanged();
}

QString ConfigManager::outputDir() const {
    return readSetting(m_settings, kOutputDir, QString());
}

void ConfigManager::setOutputDir(const QString &value) {
    if (outputDir() == value)
        return;
    writeSetting(m_settings, kOutputDir, value);
    emit outputDirChanged();
}

int ConfigManager::parallelJobs() const {
    return readSetting(m_settings, kParallelJobs, 2);
}

void ConfigManager::setParallelJobs(int value) {
    value = qBound(1, value, 16);
    if (parallelJobs() == value)
        return;
    writeSetting(m_settings, kParallelJobs, value);
    emit parallelJobsChanged();
}

bool ConfigManager::notifications() const {
    return readSetting(m_settings, kNotifications, true);
}

void ConfigManager::setNotifications(bool value) {
    if (notifications() == value)
        return;
    writeSetting(m_settings, kNotifications, value);
    emit notificationsChanged();
}

QString ConfigManager::logLevel() const {
    return readSetting(m_settings, kLogLevel, QStringLiteral("info"));
}

void ConfigManager::setLogLevel(const QString &value) {
    if (logLevel() == value)
        return;
    writeSetting(m_settings, kLogLevel, value);
    emit logLevelChanged();
}

int ConfigManager::windowWidth() const {
    return readSetting(m_settings, kWindowWidth, 920);
}

void ConfigManager::setWindowWidth(int value) {
    if (windowWidth() == value)
        return;
    writeSetting(m_settings, kWindowWidth, value);
    emit windowWidthChanged();
}

int ConfigManager::windowHeight() const {
    return readSetting(m_settings, kWindowHeight, 660);
}

void ConfigManager::setWindowHeight(int value) {
    if (windowHeight() == value)
        return;
    writeSetting(m_settings, kWindowHeight, value);
    emit windowHeightChanged();
}

QString ConfigManager::ffmpegPath() const {
    return readSetting(m_settings, kFfmpegPath, QString());
}

void ConfigManager::setFfmpegPath(const QString &value) {
    if (ffmpegPath() == value)
        return;
    writeSetting(m_settings, kFfmpegPath, value);
    emit ffmpegPathChanged();
}

QString ConfigManager::ffprobePath() const {
    return readSetting(m_settings, kFfprobePath, QString());
}

void ConfigManager::setFfprobePath(const QString &value) {
    if (ffprobePath() == value)
        return;
    writeSetting(m_settings, kFfprobePath, value);
    emit ffprobePathChanged();
}

void ConfigManager::saveWindowSize(int width, int height) {
    if (width > 0 && height > 0) {
        setWindowWidth(width);
        setWindowHeight(height);
    }
    m_settings.sync();
}

void ConfigManager::resetDefaults() {
    m_settings.clear();
    m_settings.sync();
    emit languageChanged();
    emit outputFormatChanged();
    emit bitrateChanged();
    emit compressionLevelChanged();
    emit sampleRateChanged();
    emit channelsChanged();
    emit normalizeChanged();
    emit keepMetadataChanged();
    emit keepCoverChanged();
    emit overwriteChanged();
    emit outputDirChanged();
    emit parallelJobsChanged();
    emit notificationsChanged();
    emit logLevelChanged();
    emit windowWidthChanged();
    emit windowHeightChanged();
    emit ffmpegPathChanged();
    emit ffprobePathChanged();
}
