# CG-Mini-Project
This repo contains all the files used in CG Mini Project on topic "atom simulation"

Bohr-model viewer for **36 elements (Hydrogen to Krypton)** with up to **4 electron shells (K, L, M, N)**,
nucleus built from individual protons and neutrons, and live facts for each element.

The same `atomsimu.c` works on macOS, Linux and Windows (no separate Windows file is needed).

# How to run


## macOS
1. Install Xcode command line tools: `xcode-select --install` (GLUT and OpenGL ship with macOS)
2. Build: `make`
3. Run: `./atomsimu` (or `make run`)

Without make: `clang atomsimu.c -Wno-deprecated-declarations -framework GLUT -framework OpenGL -lm -o atomsimu`

## Linux
1. Install the tools (Ubuntu/Debian): `sudo apt install build-essential freeglut3-dev`
2. Build and run: `make run`

Without make: `gcc atomsimu.c -lGL -lGLU -lglut -lm -o atomsimu && ./atomsimu`

## Windows (MSYS2 + MinGW, recommended)
1. Install MSYS2 from https://www.msys2.org and open the **MSYS2 UCRT64** terminal.
2. Install the compiler, make and freeglut:
   `pacman -S --needed mingw-w64-ucrt-x86_64-gcc make mingw-w64-ucrt-x86_64-freeglut`
3. Go to the project folder (your C: drive is `/c/`), for example: `cd /c/Users/<you>/CG-Mini-Project`
4. Build: `make`
5. Run: `./atomsimu.exe` (or `make run`)

Without make:
`gcc atomsimu.c -o atomsimu.exe -lfreeglut -lopengl32 -lglu32 -lm`

If the program says `freeglut.dll` is missing when you double-click the .exe, run it from the MSYS2 UCRT64 terminal,
or copy `freeglut.dll` (from `C:\msys64\ucrt64\bin`) next to `atomsimu.exe`.

### Windows (Visual Studio alternative)
1. Install freeglut with vcpkg: `vcpkg install freeglut:x64-windows`, then `vcpkg integrate install`
2. Create an empty C++ Console project in Visual Studio, add `atomsimu.c`, and build (x64).
3. Run with Ctrl+F5.

# Controls
- Enter: continue from the title screen
- Right click: menu (elements grouped by period, start/stop, labels, home, exit)
- Left / Right arrow keys: previous / next element
- Space: start / stop rotation (left click also starts), `s`: stop
- `+` / `-`: electron speed, `l`: toggle labels, `b`: home screen, `q`: quit
- Esc: reset window size, F10: full screen
