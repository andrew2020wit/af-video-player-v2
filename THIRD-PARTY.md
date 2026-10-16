# Third-party software

AF Video Player is licensed under GPL-3.0-or-later (see [LICENSE](LICENSE)). The release packages also contain
the third-party libraries listed below, each under its own license. The exact versions of Qt, libmpv and FFmpeg in
a package are shown in the player: Settings → About.

## Libraries

| Library | License | Source code |
|---------|---------|-------------|
| [Qt 6](https://www.qt.io/) (Qt Base, Qt Declarative, Qt SVG and their plugins) | LGPL-3.0 | <https://download.qt.io/official_releases/qt/> |
| [mpv](https://mpv.io/) (libmpv) | GPL-2.0-or-later | <https://github.com/mpv-player/mpv> |
| [FFmpeg](https://ffmpeg.org/) (libavformat, libavcodec, libavutil, libswscale, libswresample) | LGPL-2.1-or-later; GPL-2.0-or-later for builds with GPL parts | <https://ffmpeg.org/download.html> |

libmpv and FFmpeg in turn use other open source libraries (for example libass, FreeType, HarfBuzz, FriBidi,
libplacebo, zlib). Which of them are in a package and how they are built depends on the platform:

- **Windows** (zip): Qt from the official Qt builds; libmpv from
  [shinchiro/mpv-winbuild-cmake](https://github.com/shinchiro/mpv-winbuild-cmake), which links its dependencies
  statically; FFmpeg from the LGPL shared builds of [BtbN/FFmpeg-Builds](https://github.com/BtbN/FFmpeg-Builds).
  The build recipes of both projects list every library with its version and source. The pinned builds are in
  [scripts/fetch-deps-windows.ps1](scripts/fetch-deps-windows.ps1). The package also contains the Microsoft Visual
  C++ runtime, redistributed under the Microsoft Visual Studio license terms.
- **Linux** (AppImage): Qt from the official Qt builds; libmpv, FFmpeg and the libraries they depend on from
  Ubuntu 22.04 packages. Their sources and licenses are available from Ubuntu: <https://packages.ubuntu.com/source/jammy/mpv>,
  <https://packages.ubuntu.com/source/jammy/ffmpeg>.
- **macOS** (zip with `.app`): Qt from the official Qt builds; libmpv, FFmpeg and the libraries they depend on from
  Homebrew. The formulae with their sources and licenses: <https://formulae.brew.sh/formula/mpv>,
  <https://formulae.brew.sh/formula/ffmpeg>.

The license texts of these libraries are available at the source links above. The source code of AF Video Player
itself: <https://github.com/andrew2020wit/af-video-player-v2>.
