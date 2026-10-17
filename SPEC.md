# AF Video Player specification

> Status: **implementation**. The P1 requirements (version 1.0) are implemented; P2 and P3 only after discussion.
> What is left to do is in [TODO.md](TODO.md).

The document has these parts:

0. [Principles and decisions](#0-principles-and-decisions)
1. [Basic functionality](#1-basic-functionality) — the main scenario: playback and the subtitle list.
2. [Requirements](#2-requirements) — everything else the player does.
3. [Technology stack](#3-technology-stack) — the chosen stack, fallbacks, technical notes.

Items are numbered (`P-xx`, `F-xx`, `I-xx`) so that they can be referenced. Numbers are never reused, so the
numbering has gaps. Questions Q-01…Q-10 are resolved and recorded in section 0; new questions are numbered from
Q-11.

UI labels are quoted as they appear in the interface (English, P-13).

---

## 0. Principles and decisions

- **P-01. Free software.** The source code is open, the license is **GPL-3.0-or-later**.
- **P-02. Open source only.** Only open source technologies and libraries with licenses compatible with
  GPL-3.0-or-later are used.
- **P-03. Cross-platform.** Windows, Linux and macOS.
- **P-04. Full-window subtitle list** (not a side panel); the video is paused while it is shown.
- **P-06. No web stack for the UI.** Electron, Tauri and WebView are not used as the basis of the interface: video
  in them is played by a browser engine, which lacks AC3/DTS/TrueHD, some video codecs and full MKV support.
- **P-07. No built-in translation.** For now there is no translation of cues inside the application — neither by
  offline models nor through online APIs. Translation happens only in the external browser (I-47).
- **P-08. Stack: Qt 6 / QML + libmpv** + FFmpeg for subtitle extraction. Other stacks (3.3) are considered only if
  this one runs into problems.
- **P-09. C++20 core:** libmpv and FFmpeg, parsing and merging of subtitles, history, settings. The interface and
  its logic are QML/JavaScript.
- **P-10. Distribution:** GitHub Releases with portable builds for Windows (zip), Linux (AppImage) and macOS (zip
  with an arm64 `.app`). GitHub Actions makes the builds when a version git tag is created. No code signing with a
  certificate (the first start shows SmartScreen / Gatekeeper warnings; the macOS bundle gets only an ad-hoc
  signature, without which Apple Silicon does not run it) and no installers. Each package contains `LICENSE` and
  `THIRD-PARTY.md` — the bundled third-party libraries, their licenses and where their source code is.
- **P-11. Text subtitles only.** Image-based subtitles (PGS, VobSub, DVB) are not supported and are ignored
  completely: they are not shown over the video, not listed among tracks and not exported.
- **P-12. Subtitles come from external files only.** For a video the user chooses two external subtitle files:
  the main one (original) and the secondary one (translation). They are shown in the full-window list
  (F-42–F-44). Embedded tracks are not used directly (and mpv does not render them over the video); without
  external files the user first exports embedded subtitles to external files (I-08).
- **P-13. The interface language is English.** Strings are wrapped in `qsTr()`, so translations can be added later.
- **P-14. A hand cursor over anything clickable.** Over every enabled clickable element (buttons, including those
  in dialogs and menus, combo boxes, spin box arrows, cue time and text, the progress bar) the mouse cursor is a
  hand; over disabled elements it is the normal arrow, over text fields an I-beam. For Qt Quick Controls this is
  done by one event filter in C++, not by setting up each element.
- **P-15. No Anki integration.** No card export, no "for Anki import" formats, no media for cards.
- **P-16. No file associations.** The application does not register itself as a handler of video file types.
  Files are opened through "Open with", drag & drop and the command line (I-05); the user can make the player the
  default application with the OS's own tools.

---

## 1. Basic functionality

Purpose: watching films with subtitles to learn a language. The key idea: during playback subtitles do not get in
the way, and on demand (`Space`) the video pauses and the **whole list of subtitles** is shown in a column to work
with: look words up in a
dictionary, translate, jump to a time.

### 1.1. Playback and the control panel

- **F-10. The video** fills the whole window, keeping its aspect ratio.
- **F-11. A click on the video** toggles play/pause without the subtitle list (F-42), like `Ctrl+Space` and the
  play/pause button on the panel. The click takes effect after the double-click interval, so that a double click
  (full screen, I-30) does not pause the video.
- **F-12. The bottom control panel** over the video: play/pause, progress bar, time `current / total` (format
  `H:MM:SS` or `MM:SS`) and buttons — see "Control panel" in 2.2.
- **F-13. Seeking by a click** on the progress bar; dragging — I-34.
- **F-14. Auto-hide:** the panel and the mouse cursor hide after 2 s without mouse movement and appear on movement
  (with a smooth opacity change). They do not hide on pause, while the mouse is over the panel or while the video
  context menu is open.
- **F-15. Hotkeys:** `Space` — the subtitle list (F-42), `Ctrl+Space` — play/pause, `←` / `→` — seek 5 s back /
  forward. The full list is in 2.4.

### 1.2. The subtitle list

- **F-41. During playback subtitles are not shown over the video** by default; they are seen in the list (F-42).
  Subtitles over the video can be turned on (I-22).
- **F-42. The subtitle list** is a separate action: `Space` covers the video completely with a full-window list of
  all cues of the main subtitle file, and the video pauses as a consequence. A pause by itself (a click on the
  video, `Ctrl+Space`, the panel button) does not open the list. `Space` again, `Ctrl+Space`, `Esc`, the `✕` button in
  the top right corner or a click on a cue time (F-46) close the list and continue playback. While the
  list is open the control panel is hidden, and clicks and the mouse wheel outside the cues do not act on the
  video (the wheel only scrolls the list, never changes the volume). The list also opens when the video has played
  to the end (I-16). Below the `✕` button are the buttons "Settings" (I-50) and "Subtitles" (`CC`, I-57), as in
  the control panel.
- **F-43. A list row** contains, from left to right:
  - the secondary subtitle text (dark green), see F-44;
  - the start time of the cue (grey); a click — F-46;
  - the main cue text (large);
  - on hover — the translation button `G` (F-51).

  The secondary and main texts share the width left by the time and the button as 40% and 60%: the original
  gets more space. Their first lines are aligned on the baseline.
  When a secondary file is chosen, its column is kept for cues without a translation, so the columns are even
  across the list; when it is not chosen, the time and the main text take the whole width.
- **F-44. Secondary subtitles are attached to the main ones.** They are not shown as separate rows: each secondary
  cue is attached to one main cue (algorithm — I-20).
- **F-45. Row highlights:**
  - *current* — the cue playback is at (the last one with `start ≤ currentTime`); when the list opens it is
    scrolled so that this cue is in the centre;
  - *previous current* (light yellow) — the cue that was current when the list was closed last time; it shows how
    much has been watched since the last pause;
  - *marked by the user* (bold) — by a right click on the row. There is one mark, in memory, not kept between
    sessions.
- **F-46. Jump to a time:** a click on the cue time seeks to its start, starts playback and closes the list.
- **F-47. Pause indicator:** between cues with a pause of 5 s or more a horizontal bar is drawn, its length
  proportional to the pause (100% = 60 s or more). It helps to see "scenes".

**Cue text in the list** (main and secondary):

- line breaks inside a cue are replaced with a space — they exist for the screen width, not for the list;
- a line break is kept when the next line starts with `-`, `–` or `—` (lines of different speakers);
- spaces at the edges are trimmed, repeated spaces are collapsed;
- cues that are empty after removing tags are dropped when the file is parsed.

### 1.3. Dictionary and translation

- **F-31. Dictionary lookup:** a click on the text of a cue (main or secondary) opens the online dictionary (I-42)
  in the default browser with the word under the cursor or the selected text (rules — I-59).
- **F-51. The `G` button** opens the online translator (I-47) with the text of the main cue in the default browser.

---

## 2. Requirements

Priorities: **P1** — required for the first version, **P2** — desirable, **P3** — later.

### 2.1. Formats and files

| ID   | Requirement | Priority |
|------|-------------|----------|
| I-01 | Playback of the formats supported by libmpv / FFmpeg (MKV, AVI, HEVC, AC3/DTS, …) | P1 |
| I-02 | Choice of the audio track during playback (menu + hotkey); the choice is remembered for the file | P1 |
| I-04 | External subtitles in SRT, VTT, ASS/SSA (formatting tags are removed). The format is taken from the extension; for another extension — from the content (`WEBVTT` header, `[Script Info]` / `[Events]`, `-->`). Files larger than 64 MB and files of an unknown format are not opened, the error is shown in a dialog | P1 |
| I-05 | Opening files from the file manager ("Open with"), by drag & drop and as a command line argument. A subtitle file (`.srt`, `.vtt`, `.ass`, `.ssa`) dropped onto the window with an open video becomes its main subtitles (a user's choice, I-06), without an open video it is not opened and a message says so; any other dropped file is opened as a video. The application does not register file type associations (P-16) | P1 |
| I-06 | Automatic search for external subtitles next to the video (`movie.srt`, `movie.en.srt`, `movie.uk.vtt`, `movie.en.forced.srt`, `movie.en.2.srt` — see the export file names of I-08, the `Subs/` folder). When the user has not chosen subtitles for this video yet, the found files are assigned as main and secondary by the language in the file name (I-55); the automatic search does not change the user's choice | P2 |
| I-08 | **Export of embedded subtitles** from a video file to an external file (SRT / VTT / ASS) — of one or all text tracks. It should be **fast**: all tracks in one pass over the file, in the background, with progress and cancellation. It is the only way to use embedded subtitles (P-12). The scenario is below, the technique — 3.2 | P1 |
| I-85 | **Automatic export of embedded subtitles** for a new video without subtitle files: a track in the studied language and a track in the native one are exported next to the video and become its subtitles. Rules — below | P2 |

**Export scenario (I-08):**

1. For the open video the user starts the export of subtitles ("Export embedded…").
2. The list of embedded text tracks is shown (image-based ones are not, P-11) — with the language and the track
   title from the container metadata when they exist, the codec and the "forced" / "SDH" flags. Each track has
   a check box.
3. The user checks the tracks to export and presses "Save…" (the button is disabled while nothing is checked).
4. The system folder dialog opens. By default it is the folder of the video file.
5. Each checked track is saved to the chosen folder as a separate file.
6. After saving the application offers to use the exported files for this video right away: to choose the main
   subtitles (original) and the secondary ones (translation) among them. The user can confirm ("Use") or decline
   ("Don't use") — then the current subtitles of the video do not change. The choice is pre-filled by the
   language of the tracks (I-55): main — the track in the studied language, secondary — in the native one. Each
   list also has the item "— keep current —": with it the current file is not changed; it is pre-selected when
   no track in the language exists.

**File format.** A track is saved in its own format, without conversion: SubRip → `.srt`, ASS → `.ass`,
SSA → `.ssa`, WebVTT → `.vtt`. Packets are copied as they are, without re-encoding, which keeps the export fast.
The exception is `mov_text` (MP4/MOV): it has no file format of its own and is converted to `.srt`. The dialog has
no choice of format. Files are written in UTF-8.

**File names:** `<video name>.<language>[.forced][.sdh].<extension>`, for example `movie.en.srt`,
`movie.en.forced.srt`, `movie.en.sdh.srt`. This is the naming convention used by mpv, VLC and Jellyfin/Plex, and
the automatic search (I-06) reads it.

- `<video name>` — the video file name without the extension.
- `<language>` — the language tag of the track from the container, converted to ISO 639-1 when such a code exists
  (`eng` → `en`); otherwise the tag as it is. Without a language — `und`.
- `.forced` and `.sdh` are added by the track flags in the container ("forced" and "hearing impaired").
- When several checked tracks get the same name, a number is added from the second one on: `movie.en.srt`,
  `movie.en.2.srt`, `movie.en.3.srt` (in the order of tracks in the container).

**Existing files.** After the folder is chosen (step 4) and before the export starts, the application checks
whether the folder has files with these names. If it does, their list is shown with one question for all of them:
"Replace", "Keep both" (the next free number, `movie.en.2.srt`) or "Cancel" (back to the choice of tracks).

**Progress.** While the export runs, a small window in the corner shows the progress and a "Cancel" button; the
video can be watched meanwhile. An export error is shown in a dialog; a cancelled export shows nothing.

**Automatic subtitle search (I-06).** It runs every time a video is opened, when the "Subtitles" dialog (I-57) opens
and after an export; file and folder names are compared case-insensitively. It looks for `.srt`, `.vtt`, `.ass`,
`.ssa` files:

- in the video folder — files whose name starts with the video name without the extension followed by `.`, `_`,
  `-` or a space: `movie.en.srt`, `movie.srt`, but not `movie2.srt`;
- in a `Subs/` or `Subtitles/` subfolder — the same; when the folder has only one video — all subtitle files;
- in a `Subs/<video name>/` subfolder (typical for series: `Subs/s02e05/2_English.srt`) — all files.

**The language of a file** is found from the parts of its name separated by `.`, `_`, `-` or a space (when the name
starts with the video name — only from the parts after it): the first part that is an ISO 639-1 / 639-2 code
(`en`, `eng`) or an English language name (`English`, `Russian`). The parts `forced` and `sdh`/`hi`/`cc` are flags,
as in the export names (I-08); `hi` is always the "hearing impaired" flag, not the code of Hindi. Parts that match
short language codes are not excluded even when they look like ordinary words: `Subs/it.srt` is Italian. False
matches in such names are rare, and this is acceptable.

**Assignment** (only when the file has no saved choice of the user, I-13): main — the file in the studied language,
secondary — in the native one (I-55); when the studied and native languages are the same, there is no secondary
file. When several files have the same language, the choice is: without flags, then `sdh`, then `forced`; on a tie
— the video folder before subfolders, then by name. When exactly one file is found and its language is unknown, it
becomes the main one. When there is no file in the studied language but there is one in the native language, it
becomes the main one and there is no secondary: otherwise the subtitle list would be empty although a translation
exists.

The result of the automatic search is not saved as the user's choice (like the automatic choice of the audio track,
I-54) and is recalculated every time the video is opened and when the studied or native language changes. **The
user's choice** is any change in the "Subtitles" dialog (I-57), a subtitle file dropped onto the window (I-05) or
acceptance of the exported files (I-08, step 6) or the automatic export (I-85): then both current files, including one found by the automatic
search, and the `subtitlesChosen` flag (3.4) are written to the history; after that the automatic search no longer
applies to the video. Removing both files is a choice too: the video stays without subtitles.

All found files are listed in the "Subtitles" dialog (I-57) under "Found next to the video:" for a quick choice:
the path relative to the video folder, the language and flags, the "Main" and "Secondary" buttons. When the current
subtitles were chosen by the automatic search, the dialog says so.

**Automatic export (I-85).** When a video that is not in the history is opened (a new entry by I-12, including
another file at the same path) and the automatic search (I-06) finds no subtitle file, embedded text tracks are
exported without questions:

- forced tracks are skipped: they contain only part of the text (signs, foreign speech). A track is forced by the
  container flag or when its title contains the word `forced`;
- of the other tracks, the first one (in the order of the container) in the studied language and the first one in
  the native language (I-55) are exported; when the two languages are the same — one track. Tracks without a
  language are not exported;
- SDH tracks (the "hearing impaired" flag, or the words `SDH`, `CC`, `HI` in the title) are taken only when the
  language has no other track.

When no track fits, nothing happens. Otherwise the export runs as in I-08: the files are written to the video folder
with the names of I-08 (an existing file is never replaced — "Keep both"), in the background, with the progress
window and "Cancel"; an error is shown in a dialog. It does not start while another export is running.

After the export the files become the subtitles of this video as the user's choice (I-13, `subtitlesChosen`),
without the question of step 6: main — the track in the studied language, secondary — the one in the native
language; when there is no track in the studied language, the native one becomes main and there is no secondary
(as in I-06). The files are assigned even when another video was opened meanwhile, but not when the user has chosen
subtitles for the video during the export.

### 2.2. Files and history

| ID   | Requirement | Priority |
|------|-------------|----------|
| I-10 | An "Open file" button and key during playback | P1 |
| I-11 | Opening a file by a click in the history (by the full path) | P1 |
| I-12 | A file is identified by its path and size, not by its name — so that episodes with the same name from different seasons/folders are not mixed up and the data of a moved file is not lost. Rules — below | P2 |
| I-13 | For each file in the history remember: the position, the chosen audio track, the chosen subtitles (main and secondary), the subtitle offsets | P1 |
| I-14 | Save the position periodically (every N seconds + on pause/close); keep the settings and the history in JSON files in the user directories of the OS (see 3.4) | P1 |
| I-15 | Removing entries from the history: the ✕ button on a hovered entry of the start page removes it at once, on a series header (I-76) — all its entries after a confirmation; clearing the whole history — in the settings. The history size is a setting (1–1000 files) | P3 |
| I-16 | "Next file in the folder" (next episode) | P2 |
| I-86 | The file open dialog starts where the last video was opened. Rules — below | P2 |
| I-56 | The start page: opening a video and the history. Description — below | P1 |
| I-57 | The "Subtitles" dialog in the player: choice of the main and secondary files, export of embedded ones (I-08) | P1 |
| I-58 | Back to the start page from the player (a button on the panel, `Ctrl+H`) | P1 |

**The start page (I-56)** is shown at a start without a file and by I-58. It contains:

- the "Open video…" button (`Ctrl+O`) and a hint that a file can be dropped onto the window (I-05);
- the history (I-11): file name, folder, watched share in % (`positionMs / durationMs`), date of the last opening;
  a click opens the file with the data saved for it (I-13); entries of files that no longer exist on disk are not
  shown but stay in `history.json` — to find a moved file (I-12); they, like the oldest entries, are pushed out by
  the `historySize` limit; a hovered entry gets a hand cursor and an outline; entries of series are grouped
  (I-76);
- in the top right corner: the "GitHub" button, which opens the project page in the browser, the settings
  button (I-50) and the full screen button (I-30); `F` toggles full screen here too, `Esc` leaves it.

The chosen video starts playing at once. From the file manager, the command line or by drag & drop the video opens
at once, without the start page.

**The file open dialog (I-86)** — "Open video…" on the start page and "Open file" on the panel (I-10). The player
remembers the last opened video, however it was opened (the dialog, the history, drag & drop, the command line),
also between runs (`lastOpenFile`, 3.4). The dialog opens in the folder of that video with the video selected; when
the video no longer exists — in its folder without a selection; when the folder no longer exists — in its nearest
existing parent folder, up to the root of the drive; when none exists (a removed drive) or no video has been opened
yet — in the user's videos folder.

**Series (I-76).** The history on the start page is grouped by folders (groups are ordered by the most recent
opening in them). A folder with two or more video files on disk is a series: one header row with the folder name,
"N of M watched" (M — all video files of the folder), the "Continue: <file>" button and the date of the last
opening; a click on the header expands the history entries of this folder (indented, without the path); the
expanded state is kept until the application exits. A file is watched when it has been played to at least 95%
(`positionMs / durationMs`); it gets a ✓ mark instead of the percentage. "Continue" is the last opened episode when
it is not watched, otherwise the first unwatched one after it in the natural order of names (I-16); there is no
button when there is no such file. Other folders are shown as plain entries. The setting "Start watched files from
the beginning" (`restartWatched`, on by default) in the "History" section: a watched file opens from the beginning,
not from the saved position. The list is built only while the start page is visible — it reads the contents of
folders.

**Finding the history entry (I-12)** when a video is opened (paths are compared case-insensitively on Windows):

1. An entry with the same path. When its `size` equals the file size, it is the same file. When it does not, another
   file lies at the same path: the old entry is replaced with a new one, the position, audio track and subtitles
   start from scratch.
2. Otherwise — an entry with the same file name and size whose file no longer exists at the old path (the file was
   moved to another folder; when the old file is still in place, the new one is a copy and the entry is not taken).
   When there are several, the most recently opened one. The entry moves to the new path.
3. Otherwise — a new entry.

When a file is moved (step 2), the subtitle paths are recalculated: a subtitle file from the old video folder or
its subfolders is looked for at the same relative path in the new folder; a file outside the old folder stays when
it exists. When at least one subtitle file is not found, the choice of subtitles is reset (`subtitlesChosen` =
false) and the automatic search (I-06) applies. A renamed file (another name) is not recognised. A hash of the file
contents is not used: the name and the size are enough, and a hash needs reading the file at every opening.

**Subtitles when a video is opened:** 1) the ones saved for the file in the history (I-13); 2) otherwise — the ones
found by the automatic search (I-06); 3) otherwise — none; for a video that is not in the history the automatic
export (I-85) may then add them. Without main subtitles the subtitle list shows a hint
instead of the cues: "No subtitles", the "Choose files…" button and, when the video has embedded text tracks, "Export
embedded…" (I-08).

**The "Subtitles" dialog (I-57)** is opened by the `CC` button on the control panel. It contains:

- the main file: name, a button to choose a file (the file dialog opens in the video folder), a "Remove" button;
- the secondary file — the same;
- the list of found files (I-06);
- the "Export embedded…" button (I-08) when the video has embedded text tracks.

Changes apply at once and are saved in the history (I-13).

**The control panel** (F-12): play/pause, progress bar, time, the buttons "Subtitles" (`CC`, I-57), "Open file"
(I-10), "Start page" (I-58), "Settings" (I-50), "Full screen" (I-30). "Open file" (I-10) shows the system file
dialog as on the start page (I-86); on cancel the playback continues. While the subtitle list is open
(F-42) the panel is hidden — the list takes the whole window; the panel appears again when the list closes.

**The video context menu** — a right click on the video (in the subtitle list a right click marks a cue, F-45): "Subtitles on video (S)" (I-22, a check item) and the "Audio track (A)" submenu (I-02).

**Subtitles on video (I-22)** — over the video during playback and on a pause without the list: the main ones at the top, the secondary ones at
the bottom (above the panel when it is visible). Each are shown by their own timing: the cues whose interval
contains the current moment, each overlapping cue on its own line; nothing between cues. The lines of a cue are
joined as in the list (1.2, "Cue text in the list": a break is kept only before a speaker dash), and the text wraps
at 90% of the window width, but not wider than 1200 px. White text on a semi-transparent dark background. Font sizes are their own, independent of the sizes in the list (I-50). Turned on and
off by the `S` key (with an OSD, I-60) or in the context menu; remembered in `showSubtitleOverlay`. The subtitle
list (F-42) covers them.

**Next file in the folder (I-16)** — the next video file in the folder of the current one (by extension: `mkv`,
`mp4`, `avi`, `mov`, `webm`, `m4v`, `wmv`, `ts`, `m2ts` and others) in the "natural" order of names:
case-insensitive, numbers are compared as numbers (`e9` < `e10`); names without the extension are compared first
(`Movie.mkv` < `Movie 2.mkv`). It is opened by the `N` key; for the last file — the OSD "This is the last file"
(I-60). When the video has played to the end, it stops on the last frame and the subtitle list opens;
at the top of the list there is the "Next: `s02e06.mkv`" button (when there is a next file). There is no automatic
move to the next file.

### 2.3. Subtitles

| ID   | Requirement | Priority |
|------|-------------|----------|
| I-20 | Secondary subtitles are attached to the main cues by the overlap of intervals. Algorithm — below | P1 |
| I-22 | Optional display of the current cues over the video during playback: the main ones at the top, the secondary ones at the bottom (off by default, toggled by the `S` key and in the video context menu). Description — 2.2, "Subtitles on video" | P2 |
| I-26 | Configurable font sizes of the subtitle list | P2 |
| I-55 | The settings "studied language" and "native language". They are used to pre-select subtitles: main — in the studied language, secondary — in the native one (step 6 of the export scenario I-08, the automatic search I-06). The language is taken from the container metadata (on export) or from the file name (in the automatic search, for example `movie.en.srt`); ISO 639-1 and 639-2 codes are treated as equal, as in I-54. When the language is unknown, nothing is pre-selected | P1 |

**Merging secondary subtitles (I-20).** The list is built from the main cues (F-44); secondary cues are not shown
as separate rows.

1. Each secondary cue is attached to **exactly one** main cue — the one with the largest overlap of the intervals
   `[start, end)`. On a tie — to the earlier main cue.
2. When a secondary cue overlaps no main cue (it falls into a pause), it is attached to the nearest main cue in time
   (the distance is the gap between the intervals), on a tie — to the earlier one. So no translation text is lost.
3. The secondary cues of one main cue go in the order of `start`, separated by a line break.
4. The merge is recalculated when a subtitle file changes; it uses the cue times shifted by the subtitle offsets of
   the history (3.4).

### 2.4. Playback and controls

| ID   | Requirement | Priority |
|------|-------------|----------|
| I-30 | Built-in full screen mode (`F`, double click) | P1 |
| I-31 | Volume and mute (mouse wheel, keys), remembered | P1 |
| I-33 | Seeking repeats while `←` / `→` are held (key auto-repeat) | P2 |
| I-34 | Seeking by dragging on the progress bar, a time preview on hover. While dragging — fast seeks to keyframes at most once per 100 ms (the bar and the time on the panel follow the mouse), on release — one exact seek; a click without dragging — an exact seek on release. Above the bar at the cursor — the time under it | P2 |
| I-69 | **The system does not turn off the display or sleep** during playback (on pause and without a file it may): Windows — `SetThreadExecutionState`, Linux — `org.freedesktop.ScreenSaver.Inhibit` (D-Bus), macOS — `IOPMAssertionCreateWithName`. With the libmpv render API the mpv option `stop-screensaver` has no effect | P1 |
| I-54 | The setting "preferred audio language"; by default it equals the studied language (I-55) until the user sets it separately. The audio track when a file is opened: 1) the track the user chose for this file (I-02, I-13); 2) otherwise — the first track in the preferred language (by the language tag in the container; ISO 639-1 and 639-2 codes are treated as equal: `en` = `eng`); 3) otherwise — the default track of the file, or the first track when none is marked as default. The automatic choice is not remembered as the user's choice | P1 |
| I-60 | On-screen messages (OSD): a short text in the corner of the video for ~1.5 s in response to actions without a visible result — volume, mute, audio track, subtitles on video, chapter, "next file" | P1 |
| I-72 | **Chapters on the progress bar:** the chapters of the file (mpv `chapter-list`) are marked on the bar with vertical marks (except a chapter at 0); the time popup above the bar shows the title of the chapter under the cursor (untitled — "Chapter N"). `PgUp` / `PgDn` during playback — previous / next chapter (like `add chapter` in mpv: at the start of a chapter — to the previous one, otherwise — to the start of the current one), the new chapter is shown in the OSD; without chapters — the OSD "No chapters" | P2 |
| I-73 | **Frame preview** on hover over the progress bar (an addition to I-34): above the time under the cursor — a frame 192 px wide. The frame is the nearest keyframe not later than the time, decoded by FFmpeg (`libavcodec`, scaling — `libswscale`) in a separate Qt image loading thread; the file is opened separately from mpv. The time is rounded down in steps of 1/400 of the duration (at least 1 s); recent frames are cached (up to 32 MB). About 20 ms per frame of 1080p H.264 | P3 |

