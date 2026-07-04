# Normaliz GUI

Native desktop GUI for Normaliz, built as the `gui/` component of the Normaliz
tree. Links `libnormaliz` statically and calls the compute engine in process.

Status: full jNormaliz parity and beyond. Complete File/Edit/Help menus, an
`.in` editor parsed by Normaliz's own parser, 25 checkbox goals plus any
ConeProperty by name in the editor, the complete engine (nauty automorphisms,
e-antic algebraic polyhedra, CoCoALib integrals), full `.out`-style output
rendering, Run/Stop off-thread, and cross-platform installers
(Windows/Linux/macOS) produced by CI (`.github/workflows/gui-release.yml`).

## Documentation

- `doc/features.md` - delivered and planned features
- `doc/coverage.md` - parity checklist vs jNormaliz (what is covered / remaining)
- `doc/architecture.md` - components, engine call path, threading, packaging
- `doc/usage.md` - install, window, running a computation, build from source
- `CHANGELOG.md` - version history

## Build

MSYS2 / MINGW64. See `doc/usage.md` for the full steps.

    cmake -G Ninja -B build -DCMAKE_PREFIX_PATH=/mingw64
    cmake --build build
    ./build/normaliz-gui.exe
