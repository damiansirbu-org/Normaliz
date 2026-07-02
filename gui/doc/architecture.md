# Normaliz GUI - Architecture

The GUI is the `gui/` component of the Normaliz source tree. It is a native
desktop front end over the existing C++ engine: it links `libnormaliz`
statically and calls the compute API in process, with no `.in`/`.out` file
round-trip.

## Components

    gui/
      CMakeLists.txt              build; links libnormaliz.a + Qt6 Widgets/Concurrent + gmp (+ OpenMP)
      src/main.cpp                QApplication, global stylesheet (QSS), entry point
      src/MainWindow.{h,cpp}      the single window and the compute worker
      scripts/win-deploy.sh       bundles Qt + transitive MinGW DLLs next to the exe
      installer/normaliz-gui.nsi  NSIS installer script (Windows setup.exe)
      doc/                        this documentation

## Engine call path

The engine is called in process, not via the CLI:

    input matrix  ->  Cone<mpz_class>(Type::cone, rows)
                  ->  ConeProperties (HilbertBasis / ExtremeRays / SupportHyperplanes)
                  ->  cone.compute(props)
                  ->  getHilbertBasis() / getExtremeRays() / getSupportHyperplanes()
                  ->  text in the output pane

Arithmetic is GMP (`mpz_class`): arbitrary precision, no overflow. The public
API is `Cone<Integer>` in `source/libnormaliz/cone.h`; computation goals are the
`ConeProperty` enum in `source/libnormaliz/cone_property.h`.

## Threading

`compute()` can run for a long time. It runs off the GUI thread via
`QtConcurrent::run`; the UI updates on `QFutureWatcher::finished`. The Compute
button is disabled while a computation is in flight, so the window never blocks.

## Error handling

`libnormaliz` throws `NormalizException` (bad input, missing optional library,
not-computable, arithmetic). Qt forbids exceptions escaping into the event loop.
The worker (`runCompute`) catches every exception and returns a plain result
struct; errors are shown as text in the output pane, never as a crash.

## Build and packaging

Local build: MSYS2 / MINGW64, `cmake -G Ninja` + `cmake --build`. A post-build
step runs `scripts/win-deploy.sh`, which calls `windeployqt` and copies every
transitive MinGW DLL next to the exe, so the binary runs standalone (double
click) without MSYS2 on PATH. On Windows the target is built with the WIN32
(GUI) subsystem, so no console window opens.

Cross-platform packaging is automated in `.github/workflows/gui-release.yml`,
one job per OS:

    Windows   NSIS setup.exe   (DLLs bundled via win-deploy.sh)
    Linux     AppImage         (Qt + libs bundled via linuxdeploy)
    macOS     .dmg             (frameworks bundled via macdeployqt)

Each job builds `libnormaliz.a` (static, NAKED), builds the GUI, packages the
native installer, and uploads it as a run artifact.

## Current limitations

- Input is fixed to the built-in 2cone example; the `.in` editor is not yet
  wired to the engine (planned: reuse `readNormalizInput` from
  `source/input.cpp`).
- `libnormaliz` is built NAKED (GMP only): no algebraic polyhedra (e-antic),
  integrals (CoCoALib), or automorphism groups (nauty).
- Goals limited to Hilbert basis, extreme rays, support hyperplanes.