**The audio track menu (I-02).** The "Audio track (A)" submenu of the video context menu (a right click, 2.2) lists
the audio tracks of the file; the current one is checked. There is no audio track button on the control panel. A
track line: number, language (native name and code, as in the settings, I-50), the title from the container, codec
and channels — for example `2. English — en · Commentary · AC3 5.1`. Without a language — "Track 2". A click
switches the track without stopping playback. The `A` key switches to the next track in a circle and shows it in
the OSD (I-60). A choice from the menu or by the key is the user's choice: it is saved for the file
(`audioTrackId`, I-13) while the file is in the history.

**Default hotkeys.** The layout is close to mpv. Keys act on key press; seeking repeats while a key is held
(I-33). Keys are recognised by their physical position, so they also work with a Russian or Ukrainian keyboard
layout. Keys cannot be reassigned.

| Key | Action | ID |
|-----|--------|----|
| `Space` | open the subtitle list; the video pauses | F-15, F-42 |
| `Ctrl+Space`, click on the video | play/pause without the subtitle list | F-11, F-15 |
| `←` / `→` | seek 5 s back / forward | F-15 |
| `↑` / `↓`, mouse wheel over the video | volume ±5% | I-31 |
| `M` | mute | I-31 |
| `A` | next audio track | I-02 |
| `S` | subtitles on video: on / off | I-22 |
| `PgUp` / `PgDn` | previous / next chapter (in the subtitle list — scrolling) | I-72 |
| right click on the video | context menu: subtitles on video, audio track | I-22, I-02 |
| `F`, double click on the video | full screen; `Esc` — leave it | I-30 |
| `Ctrl+O` | open a file | I-10 |
| `Ctrl+H` | to the start page (history) | I-58 |
| `Ctrl+,` | settings | I-50 |
| `N` | next file in the folder | I-16 |

