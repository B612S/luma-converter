#pragma once
#include <QString>

// A single ffprobe run: enough information to build the ffmpeg command line
// (duration for the progress bar, cover art detection) and to show the source
// properties in the UI.
struct ProbeResult {
    bool ok = false;
    bool hasAudio = false;
    bool hasCover = false;
    QString errorKey;        // translation key, e.g. "error_no_audio"
    double durationMs = 0.0;
    int channels = 0;
    int sampleRate = 0;
    QString codec;
    qint64 bitRate = 0;
    qint64 size = 0;
};

// Runs ffprobe synchronously. ffprobe is fast (a few tens of milliseconds), so
// this is called when files are added, not on a worker thread.
ProbeResult probeFile(const QString &path, const QString &ffprobePath);
