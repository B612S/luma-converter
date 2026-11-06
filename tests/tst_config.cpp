#include <QDir>
#include <QTemporaryDir>
#include <QtTest>

#include "ConfigManager.hpp"

class TestConfig : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void defaults();
    void roundTrip();
    void parallelJobsAreClamped();

private:
    QTemporaryDir m_work;
};

void TestConfig::initTestCase() {
    QVERIFY(m_work.isValid());
    QDir().mkpath(m_work.filePath(QStringLiteral("config")));
    qputenv("XDG_CONFIG_HOME", m_work.filePath(QStringLiteral("config")).toUtf8());
}

void TestConfig::defaults() {
    ConfigManager config;
    QCOMPARE(config.outputFormat(), QStringLiteral("mp3"));
    QCOMPARE(config.parallelJobs(), 2);
    QVERIFY(config.keepMetadata());
    QVERIFY(config.keepCover());
    QVERIFY(!config.overwrite());
    QVERIFY(config.notifications());
    QCOMPARE(config.sampleRate(), 0);
    QCOMPARE(config.channels(), 0);
    QCOMPARE(config.windowWidth(), 920);
}

void TestConfig::roundTrip() {
    {
        ConfigManager config;
        config.setOutputFormat(QStringLiteral("opus"));
        config.setBitrate(96);
        config.setParallelJobs(4);
        config.setOutputDir(QStringLiteral("/music/converted"));
        config.setKeepCover(false);
    }

    ConfigManager config;
    QCOMPARE(config.outputFormat(), QStringLiteral("opus"));
    QCOMPARE(config.bitrate(), 96);
    QCOMPARE(config.parallelJobs(), 4);
    QCOMPARE(config.outputDir(), QStringLiteral("/music/converted"));
    QVERIFY(!config.keepCover());
}

void TestConfig::parallelJobsAreClamped() {
    ConfigManager config;
    config.setParallelJobs(0);
    QCOMPARE(config.parallelJobs(), 1);
    config.setParallelJobs(999);
    QCOMPARE(config.parallelJobs(), 16);
}

QTEST_GUILESS_MAIN(TestConfig)
#include "tst_config.moc"