In the subtitle list:

| Key | Action |
|-----|--------|
| `Space`, `Ctrl+Space`, `Esc` | close the list and continue playback (like the `✕` button, F-42) |
| `↑` `↓` `PgUp` `PgDn` `Home` `End`, mouse wheel | scroll the list (these keys and the wheel do not change the volume) |
| `C` | scroll to the current cue |
| `F`, `M`, `N`, `Ctrl+O`, `Ctrl+H`, `Ctrl+,` | as during playback (`N` — for example at the end of the video) |

Other keys do nothing in the list.

### 2.5. Dictionary and translation

| ID   | Requirement | Priority |
|------|-------------|----------|
| I-40 | Correct URL encoding of the word/phrase for the dictionary and the translator | P1 |
| I-42 | One dictionary — a URL template in the settings (Longman by default); it opens in the default browser, there is no built-in panel (WebView) | P1 |
| I-47 | One translator for the `G` button (F-51) — a URL template in the settings (Google Translate by default); it opens in the external browser | P1 |
| I-59 | Rules for the dictionary word (F-31): an apostrophe inside a word, the full set of separators. Description — below | P1 |

**The dictionary word (I-59).** A click on the cue text (main or secondary) opens the dictionary:

- **With a selection** (by the mouse, on button release) — the selected text is taken: spaces and line breaks are
  collapsed into one space, punctuation and quotes at the edges are trimmed.
