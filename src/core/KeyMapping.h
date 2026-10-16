#pragma once

namespace afvp {

// Hotkeys work by physical key position (SPEC 2.4): with a Russian or Ukrainian layout Qt reports Cyrillic
// key codes, which are mapped to the Latin keys at the same position of a QWERTY / ЙЦУКЕН keyboard.
// Other keys are returned unchanged.
int physicalKey(int key);

} // namespace afvp
