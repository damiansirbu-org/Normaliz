# Normaliz GUI - Architecture

The GUI is the `gui/` component of the Normaliz source tree. It is a native
desktop front end over the existing C++ engine: it links `libnormaliz`
statically and calls the compute API in process, with no `.in`/`.out` file
round-trip.

## Components

    gui/
      CMakeLists.txt              build; links libnormaliz.a + nauty + e-antic + CoCoA + FLINT + Qt6 + gmp (+ OpenMP)
      src/main.cpp                QApplication, global stylesheet (QSS), entry point
      src/MainWindow.{h,cpp}      the single window and the compute worker
      scripts/win-deploy.sh       bundles Qt + transitive MinGW DLLs next to the exe
      installer/normaliz-gui.nsi  NSIS installer script (Windows setup.exe)
      doc/                        this documentation

## Engine call path

The engine is called in process, not via the CLI:

    .in text  ->  readNormalizInput<mpq_class>  ->  InputMap
              |   (NumberFieldInputException -> re-parse as renf_elem_class)
              ->  Cone<mpz_class> or Cone<renf_elem_class>
              ->  ConeProperties (checkboxes + goals typed in the editor)
              ->  cone.compute(props)   [+ modifyCone for add_* input, as the CLI]
              ->  Output::write_files into a per-run QTemporaryDir
              ->  .out text (+ side files: .tri, .aut, ...) in the output pane

The editor text is parsed by Normaliz's own parser `readNormalizInput`
(`source/input.cpp`), the same one the CLI uses, so every input type is
supported (cone, vertices, inequalities, equations, congruences, grading, ...).
Rational input computes on GMP (`mpz_class`): arbitrary precision, no overflow.
A `number_field` input dispatches to `Cone<renf_elem_class>` (e-antic), exactly
as `normaliz.cpp` does. The public API is `Cone<Integer>` in
`source/libnormaliz/cone.h`; computation goals are the `ConeProperty` enum in
`source/libnormaliz/cone_property.h`. Results are rendered with Normaliz's own
`Output` class into a temporary directory that is unique per run and removed
afterwards; the `.out` text plus any side files are shown in the output pane.

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

Each job builds the optional-library chain (nauty, pinned FLINT 3.0.1 +
e-antic 2.0.2, CoCoALib) into `<repo>/local`, builds the full `libnormaliz.a`,
builds the GUI, packages the native installer, and uploads it as a run artifact.

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

## Engine coverage

The GUI exposes 25 computation goals as checkboxes through a goal table in
`MainWindow.cpp` (adding one is a one-line change). Beyond the checkboxes, any
of libnormaliz's ~150 `ConeProperty` goals can be requested by typing its name
in the `.in` editor, exactly like the CLI; the full Normaliz output (all
computed properties, including side files such as the triangulation) is shown.

`libnormaliz` is built complete (non-NAKED): algebraic polyhedra (e-antic),
integrals / weighted Ehrhart (CoCoALib) and automorphism groups (nauty) are
all available. On algebraic (`number_field`) input, the goals the engine does
not support on renf are skipped with a note (the applicability test is the
engine's own `check_Q_permissible`).

## Current limitations

- Results are text (the `.out` rendering), not structured tables.
- The Console tab fills when the run ends (no live streaming yet).
- Large results are rendered as a single text block (no paging/streaming yet).
