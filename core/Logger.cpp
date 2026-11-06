#include "Logger.hpp"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace {
constexpr qint64 kMaxLogSize = 1024 * 1024;  // 1 MiB, then rotate to .1
}

Logger &Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::Logger() {
    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation)
                             + QStringLiteral("/luma-converter");
    QDir().mkpath(cacheDir);
    m_path = cacheDir + QStringLiteral("/converter.log");
}

Logger::~Logger() {
    if (m_file.isOpen()) {
        m_stream.flush();
        m_file.close();
    }
}

void Logger::setMode(const QString &mode) {
    const QString value = mode.trimmed().toLower();
    if (value == QLatin1String("off") || value == QLatin1String("none")) {
        m_enabled = false;
        return;
    }
    m_enabled = true;
    if (value == QLatin1String("debug")) {
        m_minLevel = Debug;
    } else if (value == QLatin1String("error") || value == QLatin1String("warn")
               || value == QLatin1String("warning")) {
        m_minLevel = Warning;
    } else {
        m_minLevel = Info;
    }
}

void Logger::openIfNeeded() {
    if (m_file.isOpen())
        return;
    rotateIfNeeded();
    m_file.setFileName(m_path);
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return;
    m_stream.setDevice(&m_file);
}

void Logger::rotateIfNeeded() {
    const QFileInfo info(m_path);
    if (!info.exists() || info.size() < kMaxLogSize)
        return;
    QFile::remove(m_path + QStringLiteral(".1"));
    QFile::rename(m_path, m_path + QStringLiteral(".1"));
}

void Logger::log(Level level, const QString &message) {
    if (!m_enabled || level < m_minLevel)
        return;
    openIfNeeded();
    if (!m_file.isOpen())
        return;

    static const char *const names[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    const QString stamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    m_stream << stamp << ' ' << names[level] << ' ' << message << '\n';
    m_stream.flush();
}
