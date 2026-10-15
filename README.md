# Bank Management System - Qt GUI

Qt Widgets front end for the original console bank program.

## Open, build, run (Qt Creator, Windows)
1. File > Open File or Project... > select `BankGUI.pro` (or `CMakeLists.txt`).
2. Pick a Qt 5.15+ or Qt 6 Desktop kit (MinGW or MSVC) and click Configure Project.
3. Build (Ctrl+B) and Run (Ctrl+R).

## Where Bank.txt goes
The backend opens `Bank.txt` by relative name, exactly as before. At start-up the app
looks for an existing `Bank.txt` in: the working directory, then the .exe folder and up to
4 parent folders (plus their direct sub-folders, which covers the project folder next to the build folder). Easiest: put your `Bank.txt` next to
`BankGUI.pro`. The path in use is shown at the bottom of the sidebar. If none exists,
it is created in the working directory when the first client is added.

## Files
- `bank_project.cpp` - your original backend (see `backend_changes.diff`)
- `bank_backend.h` - `sClients` (moved here unchanged) + declarations of the GUI-callable functions
- `main.cpp`, `mainwindow.h`, `mainwindow.cpp` - the GUI (no .ui file; widgets are built in code)
- `BankGUI.pro` / `CMakeLists.txt` - project files (both define `BANK_GUI`)

To build the original console program again, compile `bank_project.cpp` without `BANK_GUI`.
