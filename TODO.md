# TODO

> What is still to be done or can be done. What and why is in [SPEC.md](SPEC.md); here is the list of work with
> references to its items (`I-xx`, `P-xx`). Version 1.0 (all P1 requirements) is implemented.

The order of work is free. Each change leaves the project working: it builds and the tests pass; new core logic is
covered by tests (I-52).

## 1. To verify

Done but not verified.

- [ ] CI and the release through GitHub Actions (`.github/workflows`) — never run, a push is needed.
- [ ] The build and packages for Linux (AppImage) and macOS (`.app`) — the scripts are only written. The I-69 code
  (D-Bus on Linux, IOKit on macOS) has not been compiled.
- [ ] Sleep prevention (I-69) on Linux and macOS; a single instance (I-74) on Linux and macOS.
- [ ] The application icon (I-84) on Linux (window, AppImage) and macOS (`.app`, Dock).
- [ ] Start, playback and export on clean systems of all three OSes.
- [ ] Manually: opening the dictionary and the translator in the browser (URL building is covered by tests),
  drag & drop, full screen, cancelling an export from the interface (cancellation is covered by a test).
- [ ] Manually: the automatic subtitle search when a video is opened (I-06) and the list of found files in the
  "Subtitles" dialog.
- [ ] Manually: finding a moved file in the history (I-12); the `N` key and the "Next" button at the end of the
  video (I-16).
- [ ] Manually (0.1.2): subtitles on video (I-22) and the video context menu, the panel does not hide under the
  cursor, the settings button in the player with pause and resume, the outline of a hovered history entry.
- [ ] Manually (0.1.4–0.1.8): dragging on the progress bar and the time under the cursor (I-34), chapters on the bar
  and `PgUp`/`PgDn` on a file with chapters (I-72), the frame preview (I-73), series on the start page and
  "Continue" (I-76), removing history entries (I-15).
- [ ] Manually (0.1.15): a subtitle file dropped onto the start page shows a message (I-05); an export cancelled
  after "Replace" keeps the existing files; an export finished after another video was opened offers nothing.
- [ ] Seeking repeats while `←`/`→` are held (I-33): there is no separate code, it should work through key
  auto-repeat.

## 2. Build and release

- [ ] Update the README before the first release: installation, first steps, SmartScreen / Gatekeeper warnings.
- [ ] Publish release 1.0 with three archives (P-10, I-51).

## 3. Approved requirements (P2, P3)

Everything approved is done.