- **Without a selection** — the word under the cursor is taken:
  - a word is a continuous sequence of letters and digits (of any Unicode script, including combining marks);
    everything else is a separator (`”`, `«`, `»`, `…`, `–` and so on);
  - an apostrophe (`'` or `’`) between two letters is part of the word: `don't`, `o'clock`, `rock'n'roll`;
  - the ending `'s` / `’s` is dropped: `John's` → `John`, `it's` → `it`;
  - a hyphen is a separator: a click on `well-known` gives `well` or `known`; a compound word is taken as a whole by
    selecting it;
  - a click on a separator or a space does not open the dictionary.
- The case is not changed. The word is URL-encoded (I-40).

The logic lives in the C++ core and is covered by tests (I-52, 3.4).

### 2.6. Other

| ID   | Requirement | Priority |
|------|-------------|----------|
| I-50 | Settings — a separate page, saved at once (no separate "Save" buttons). Description — below | P1 |
| I-51 | Portable builds for Windows, Linux and macOS by GitHub Actions, published in GitHub Releases (P-10) | P2 |
| I-52 | Automated tests for the subtitle logic (parsing, merging, finding the current cue, word selection) | P1 |
| I-74 | **A single application instance** per user: a file opened from the file manager or the command line while the player is running opens in the current window (the position of the current video is saved), the window comes to the front; a start without a file only activates the window. `QLocalServer` / `QLocalSocket` | P2 |
| I-75 | The size, position and state of the window (maximized / full screen) are remembered between runs; saved on close in `settings.json` (`window`). When the top edge of the saved window is on none of the screens — the default window | P2 |
| I-76 | **Series on the start page.** Description — 2.2 | P2 |
| I-83 | The application version in the window title next to the name: `AF Video Player 0.1.1`, with an open video — `movie.mkv — AF Video Player 0.1.1`. The last number of the version (build) is increased with every code change | P1 |
| I-84 | **The application icon:** a video screen with the play button above the two-column subtitle list (F-42, F-43) — the translation on the left (green), the cue time, the original on the right, the current cue in the centre highlighted. The executable and the window on Windows (`.ico`), the `.app` bundle on macOS (`.icns`), the window and the `.desktop` file on Linux (PNG, SVG). The source is `packaging/icons/af-video-player.svg`; the other files are generated from it by `scripts/make-icons.ps1` | P1 |

