# Maintainer: B612S

pkgname=luma-converter
# VERSION is the single source of the application version (MAJOR.MINOR.PATCH).
pkgver="$(tr -d '[:space:]' < "${startdir:-$PWD}/VERSION" 2>/dev/null || true)"
pkgver="${pkgver:-1.0.0}"
# Increment for a packaging-only rebuild; reset to 1 when pkgver changes.
pkgrel=1
pkgdesc="Audio converter with a Kirigami interface, powered by FFmpeg"
arch=('x86_64' 'aarch64')
url="https://github.com/B612S/luma-converter"
license=('GPL-3.0-or-later')

depends=(
    'qt6-base>=6.6.0'
    'qt6-declarative>=6.6.0'
    'qt6-svg>=6.6.0'
    'kirigami'
    'ffmpeg'
    'hicolor-icon-theme'
    'plasma-integration'
    'qqc2-desktop-style'
)

makedepends=(
    'cmake'
    'ninja'
    'pkgconf'
    'qt6-tools>=6.6.0'
)

# This PKGBUILD packages the working tree, so there is no remote source.
source=()
sha256sums=()

prepare() {
    # Copy the checkout into $srcdir without local build, VCS or packaging data.
    rm -rf "$srcdir/luma-converter"
    mkdir -p "$srcdir/luma-converter"
    tar -C "${startdir:-$PWD}" \
        --exclude='./.git' \
        --exclude='./build' \
        --exclude='./dist' \
        -cf - . | tar -C "$srcdir/luma-converter" -xf -
}

build() {
    cd "$srcdir/luma-converter"

    cmake -B build -S . \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DLUMA_CONVERTER_BUILD_TESTS=OFF \
        -G Ninja

    cmake --build build
}

package() {
    cd "$srcdir/luma-converter"

    # Installs the binary, QML module, translations, desktop file and icons.
    DESTDIR="$pkgdir" cmake --install build

    # This deployment map is not needed by the installed QML module.
    rm -f "$pkgdir/usr/lib/qt6/qml/LumaConverter/luma_converter_qml_module_dir_map.qrc"
}
