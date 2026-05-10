
# Simple UI library in C.

This is a simple ui library made completely in C the only thirdparty that is required is
either windows or X11/Xwayland.

Dependencies:
- stb_truetype.h

Features:
- cross-platform
- labels
- buttons
- checkboxes
- dropdown boxes
- immediate ui: imperative instead of pool
- simple and minimal:
   306 LOC simpleui.c
   284 LOC simpleui_win32.c
   435 LOC simpleui_x11.c
  (5079 LOC stb_truetype.h)

Link flags:
- Linux X11: `-lX11 -lm`
- Windows: `-luser32 -lgdi32 -winmm`

