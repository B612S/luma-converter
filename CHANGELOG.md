# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed

- The output format, quality, sample rate and channel layout are now selected in
  a bar at the bottom of the window instead of at the top.
- Settings use the Luma sidebar layout (General, Output, Debug, About) with the
  same 800x600 size, row spacing, lighter category panel and About page; the
  output folder, loudness normalization, tags, cover art, overwrite and parallel
  jobs moved there from the main window.
- The KDE QML controls style is selected explicitly, so the settings keep the
  Breeze/Kirigami look when the app is started outside the Plasma launcher.
- About is now a page of the settings window, opened from the same Settings
  action as everything else.
- The application icon uses a flat background instead of a gradient and a
  smaller conversion ring.
- The minimum window size grew to 860x640 so the 800x600 settings window is
  never clipped.

### Removed

- The separate About window and its toolbar action.

### Fixed

- Language, notification and log level choices are restored after a restart;
  they were written to a QSettings group that could not be read back.

## [1.0.0] - 2026-11-06

### Added

- Kirigami/Qt Quick interface with a batch queue, drag and drop, recursive folder
  adding and command line file arguments.
- Formats: MP3, M4A (AAC), Opus, OGG Vorbis, FLAC, ALAC and WAV, with bitrate or
  compression level, sample rate and channel selection.
- Copying of tags and cover art where the container supports it, automatic
  disabling for WAV.
- Optional EBU R128 loudness normalization.
- Output to the source folder or a chosen directory, `(2)`-style collision
  suffixes, explicit overwrite mode and temporary files written before the final
  rename.
- Parallel conversions, per-file progress and speed, pause/resume, stop and
  translated per-file error messages.
- Desktop notifications when a batch finishes.
- English and Russian translations, remembered settings and window size.
- Autotests for argument building, configuration and a real ffmpeg conversion.
- `PKGBUILD` for Arch Linux.