**The settings page (I-50)** is a full-window page in the same window over the player, with the sections centred
and a `✕` close button in the top right corner (and `Esc`). It is opened by the ⚙ button on the start page and
from the player (the ⚙ button on the control panel and under the `✕` button of the subtitle list, `Ctrl+,`); the
subtitle list stays open under the settings. When the video was playing, it is paused and continues after the
settings are closed; a paused video stays paused. While the settings are open, the mouse (clicks, the wheel) does
not act on the player below them.
Changes apply and are saved at once; an invalid value is highlighted and not saved. Sections (fields — see
`settings.json`, 3.4):

- **Languages:** studied, native, preferred audio language (the default option is "Same as studied", I-54).
  A choice from the list of ISO 639-1 languages with a filter by typing; a language is shown by its native name and
  code: "English — en", "Українська — uk".
- **Dictionary** (I-42) and **Translator** (I-47): one URL template each and a "Reset to default" button. A template
  must contain `{text}` and no whitespace (leading and trailing whitespace is ignored).
- **Subtitles** (I-26): font sizes of the main and secondary subtitles in the subtitle list and, separately, over
  the video (I-22); 10–72 px, with a sample text next to them in the colours of the list / the video.
  Number fields here and in the history section accept digits only; an empty value is not applied.
- **History:** size (I-15), "Start watched files from the beginning" (I-76), "Clear history…" with a confirmation.
- **About:** version, the GPL-3.0-or-later license, a link to the source code, the versions of Qt, libmpv and
  FFmpeg.

