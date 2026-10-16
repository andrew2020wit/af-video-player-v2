#!/usr/bin/env bash
# Builds the Linux release: dist/af-video-player-<version>-linux-x86_64.AppImage (P-10).
# Requires: CMake, Ninja, Qt (QT_ROOT_DIR), libmpv and FFmpeg development packages, curl.
set -euo pipefail

version="${1:?usage: package-linux.sh <version>}"
root="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="$root/build/package-linux"
app_dir="$build_dir/AppDir"
tools_dir="$build_dir/tools"
dist_dir="$root/dist"

cmake -S "$root" -B "$build_dir" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_PREFIX_PATH="${QT_ROOT_DIR:-}"
cmake --build "$build_dir" --target af-video-player
rm -rf "$app_dir"
DESTDIR="$app_dir" cmake --install "$build_dir"

mkdir -p "$tools_dir" "$dist_dir"

for tool in linuxdeploy-x86_64.AppImage linuxdeploy-plugin-qt-x86_64.AppImage; do
    if [ ! -x "$tools_dir/$tool" ]; then
        project="${tool%-x86_64.AppImage}"
        curl -sfL -o "$tools_dir/$tool" "https://github.com/linuxdeploy/$project/releases/download/continuous/$tool"
        chmod +x "$tools_dir/$tool"
    fi
done

# GitHub runners have no FUSE.
export APPIMAGE_EXTRACT_AND_RUN=1
export QMAKE="${QT_ROOT_DIR:+$QT_ROOT_DIR/bin/}qmake"
export QML_SOURCES_PATHS="$root/src/app/qml"
export LINUXDEPLOY_OUTPUT_VERSION="$version"
export PATH="$tools_dir:$PATH"

cd "$build_dir"
linuxdeploy-x86_64.AppImage --appdir "$app_dir" --plugin qt --output appimage \
    --desktop-file "$app_dir/usr/share/applications/af-video-player.desktop" \
    --icon-file "$app_dir/usr/share/icons/hicolor/scalable/apps/af-video-player.svg"

mv "$build_dir"/AF_Video_Player-*.AppImage "$dist_dir/af-video-player-$version-linux-x86_64.AppImage"
echo "Created $dist_dir/af-video-player-$version-linux-x86_64.AppImage"
