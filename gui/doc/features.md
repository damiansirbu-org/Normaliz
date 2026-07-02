# Normaliz GUI - Feature List

Status of the desktop GUI, tracked against parity with jNormaliz (the Java GUI
being replaced) and beyond. Delivered features run today; planned features form
the roadmap toward a complete replacement.

## Delivered

- Native desktop GUI (Qt6 Widgets, C++); single window: input / goals / output.
- Engine in process: links `libnormaliz` statically, calls `Cone<mpz_class>`
  directly - no `.in`/`.out` file round-trip.
- Input editor for Normaliz `.in` text, parsed by Normaliz's own parser
  (`readNormalizInput`): every input type works (cone, vertices, inequalities,
  equations, congruences, grading, ...).
- File menu: New (size dialog), Open, Close, Save, Save As, Print; the window
  title tracks the current file and unsaved changes.
- 21 computation goals (scrollable): Hilbert basis, extreme rays, support
  hyperplanes, module generators, degree-1 elements, maximal subspace, Hilbert
  and Ehrhart series, multiplicity, volume, lattice points, triangulation size,
  class group, grading, dehomogenization, rank, embedding dimension, recession
  rank, and is-pointed / is-Gorenstein / deg-1-extreme-rays tests.
- Edit menu (Undo/Redo/Cut/Copy/Paste/Select All) and Help menu (Normaliz
  website, manual, About).
- Backend selector: Local (default, embedded) and Cloud (WIP, shown disabled) -
  the distributed backend is planned; the choice is already visible in the UI.
- Computation runs off the GUI thread (QtConcurrent); UI stays responsive,
  status shows Computing / Ready / Error.
- Stop button cancels a running computation (nmz_interrupted); status bar shows
  elapsed time and memory usage.
- Toolbar: Algorithm (Default/Primal/Dual), Mode (Goals only / DefaultMode),
  Precision (Default / BigInt), mapped to ConeProperty flags.
- Tabbed panel: Output / Console (captured libnormaliz verbose log) / Options
  (thread count, font size).
- Exception-safe: `libnormaliz` errors are caught and shown as text, never crash.
- Arbitrary-precision arithmetic (GMP `mpz_class`).
- Neutral high-contrast theme; no console window (WIN32 subsystem).
- Cross-platform native installers, produced by CI:
  Windows NSIS setup.exe, Linux AppImage, macOS .dmg.
- Standalone packaging: all runtime libraries bundled (double-click runs).

## Planned (parity with jNormaliz + beyond)

Parity targets are drawn from the jNormaliz 1.7 feature inventory.

- Live-streaming Console (currently the verbose log is shown when the run ends).
- Output-file options (`.out/.gen/.inv/.typ/.cst`, triangulation, Stanley) - not
  applicable to the in-process engine.
- Full `libnormaliz` build (non-NAKED) to reach the rest of the engine:
  algebraic polyhedra (e-antic), integrals / weighted Ehrhart (CoCoALib),
  automorphism groups (nauty).
- Structured output: matrices in tables, series as text; export.
- NmzIntegrate: generalized Ehrhart series, quasipolynomial leading
  coefficient, Lebesgue integral (needs CoCoALib).

## Beyond jNormaliz

- In-process engine (no CLI/file round-trip): faster, no temporary files.
- Structured, typed result views instead of raw `.out` text.
- Single self-contained installer per OS.
