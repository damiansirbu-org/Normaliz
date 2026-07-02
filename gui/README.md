# Normaliz GUI

Native desktop GUI for Normaliz, built as the `gui/` component of the Normaliz
tree. Links `libnormaliz` statically and calls the compute engine in process.

Status: skeleton. Computes Hilbert basis / extreme rays / support hyperplanes
on the built-in 2cone example; cross-platform installers (Windows/Linux/macOS)
are produced by CI (`.github/workflows/gui-release.yml`).

## Documentation

- `doc/architecture.md` - components, engine call path, threading, packaging
- `doc/usage.md` - install, window, running a computation, build from source

## Build

MSYS2 / MINGW64. See `doc/usage.md` for the full steps.

    cmake -G Ninja -B build -DCMAKE_PREFIX_PATH=/mingw64
    cmake --build build
    ./build/normaliz-gui.exe
