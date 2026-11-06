# Versioning

`VERSION` holds `MAJOR.MINOR.PATCH` and is the single source of the application
version. CMake reads it, passes it to `main.cpp` as `LUMA_CONVERTER_VERSION`, and
uses it for the project and QML module version. `luma-converter.desktop`'s
`Version` key is the desktop-entry specification version, not the app version.

## Bump rules

- **MAJOR** — the configuration format or the meaning of existing settings
  breaks, or a user-visible workflow is removed.
- **MINOR** — new formats, new options, new UI capabilities.
- **PATCH** — fixes and translation updates that do not change behaviour.

## Release checklist

1. Bump `VERSION`.
2. Move `## [Unreleased]` entries in `CHANGELOG.md` under the new version with the
   release date.
3. Bump `pkgrel` to `1` in `PKGBUILD` (increment it for packaging-only rebuilds of
   the same version).
4. Rebuild and run the tests:
   `cmake -B build -S . -DLUMA_CONVERTER_BUILD_TESTS=ON && ctest --test-dir build`.
