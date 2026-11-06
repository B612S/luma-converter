#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

#include "ConfigManager.hpp"
#include "ConversionQueue.hpp"

// End-to-end test of the conversion pipeline: real ffmpeg, real files.
// Skipped when ffmpeg is not installed.
class TestConversion : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void convertsWavToMp3();
    void reportsFilesWithoutAudio();

private:
    QString createTone(const QString &path, const QString &duration = QStringLiteral("2"));

    QString m_ffmpeg;
    QString m_ffprobe;
    QTemporaryDir m_work;
};

void TestConversion::initTestCase() {
    m_ffmpeg = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
    m_ffprobe = QStandardPaths::findExecutable(QStringLiteral("ffprobe"));
    if (m_ffmpeg.isEmpty() || m_ffprobe.isEmpty())
        QSKIP("ffmpeg/ffprobe are not installed");

    QVERIFY(m_work.isValid());
    // Keep the test away from the real configuration and cache.
    QDir().mkpath(m_work.filePath(QStringLiteral("config")));
    QDir().mkpath(m_work.filePath(QStringLiteral("cache")));
    qputenv("XDG_CONFIG_HOME", m_work.filePath(QStringLiteral("config")).toUtf8());
    qputenv("XDG_CACHE_HOME", m_work.filePath(QStringLiteral("cache")).toUtf8());
}

QString TestConversion::createTone(const QString &path, const QString &duration) {
    QProcess process;
    process.start(m_ffmpeg,
                  {QStringLiteral("-hide_banner"), QStringLiteral("-loglevel"), QStringLiteral("error"),
                   QStringLiteral("-y"), QStringLiteral("-f"), QStringLiteral("lavfi"),
                   QStringLiteral("-i"), QStringLiteral("sine=frequency=440:duration=") + duration,
                   QStringLiteral("-ac"), QStringLiteral("2"),
                   QStringLiteral("-ar"), QStringLiteral("48000"), path});
    if (!process.waitForFinished(30000) || process.exitCode() != 0)
        return QString();
    return path;
}

void TestConversion::convertsWavToMp3() {
    const QString source = m_work.filePath(QStringLiteral("tone.wav"));
    QVERIFY(!createTone(source).isEmpty());

    const QString outputDir = m_work.filePath(QStringLiteral("out"));
    QVERIFY(QDir().mkpath(outputDir));

    ConfigManager config;
    ConversionQueue queue(&config);
    QSignalSpy finished(&queue, &ConversionQueue::allDone);

    queue.addFiles({source});
    QCOMPARE(queue.count(), 1);

    QVariantMap options;
    options.insert(QStringLiteral("format"), QStringLiteral("mp3"));
    options.insert(QStringLiteral("bitrate"), 128);
    options.insert(QStringLiteral("outputDir"), outputDir);
    options.insert(QStringLiteral("overwrite"), true);
    queue.start(options);

    QVERIFY(finished.wait(120000));
    QCOMPARE(queue.failedCount(), 0);
    QCOMPARE(queue.finishedCount(), 1);

    const QString output = outputDir + QStringLiteral("/tone.mp3");
    QVERIFY(QFileInfo::exists(output));
    QVERIFY(QFileInfo(output).size() > 1000);
}

void TestConversion::reportsFilesWithoutAudio() {
    const QString source = m_work.filePath(QStringLiteral("notes.txt"));
    QFile file(source);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("this is not audio\n");
    file.close();

    const QString outputDir = m_work.filePath(QStringLiteral("out-bogus"));
    QVERIFY(QDir().mkpath(outputDir));

    ConfigManager config;
    ConversionQueue queue(&config);
    QSignalSpy finished(&queue, &ConversionQueue::allDone);

    queue.addFiles({source});
    QCOMPARE(queue.count(), 1);

    QVariantMap options;
    options.insert(QStringLiteral("format"), QStringLiteral("mp3"));
    options.insert(QStringLiteral("outputDir"), outputDir);
    queue.start(options);

    QVERIFY(finished.wait(30000));
    QCOMPARE(queue.failedCount(), 1);
    QCOMPARE(queue.finishedCount(), 0);
}

QTEST_GUILESS_MAIN(TestConversion)
#include "tst_conversion.moc"
