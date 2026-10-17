# AGENTS.md

Instructions for AI coding agents working in this repository.

## Project

AF Video Player — a desktop video player for watching movies with subtitles, aimed at language learning.
`SPEC.md` is the reference for its behaviour.

## Current phase

- Version 1.0 scope (all P1 requirements) is implemented. `TODO.md` lists what is left: things to verify,
  the release and P2/P3 requirements. Discuss P2/P3 work with the user before starting it.
- `SPEC.md` describes only what the player does. Do not implement new features without the user's approval;
  an approved feature is added to `SPEC.md` as a requirement.

## Code layout

- `src/core` — static library `afvp_core`, Qt Core only: subtitle parsing and merging, word lookup, languages,
  settings and history storage, subtitle file search, next video in a folder, export file naming. All testable logic lives here.
- `src/export` — `afvp_export`: probing and extraction of embedded subtitles with libavformat.
- `src/app` — the executable: `MpvObject` (libmpv render API in Qt Quick), `AppController` (QML singleton
  `App`), `SubtitleListModel`, QML in `src/app/qml`. QML/JavaScript holds presentation logic only.
- `tests` — Qt Test unit tests (one executable per area), sample files in `tests/data`.
- `scripts` — fetching Windows dependencies, packaging; `.github/workflows` — CI and releases.
- UI strings are English, wrapped in `qsTr()` (SPEC P-13).

## Build and test

- Windows: `scripts/fetch-deps-windows.ps1` once, then with `QT_ROOT_DIR` set to the Qt MSVC kit:
  `cmake --preset windows`, `cmake --build --preset windows`, `ctest --preset windows`.
- Linux / macOS: the `unix` presets; libmpv and FFmpeg come from system packages (pkg-config).
- Run `ctest` after changing `src/core`, `src/export` or QML; add data-driven tests for new core logic.
  The `qmllint` test fails on any qmllint warning in `src/app/qml` (an unknown property, a handler of a missing
  signal): QML errors otherwise show up only when the window loads.
- Step by step for this machine (tools not on `PATH`, the local portable build in
  `dist/windows/af-video-player/`, the default kind of build):
  `.claude/skills/local-build/SKILL.md`.

## Key decisions (see `SPEC.md`, section 0)

- Stack: C++20 core, Qt 6 / QML UI (UI logic in JavaScript), libmpv for playback, FFmpeg (`libavformat`)
  for subtitle extraction. Other stacks in `SPEC.md`, section 3.3, are fallbacks only.
- Free software, GPL-3.0-or-later. Use only open source technologies and libraries with compatible licenses.
- Target platforms: Windows, Linux and macOS.

## Files

- `README.md` — project overview.
- `SCREENSHOTS.md` — screenshots of the player (images in `screenshots/`), linked from `README.md`.
- `SPEC.md` — specification: principles and decisions (`P-xx`), basic functionality (`F-xx`),
  requirements (`I-xx`), technology stack.
- `TODO.md` — work left to do (verification, release, P2/P3) with references to `SPEC.md`.
  Tick or remove items when they are done.
- `THIRD-PARTY.md` — third-party libraries in the release packages and their licenses; installed next to `LICENSE`.
  Update it when the bundled libraries or their sources change.
- `AGENTS.md` — this file.

## Documentation rules

- All documentation (`README.md`, `SPEC.md`, `TODO.md`, `AGENTS.md`), code, code comments and commit messages
  are in **English**.
- The documentation describes the player on its own; it does not compare it with other players or earlier
  versions.
- The documentation must not promise any warranty or guarantee from the developer (see the disclaimer in
  `README.md`): describe behaviour and goals, not guarantees.
- IDs in `SPEC.md` are never renumbered or reused; add new items with the next free number (new questions
  start at `Q-11`). Obsolete items may be removed, leaving a gap in the numbering.
- When a decision is made in a discussion, record it in `SPEC.md`.

## Versioning

- The version is `project(VERSION ...)` in the root `CMakeLists.txt`; the app shows it in the window title (I-83).
- After every code change, increment the build number (the last number of the version) in the same commit.
  Documentation-only changes do not bump it.
- Each time the version changes, tell the user the new value in chat.

## Git

- Commit logically complete changes with meaningful messages.
- Never push and never suggest pushing; push only on an explicit request.
