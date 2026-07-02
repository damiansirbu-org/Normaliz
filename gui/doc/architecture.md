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

    .in text  ->  readNormalizInput<mpq_class>  ->  InputMap
              ->  Cone<mpz_class>(input)
              ->  ConeProperties (HilbertBasis / ExtremeRays / SupportHyperplanes)
              ->  cone.compute(props)
              ->  getHilbertBasis() / getExtremeRays() / getSupportHyperplanes()
              ->  text in the output pane

The editor text is parsed by Normaliz's own parser `readNormalizInput`
(`source/input.cpp`), the same one the CLI uses, so every input type is
supported (cone, vertices, inequalities, equations, congruences, grading, ...).
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

## Shared core (desktop and web)

The written project targets a distributed, API-first platform with a React web
UI over `libnormaliz`; the current deliverable is this desktop GUI. They are not
in conflict: desktop and web are two front ends over one engine and one
request/result contract.

To keep that path open at near-zero cost, the GUI separates a UI-agnostic engine
layer (build the input model, select `ConeProperty` goals, call `compute`, read
results) from the Qt widgets. Today that layer calls `libnormaliz` in process;
later the same request/result types can be serialized (JSON) and served by a
computation service, with the desktop acting as an offline client and,
optionally, a hybrid client that offloads large jobs to a remote backend.

## Current limitations

- `libnormaliz` is built NAKED (GMP only): no algebraic polyhedra (e-antic),
  integrals (CoCoALib), or automorphism groups (nauty).
- Goals cover a useful subset (Hilbert basis, extreme rays, support hyperplanes,
  Hilbert series, multiplicity); more `ConeProperty` targets remain.
- No run controls (algorithm/mode/precision), console/log, or cancel yet
  (further jNormaliz parity is the next milestone).
- `libnormaliz` is built with assertions on (NAKED, no `-DNDEBUG`): a malformed
  internal state can `abort()` the app instead of throwing; a parity/release
  build should define `NDEBUG`.
- Large results are rendered as a single text block (no paging/streaming yet).