---

## 3. Technology stack

### 3.1. The chosen stack

| Component | Technology | License |
|-----------|------------|---------|
| Core | C++20 | — |
| UI | Qt 6, QML (interface logic — JavaScript) | LGPLv3 / GPL |
| Playback | libmpv (render API, embedded through `QQuickFramebufferObject`) | GPLv2+ (or LGPLv2.1+) |
| Subtitle extraction, frame previews (I-73) | FFmpeg (`libavformat`, `libavcodec`, `libswscale`) | LGPLv2.1+ / GPL |

Why:

- **libmpv** — wide format support (FFmpeg inside), a simple API for audio and subtitle tracks (`track-list`,
  `aid`, `sid`), hardware decoding. Many players are built on it (Haruna, mpc-qt, Celluloid, IINA, mpv.net). System
  engines (Media Foundation, AVFoundation, QtMultimedia) depend on the codecs of the OS.
- **Qt 6 / QML** — the most proven UI stack for cross-platform players on mpv (Haruna, mpc-qt), with a standard way
  to embed video. QML is declarative, the interface logic is written in JavaScript.
- **C++** — ready examples of embedding libmpv, direct access to libav* for subtitle extraction, simpler packaging
  and a smaller distribution than with Python (PySide6).

UI requirements to be confirmed by a prototype:

- video + control panel over the video + a full-window list over the video;
- a long scrollable list (1000–2000 cues) with a jump to the current cue;
- finding the word under the cursor in the cue text (F-31) — by a text hit test or by splitting the cue into word
  elements;
- switching the audio track;
- subtitle extraction through FFmpeg;
- builds for three OSes.

### 3.2. Subtitle extraction (I-08)

Player engines give only the current cue, and the list needs all of them at once. So the application works only with
external subtitle files (P-12), and embedded text tracks are extracted through `libavformat` by the export command
(I-08). Exported files are then used as ordinary external subtitles, so no separate cache of extracted tracks is
needed.

**Measurement** (2026-10-14, ffmpeg 8.0.1, Windows 11, NVMe SSD). A synthetic "film": 2 hours, 2.2 GB, 24 fps, two
embedded text tracks of 1500 cues each. Both tracks were extracted in one pass:
`ffmpeg -i movie.mkv -map 0:s:0 en.srt -map 0:s:1 ru.srt`.

| Operation | Time |
|-----------|------|
| List of tracks (`ffprobe`) | ~0.07 s |
| MKV: extract both tracks in one pass | ~0.56 s |
| MKV: extract one track | ~0.56 s (the same — the time is set by the pass over the file) |
| MP4/MOV (mov_text): extract both tracks | ~0.12 s |

The file was in the OS cache during the measurement, so this is the time of parsing the container, without reading
from disk.

Conclusions for the implementation:

- **MKV** has no subtitle index: the demuxer goes through the whole file, the time ≈ the time of reading the file
  from disk. An estimate for 2–4 GB from a "cold" disk: NVMe — 1–3 s, SATA SSD — 4–8 s, HDD — 15–30 s.
- **MP4** stores a sample index, only the subtitle data is read — usually a fraction of a second.
- All needed tracks are extracted **in one pass**.
- Extraction runs **in the background, with progress** (by the read position in bytes, `AVIOContext`) and
  **cancellation** (interrupt callback); the video can be watched meanwhile.
