# Zion

A small freeform text canvas in C and raylib. Write snippets, place them on an
open canvas, and pan around. The original type-then-place interaction is preserved.

Build with `make`, then run `./zion` from the project folder. Requires a C99
compiler, make, raylib, and its Linux desktop libraries (OpenGL and X11).
Verified with raylib 6.0. The canvas uses cream paper colors and charcoal ink.
The bundled JetBrains Mono NL Medium font (`JBM.ttf`) loads beside the executable.

| Control | Action |
| --- | --- |
| Click empty canvas | Start typing; the draft follows the pointer |
| Click again | Place the draft |
| Click any part of a note | Select it and append text |
| Shift+click another note | Select more notes; typing edits all selected notes |
| Click empty canvas while selected | Deselect |
| Enter / Backspace | Add a line / erase the last character |
| Delete | Remove selected notes |
| Escape | Cancel the draft or clear selection |
| Hold middle mouse and drag | Pan |
| Ctrl+S | Save now; place any draft at the pointer |

Placed notes and edits save automatically, at most once per second. Closing the
window also places any unfinished draft at the pointer and saves. A failed save
keeps changes open and displays an error so you can fix the folder permissions or
disk space and retry.

The canvas lives in `data.zn` and `index.zn` in the **working directory**. Keep both
files together when backing up or moving a canvas, and open only one instance per
canvas. Records use the original native binary format and are not intended for
transfer between different architectures. Unreadable saved files stop startup
with an error instead of being overwritten. Text is stored as UTF-8, but the
current font load uses a limited glyph set. Editing remains append/backspace;
there is no text cursor, undo, or relationship UI in this cleanup.

Run `make test` for headless regression checks with UndefinedBehaviorSanitizer.
Tests use a temporary folder and do not touch your canvas. If the AddressSanitizer
runtime is installed, also run:

```sh
make -B test TEST_CFLAGS='-g -fsanitize=address,undefined -fno-omit-frame-pointer'
```
