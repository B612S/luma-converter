#pragma once
#include <QFile>
#include <QString>
#include <QTextStream>

// Minimal file logger: one file under ~/.cache/luma-converter/ with a size cap.
// Modes follow the same names Luma uses: "off", "error", "info", "debug".
class Logger {
public:
    enum Level { Debug = 0, Info = 1, Warning = 2, Critical = 3 };

    static Logger &instance();

    void setMode(const QString &mode);
    void log(Level level, const QString &message);

    void debug(const QString &message) { log(Debug, message); }
    void info(const QString &message) { log(Info, message); }
    void warning(const QString &message) { log(Warning, message); }
    void critical(const QString &message) { log(Critical, message); }

    QString filePath() const { return m_path; }

private:
    Logger();
    ~Logger();
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    void openIfNeeded();
    void rotateIfNeeded();

    QString m_path;
    QFile m_file;
    QTextStream m_stream;
    int m_minLevel = Info;
    bool m_enabled = true;
};
