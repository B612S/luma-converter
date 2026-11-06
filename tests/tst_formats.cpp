#include <QDir>
#include <QTemporaryDir>
#include <QtTest>

#include "ConversionJob.hpp"
#include "FormatPreset.hpp"
#include "MediaProbe.hpp"

// Pure argument-building tests: they never invoke ffmpeg, so they run
// everywhere and are the fastest way to catch a broken command line.
class TestFormats : public QObject {
    Q_OBJECT

private slots:
    void mp3UsesLameAtTheRequestedBitrate();
    void flacUsesCompressionLevel();
    void wavDropsTags();
    void coverIsCopiedForFormatsThatSupportIt();
    void coverIsNotMappedForWav();
    void normalizationIsAdded();
    void existingOutputGetsANumber();
    void overwriteKeepsTheName();
    void unknownFormatFallsBackToMp3();
};

namespace {

ProbeResult audioProbe(bool withCover = false) {
    ProbeResult probe;
    probe.ok = true;
    probe.hasAudio = true;
    probe.hasCover = withCover;
    probe.durationMs = 3000.0;
    probe.channels = 2;
    probe.sampleRate = 48000;
    return probe;
}

bool containsSequence(const QStringList &args, const QStringList &sequence) {
    const int last = args.size() - sequence.size();
    for (int start = 0; start <= last; ++start) {
        bool matches = true;
        for (int offset = 0; offset < sequence.size(); ++offset) {
            if (args.at(start + offset) != sequence.at(offset)) {
                matches = false;
                break;
            }
        }
        if (matches)
            return true;
    }
    return false;
}

} // namespace

void TestFormats::mp3UsesLameAtTheRequestedBitrate() {
    ConvertOptions options;
    options.formatId = QStringLiteral("mp3");
    options.bitrate = 256;

    const QStringList args = ConversionJob::buildArguments(
        Formats::byId(options.formatId), options, audioProbe(),
        QStringLiteral("/music/song.flac"), QStringLiteral("/music/song.mp3"));

    QVERIFY(containsSequence(args, {QStringLiteral("-c:a"), QStringLiteral("libmp3lame")}));
    QVERIFY(containsSequence(args, {QStringLiteral("-b:a"), QStringLiteral("256k")}));
    QCOMPARE(args.last(), QStringLiteral("/music/song.mp3"));
    QVERIFY(args.contains(QStringLiteral("-progress")));
}

void TestFormats::flacUsesCompressionLevel() {
    ConvertOptions options;
    options.formatId = QStringLiteral("flac");
    options.compressionLevel = 8;

    const QStringList args = ConversionJob::buildArguments(
        Formats::byId(options.formatId), options, audioProbe(),
        QStringLiteral("/music/song.wav"), QStringLiteral("/music/song.flac"));

    QVERIFY(containsSequence(args, {QStringLiteral("-compression_level"), QStringLiteral("8")}));
    QVERIFY(!args.contains(QStringLiteral("-b:a")));
}

void TestFormats::wavDropsTags() {
    ConvertOptions options;
    options.formatId = QStringLiteral("wav");
    options.keepMetadata = true;

    const QStringList args = ConversionJob::buildArguments(
        Formats::byId(options.formatId), options, audioProbe(true),
        QStringLiteral("/music/song.flac"), QStringLiteral("/music/song.wav"));

    QVERIFY(containsSequence(args, {QStringLiteral("-map_metadata"), QStringLiteral("-1")}));
}

void TestFormats::coverIsCopiedForFormatsThatSupportIt() {
    ConvertOptions options;
    options.formatId = QStringLiteral("m4a");
    options.keepCover = true;

    const QStringList args = ConversionJob::buildArguments(
        Formats::byId(options.formatId), options, audioProbe(true),
        QStringLiteral("/music/song.flac"), QStringLiteral("/music/song.m4a"));

    QVERIFY(containsSequence(args, {QStringLiteral("-map"), QStringLiteral("0:v:0")}));
    QVERIFY(containsSequence(args, {QStringLiteral("-c:v"), QStringLiteral("copy")}));
    QVERIFY(containsSequence(args, {QStringLiteral("-disposition:v"), QStringLiteral("attached_pic")}));
}

void TestFormats::coverIsNotMappedForWav() {
    ConvertOptions options;
    options.formatId = QStringLiteral("wav");

    const QStringList args = ConversionJob::buildArguments(
        Formats::byId(options.formatId), options, audioProbe(true),
        QStringLiteral("/music/song.flac"), QStringLiteral("/music/song.wav"));

    QVERIFY(!args.contains(QStringLiteral("0:v:0")));
}

void TestFormats::normalizationIsAdded() {
    ConvertOptions options;
    options.formatId = QStringLiteral("opus");
    options.normalize = true;

    const QStringList args = ConversionJob::buildArguments(
        Formats::byId(options.formatId), options, audioProbe(),
        QStringLiteral("/music/song.flac"), QStringLiteral("/music/song.opus"));

    QVERIFY(containsSequence(args, {QStringLiteral("-af"), QStringLiteral("loudnorm=I=-16:TP=-1.5:LRA=11")}));
}

void TestFormats::existingOutputGetsANumber() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile existing(dir.filePath(QStringLiteral("song.mp3")));
    QVERIFY(existing.open(QIODevice::WriteOnly));

    const QString resolved = ConversionJob::resolveOutputPath(
        dir.path(), QStringLiteral("song"), QStringLiteral("mp3"), false);
    QCOMPARE(resolved, dir.filePath(QStringLiteral("song (2).mp3")));
}

void TestFormats::overwriteKeepsTheName() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile existing(dir.filePath(QStringLiteral("song.mp3")));
    QVERIFY(existing.open(QIODevice::WriteOnly));

    const QString resolved = ConversionJob::resolveOutputPath(
        dir.path(), QStringLiteral("song"), QStringLiteral("mp3"), true);
    QCOMPARE(resolved, dir.filePath(QStringLiteral("song.mp3")));
}

void TestFormats::unknownFormatFallsBackToMp3() {
    QVERIFY(!Formats::isValidId(QStringLiteral("nope")));
    QCOMPARE(Formats::byId(QStringLiteral("nope")).id, QStringLiteral("mp3"));
}

QTEST_GUILESS_MAIN(TestFormats)
#include "tst_formats.moc"