- **Delivery size:** a full `ffmpeg.exe` build is ~210 MB, so the libav* libraries are used (a ready-made full
  build, not a custom minimal one). On Windows libmpv is usually built with FFmpeg linked in statically, so
  extraction needs a separate copy of the libraries; on Linux the shared system FFmpeg libraries are usually used.

### 3.3. Fallbacks

Considered only if the Qt prototype reveals serious problems.

| Option | Pros | Cons |
|--------|------|------|
| **Flutter + [media_kit](https://github.com/media-kit/media-kit)** | `media_kit` is built on libmpv and already handles tracks and subtitles; fast UI development | Dependency on one package with one main maintainer; subtitles would have to be extracted through the ffmpeg CLI (FFmpegKit was discontinued in 2025) |
| **C# + Avalonia + libmpv** | Strong typing, good tooling; FFmpeg.AutoGen for libav* | Embedding libmpv has to be done by hand (an OpenGL control) |

### 3.4. Technical decisions

Decisions on former open questions (2026-10-14).

**Storage of settings and history (I-14).** Two JSON files; the directories come from `QStandardPaths`:

| File | Qt location | Windows | Linux | macOS |
|------|-------------|---------|-------|-------|
| `settings.json` | `AppConfigLocation` | `%LOCALAPPDATA%\<app>` | `~/.config/<app>` | `~/Library/Preferences/<app>` |
| `history.json` | `AppLocalDataLocation` | `%LOCALAPPDATA%\<app>` | `~/.local/share/<app>` | `~/Library/Application Support/<app>` |

- The history is separate from the settings: it is rewritten often, the settings rarely.
- On Windows `%LOCALAPPDATA%` is used, not the roaming `%APPDATA%`: the history stores absolute paths, which make
  no sense on another computer.
- Writing is atomic (`QSaveFile`: a temporary file + rename) to reduce the risk of a damaged file when writing is
  interrupted.
- Each file has a `version` field (schema version) for migrations. Unknown fields are ignored on reading, missing
  ones get default values — so new settings (I-22, I-75 and others) are added without changing the version.
- Times are stored as integer milliseconds, paths are absolute, with `/` as the separator (as in Qt).

`settings.json`:

```json
{
  "version": 1,
  "studiedLanguage": "en",
  "nativeLanguage": "uk",
  "preferredAudioLanguage": null,
  "dictionaryUrl": "https://www.ldoceonline.com/dictionary/{text}",
  "translatorUrl": "https://translate.google.com/?sl={src}&tl={dst}&text={text}&op=translate",
  "mainSubtitleFontSize": 26,
  "secondarySubtitleFontSize": 20,
  "volume": 100,
  "muted": false,
  "showSubtitleOverlay": false,
  "mainOverlayFontSize": 34,
  "secondaryOverlayFontSize": 30,
  "historySize": 100,
  "restartWatched": true,
  "lastOpenFile": null,
  "window": {"x": 141, "y": 130, "width": 1280, "height": 760, "maximized": false, "fullScreen": false}
}
```

- `studiedLanguage`, `nativeLanguage` (I-55) are also used as `{src}` and `{dst}` in the URL templates.
- `preferredAudioLanguage` (I-54): `null` — equal to the studied language.
- `dictionaryUrl` (I-42), `translatorUrl` (I-47): the URL templates of the dictionary (a click on a word) and the
  translator (the `G` button). The substitutions `{text}`, `{src}`, `{dst}` are URL-encoded (I-40). An invalid
  template (without `{text}` or with whitespace) is replaced with the default one.
- `mainSubtitleFontSize`, `secondarySubtitleFontSize` — font sizes of the subtitle list in pixels (I-26), 10–72,
  26 and 20 by default.
- `showSubtitleOverlay` — subtitles on video (I-22); `mainOverlayFontSize`, `secondaryOverlayFontSize` — their
  font sizes, 10–72, 34 and 30 by default.
- `volume`, `muted` — I-31; `historySize` — I-15 (1–1000, 100 by default); `restartWatched` — a watched file opens
  from the beginning (I-76, `true` by default); `lastOpenFile` — the last opened video, the start of the file open dialog (I-86).
- `window` — the window at the last close (I-75): the position and size of the normal (not maximized) window in
  logical pixels and its state. `null` until the first close; a size smaller than 640×400 is invalid.

`history.json`:

```json
{
  "version": 1,
  "files": [
    {
      "path": "D:/Films/Movie/movie.mkv",
      "size": 2361393152,
      "lastOpened": "2026-10-14T09:30:00Z",
      "durationMs": 7200000,
      "positionMs": 1234567,
      "audioTrackId": 2,
      "mainSubtitles": "D:/Films/Movie/movie.en.srt",
      "secondarySubtitles": "D:/Films/Movie/movie.uk.srt",
      "subtitlesChosen": true,
      "mainOffsetMs": 0,
      "secondaryOffsetMs": 0
    }
  ]
}
```

- Entries are ordered by `lastOpened` (newest first). Entries beyond `historySize` are removed, the oldest first.
- An entry is found by `path` and `size`, for a moved file — by the name and `size` (I-12). Entries of missing
  files are kept but not shown on the start page (I-56).
- `audioTrackId` — the mpv track number (`aid`), only when the user chose it (I-02); `null` — the automatic choice
  by I-54.
- `mainSubtitles`, `secondarySubtitles` — the subtitle files chosen by the user, or `null` (I-13);
  `subtitlesChosen` — the user chose the subtitles of the video (or the automatic export assigned them, I-85), the
  automatic search (I-06) does not apply.
- `mainOffsetMs`, `secondaryOffsetMs` — subtitle offsets, applied when the subtitles are loaded; the interface does
  not set them, so they are 0.
- `durationMs` — to show the progress in the history list.
- The position is saved every 10 s during playback and also on pause, on a change of file and when the application
  closes (I-14). At the next opening playback continues from the saved position; a file played to 95% or further
  starts from the beginning by default (`restartWatched`, I-76).

**Subtitle parsers (I-04).** Own C++ implementation in the core, without third-party libraries: the formats are
simple, and own code is easy to cover with tests (I-52). The parsers are tolerant to errors: a broken cue is
skipped and does not stop parsing of the whole file. The result is a list of cues (`start`, `end`, text) sorted by
`start`.

- **SRT:** blocks separated by an empty line; the block number is optional; time `HH:MM:SS,mmm` (`.` is accepted).
  The tags `<i>`, `<b>`, `<u>`, `<font …>` and ASS tags like `{\an8}` are removed.
- **VTT:** the `WEBVTT` header; `NOTE`, `STYLE`, `REGION` blocks are skipped; time `MM:SS.mmm` or `HH:MM:SS.mmm`,
  cue settings after the time are ignored. The tags `<i>`, `<b>`, `<u>`, `<c.…>`, `<v …>`, `<lang …>`,
  `<ruby>`/`<rt>`, timestamps `<00:01.000>` are removed; entities are decoded (`&amp;`, `&lt;`, `&gt;`,
  `&nbsp;` …).
- **ASS/SSA:** the `[Events]` section, the field order — by the `Format:` line, the text is the last field (it may
  contain commas). `Comment:` lines are skipped. `{…}` blocks are removed; `\N` and `\n` → a line break, `\h` → a
  space. Text in the drawing mode (`\p1` … `\p0`) is dropped.
- **The encoding** is detected automatically, there is no setting: BOM → UTF-8 / UTF-16; without a BOM and valid
  UTF-8 → UTF-8; otherwise — `windows-1251` or `windows-1252`: the file is decoded with both, and the result that
  looks more like text is chosen. Words with non-ASCII letters are scored one by one (+1 / −1): a Cyrillic word
  must contain no Latin letters ("Привет", not "cafй"), a Latin word — at most one non-ASCII letter more than its
  ASCII letters ("café", "été", not "Ïðèâåò"); undefined characters and C1 controls — −2. An encoding is chosen only
  when its score is positive and strictly greater than the other one. When both encodings give the same text (only
  quotes, dashes, `…` — they match), that text is taken. **When the encoding is not determined** (equal or
  non-positive scores), the file is treated as UTF-8: invalid sequences are replaced with `�`.

**Build and dependencies.** CMake, Qt ≥ 6.8 (`qt_add_qml_module`), C++20. No separate package manager: vcpkg has
no libmpv port, and building Qt and FFmpeg from source takes hours.

| OS | Compiler | Qt | libmpv and FFmpeg | Delivery (P-10) |
|----|----------|----|-------------------|-----------------|
| Windows | MSVC 2022, x64 | the official Qt installer (in CI — `aqtinstall`) | ready-made builds: libmpv — [shinchiro/mpv-winbuild-cmake](https://github.com/shinchiro/mpv-winbuild-cmake), FFmpeg shared — [BtbN/FFmpeg-Builds](https://github.com/BtbN/FFmpeg-Builds); downloaded by a script to `deps/` (not in git); the libmpv import library for MSVC is made from the `.def` | zip, `windeployqt` |
| Linux | GCC or Clang | distribution packages when the version is ≥ 6.8, otherwise the official installer / `aqtinstall` | system packages (`libmpv-dev`, `libavformat-dev` …), found through `pkg-config` | AppImage, `linuxdeploy` + the Qt plugin |
| macOS | Apple Clang, arm64 only | Homebrew or the official installer | Homebrew (`mpv`, `ffmpeg`), found through `pkg-config` | zip with `.app`, `macdeployqt` |

- Builds use the latest released Qt 6 version (patches of the 6.8 LTS become available only to commercial users
  over time); the minimum supported version is 6.8.
- CI and the release use the same GitHub runners. Linux — Ubuntu 22.04, the oldest available runner: the AppImage
  takes glibc from the system, and CI checks the same libmpv and FFmpeg versions as the release. macOS — macOS 15:
  the Homebrew libraries are built for the runner's macOS, so the `.app` needs macOS 15 or later.

**Unit tests (I-52).** Qt Test + CTest, no third-party frameworks; data-driven tests (`_data`) for sets of parsing
cases. Test subtitle files are in `tests/data`. Tests run in CI on all three OSes.

- The testable logic lives in the C++ core, including finding the word under the cursor (F-31): QML passes the cue
  text and the character position, the core returns the word. QML/JavaScript keeps only presentation logic.

### 3.5. Implementation notes

Decisions made while implementing version 1.0 (2026-10-14).

- **Dialog buttons** are set explicitly, not by the standard Qt buttons (`standardButtons`), so that all interface
  labels are translated the same way through `qsTr()` (P-13).
- **Colours and contrast** (2026-10-15). The goal is colours that contrast with their background in every state
  (hover, pressed, disabled, selected): text — at least 4.5:1, interface elements (progress bar, outlines, marks) —
  at least 3:1 by WCAG; for semi-transparent backgrounds over the video — over any frame below them (from black to
  white).
  - There is one light palette, set explicitly in `Main.qml` (the window `palette` — all elements and popups
    inherit it), independent of the system theme and accent colour; the colour scheme of the application is light
    (`QStyleHints::setColorScheme`). There is no dark theme.
  - Own states where the Fusion style breaks them: buttons on the dark control panel get a semi-transparent white
    background on hover and press instead of the light style one; "Continue" on the start page is white text on
    blue in every state.
  - Symbol icons on buttons (`▶`, `⏸`, `⏏`, `⚙`) use a font with monochrome glyphs (Segoe UI Symbol on Windows),
    otherwise coloured emoji are substituted, which do not take the text colour.
  - New colours are checked by a contrast calculation before a commit.
- **Qt:** local development — Qt 6.11.3 (MSVC 2022). In CI — Qt 6.10.3: `aqtinstall` 3.3 (the basis of
  `install-qt-action`) does not understand the new repository layout of Qt 6.11. The minimum version is still 6.8.
- **Windows builds:** libmpv — the shinchiro build (x86_64), FFmpeg — the BtbN `n8.1` LGPL shared build. Both are
  pinned by URL and SHA-256 in `scripts/fetch-deps-windows.ps1`; old builds are removed from GitHub over time, then
  they have to be updated. The delivery includes only the FFmpeg libraries in use (`avformat`, `avcodec`, `avutil`,
  `swscale` and their dependency `swresample`) and the MSVC runtime. The zip is about 121 MB.
- **libmpv:** the user's `mpv.conf` and scripts are not loaded; `keep-open=yes`, so at the end of a file the video
  stops on the last frame and the subtitle list opens.
- **Video embedding:** Qt Quick works through OpenGL (`QQuickFramebufferObject`), not through the default D3D11 /
  Metal / Vulkan.
