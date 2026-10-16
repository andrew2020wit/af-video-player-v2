#!/usr/bin/env bash
# Builds the macOS release: dist/af-video-player-<version>-macos-arm64.zip with the .app bundle (P-10).
# Requires: CMake, Ninja, Qt (QT_ROOT_DIR), Homebrew mpv and ffmpeg.
set -euo pipefail

version="${1:?usage: package-macos.sh <version>}"
root="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="$root/build/package-macos"
stage_dir="$build_dir/install"
dist_dir="$root/dist"

cmake -S "$root" -B "$build_dir" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="${QT_ROOT_DIR:-}"
cmake --build "$build_dir" --target af-video-player
rm -rf "$stage_dir"
# macdeployqt (through the CMake deploy script) also copies non-Qt libraries such as libmpv into the bundle.
cmake --install "$build_dir" --prefix "$stage_dir"

mkdir -p "$dist_dir"
archive="$dist_dir/af-video-player-$version-macos-arm64.zip"
rm -f "$archive"
ditto -c -k --keepParent "$stage_dir/af-video-player.app" "$archive"
echo "Created $archive"
