---
name: local-build
description: Build AF Video Player for local use - the dev build with tests, or the developer's portable Windows folder dist/windows/af-video-player that runs by double click. Use when asked to build, rebuild, package or "make the project runnable locally" (e.g. "собери", "собери проект", "собери для локального запуска").
---

# Local build

Kinds of build (see `README.md`, "Build"):

- **Dev build** - `build/windows`, runs tests; the exe there needs Qt's `bin` on `PATH` to start.
- **Local portable build (the default)** - `dist/windows/af-video-player/`: the exe with Qt, mpv, FFmpeg and
  MSVC runtime next to it, no version in the path, no zip. This is what "build" / "build for local run" means:
  it starts by double click. Each build replaces the previous folder.
- **Release package** - `dist/af-video-player-<version>-windows-x64/` plus a zip. Only when the user asks for
  a release package or a zip explicitly; real releases are built by GitHub Actions on a version tag.

## 1. Find the tools

`cmake` is often not on `PATH`, and `QT_ROOT_DIR` is often not set. Take both from the existing dev build's
cache when it exists:

```bash
grep -E "^CMAKE_COMMAND:|^Qt6_DIR:" build/windows/CMakeCache.txt
```

- cmake: the `CMAKE_COMMAND` path (`ctest.exe` is in the same folder).
- Qt: `Qt6_DIR` without the trailing `/lib/cmake/Qt6`, e.g. `C:/Qt/6.11.3/msvc2022_64`.

Without the cache: `where cmake`, and the newest `C:\Qt\<version>\msvc*_64`. If nothing is found, ask the user.

`deps/` (libmpv, FFmpeg) must exist; if not, run `scripts/fetch-deps-windows.ps1` first.

## 2. Dev build and tests

```bash
CM="<folder of cmake.exe>"
"$CM/cmake.exe" --preset windows      # only if build/windows does not exist yet; needs QT_ROOT_DIR
"$CM/cmake.exe" --build --preset windows 2>&1 | grep -iE "error|warning C"
"$CM/ctest.exe" --preset windows 2>&1 | tail -3
```

Fix build errors and failing tests before packaging.

## 3. Local portable build

Run with the PowerShell tool:

```powershell
$env:QT_ROOT_DIR = "<Qt folder>"; $env:PATH = "<folder of cmake.exe>;$env:PATH"; & .\scripts\package-windows.ps1 -Local | Select-Object -Last 8
```

- **Do not redirect stderr** (`*>&1`, `2>&1`): the script sets `$ErrorActionPreference = "Stop"`, and in
  Windows PowerShell 5.1 a redirected stderr line of a native command becomes a terminating error, so a mere
  windeployqt warning fails the run.
- Expected, harmless warnings: `dxcompiler.dll and dxil.dll` not found (the player renders through OpenGL),
  `VCINSTALLDIR is not set`.
- The script removes `dist/windows/af-video-player/` before installing. If that fails (access denied), the
  player is most likely running from that folder: ask the user to close it.

Release package instead (only on request): `-Version <version>` in place of `-Local`, where the version is
`VERSION` in the `project()` call of the root `CMakeLists.txt`.

## 4. Check the result

```bash
D=dist/windows/af-video-player
ls -la "$D/bin/af-video-player.exe"            # timestamp must be from this run
ls "$D/bin" | grep -iE "libmpv|avformat|Qt6Quick.dll"
ls "$D/qml/QtQuick/Controls" "$D/plugins/platforms/qwindows.dll"
```

Report the exe path (`dist\windows\af-video-player\bin\af-video-player.exe`), and say plainly that the window
itself was not started unless it was.

## Linux / macOS

`cmake --preset unix`, `cmake --build --preset unix`, `ctest --preset unix`; packages:
`scripts/package-linux.sh <version>` (AppImage), `scripts/package-macos.sh <version>` (zip with `.app`).
