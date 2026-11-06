# AGENTS.md

Luma Converter: an audio converter for Linux. C++17 + Qt 6/QML (Kirigami) GUI on
top of the `ffmpeg`/`ffprobe` command line tools. CMake is the only build system.

Unlike [Luma](../luma) this application is **not** Wayland-only: it renders no
video and has no X11-specific code path, so it runs on whatever Qt platform is
available. Do not add a Wayland requirement.

## Build & verify

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)            # single target: --target luma_converter
./build/bin/luma-converter               # note: build/bin/, not ./build/
```

- The binary is `build/bin/luma-converter`; the CMake target is `luma_converter`
  (hyphen-free, `OUTPUT_NAME` adds the hyphen). `VERSION` is the single source of
  the application version and is passed to `main.cpp` as `LUMA_CONVERTER_VERSION`.
- Adding a `.cpp` requires editing the `qt_add_executable(luma_converter ...)`
  source list. Adding a `.qml` file requires adding it to
  `qt_add_qml_module(... QML_FILES ...)`; QML is loaded from
  `qrc:/qt/qml/LumaConverter/qml/Main.qml`. C++ classes are exposed as context
  properties in `main.cpp` (`Config`, `Queue`, `AppVersion`, `QtVersion`), not as
  QML types.
- Kirigami and `qqc2-desktop-style` are QML runtime imports; CMake cannot detect
  them missing. A missing module shows up as a QML import error on launch.
- Tests are opt-in (`-DLUMA_CONVERTER_BUILD_TESTS=ON`, off by default) and live in
  `tests/`: `tst_formats` (argument building), `tst_config` (settings) and
  `tst_conversion` (real ffmpeg run, skipped without ffmpeg). They set
  `XDG_CONFIG_HOME`/`XDG_CACHE_HOME` to a temporary directory.
- `--self-test` loads the QML and exits; `--screenshot FILE` saves a window grab
  and exits. Both are useful for headless checks on a real session.

## Architecture

- `main.cpp` is the composition root: `ConfigManager`, `ConversionQueue`, the
  `QQmlApplicationEngine`, the `Breeze` widget style, translations and the
  positional file arguments.
- `core/ConfigManager` — settings in `~/.config/luma-converter/luma-converter.conf`
  (QSettings INI), exposed with `Q_PROPERTY`s.
- `core/FormatPreset` — the codec table: id, label, extension, encoder, quality
  kind (bitrate / compression level / none), cover and tag support. Add a format
  here only.
- `core/MediaProbe` — one synchronous `ffprobe -of json` run per added file:
  duration, channels, sample rate, codec, size and attached cover art.
- `core/ConversionJob` — one ffmpeg process, progress parsed from
  `-progress pipe:1` (`out_time_us`, `speed`), stderr kept as the error tail.
  Output is written to a hidden temporary file and renamed on success, so a
  cancelled or failed job leaves no half-written track. `buildArguments()` and
  `resolveOutputPath()` are pure and unit-tested.
- `core/ConversionQueue` — `QAbstractListModel` of jobs plus the batch controls
  (start/pause/resume/stop, parallel limit, notifications). The model roles are
  what QML sees; `errorKey` is translated in QML, `errorText` is raw ffmpeg output.
- `qml/` — flat directory: `Main.qml` (window), `ConverterPage.qml` (list,
  actions, bottom batch controls), `FormatBar.qml` (format/quality/rate/channels
  row at the bottom), `FileDelegate.qml`, `SettingsDialog.qml` (Luma-style
  category sidebar: General, Output, Debug, About).

## QML gotchas

- Model roles in a delegate are **context properties**: use `fileName`,
  `progress`, `errorKey`, `index` directly. `delegate.fileName` does not resolve
  and silently yields `undefined` (an empty label).
- `SettingsDialog` copies Luma's layout: a plain `Controls.Dialog` at 800x600
  with a 200px category sidebar, a `Flickable` page and a `DialogButtonBox`
  footer. The main window minimum (860x640) keeps it from being clipped; do not
  lower it without shrinking the dialog. About is the last category, not a
  separate dialog.
- The `Breeze` widget style is created only on non-headless platforms: on
  `offscreen`/`minimal` the plugin waits for a desktop session and hangs the
  process.
- Do not force `QT_QPA_PLATFORM`; the app must stay usable on X11 as well.

## Conventions

- Code comments in English; user-visible strings are `tr()`/`qsTr()`-wrapped
  **translation keys** (`qsTr("page_title")`), with the text in `locales/*.ts`.
  Adding a string means adding a `<message>` to both `en.ts` and `ru.ts`; the
  contexts are the file/class names.
- Keep [`CHANGELOG.md`](CHANGELOG.md) up to date: user-visible changes go under
  `## [Unreleased]` in the matching Keep a Changelog section; when `VERSION`
  changes, move them under the new version with its release date.
- `VERSION` is `MAJOR.MINOR.PATCH`; `pkgrel` in `PKGBUILD` is packaging-only.
  Rules are in [`VERSIONING.md`](VERSIONING.md).
- Packaging is Arch-only: `PKGBUILD` packages the working tree and installs
  `luma-converter.desktop` and the icons itself.
