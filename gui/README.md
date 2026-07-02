# Normaliz GUI

Native desktop GUI for Normaliz, built as the `gui/` component of the Normaliz
tree. Links `libnormaliz` statically and calls the compute engine in process.

Status: early. A File menu (New/Open/Save) plus an `.in` editor parsed by
Normaliz's own parser; computes Hilbert basis, extreme rays, support
hyperplanes, Hilbert series and multiplicity on any input. Cross-platform
installers (Windows/Linux/macOS) are produced by CI
(`.github/workflows/gui-release.yml`).

## Documentation

- `doc/features.md` - delivered and planned features
- `doc/architecture.md` - components, engine call path, threading, packaging
- `doc/usage.md` - install, window, running a computation, build from source
- `CHANGELOG.md` - version history

## Build

MSYS2 / MINGW64. See `doc/usage.md` for the full steps.

    cmake -G Ninja -B build -DCMAKE_PREFIX_PATH=/mingw64
    cmake --build build
    ./build/normaliz-gui.exe
