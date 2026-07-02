# Normaliz GUI - Feature List

Status of the desktop GUI, tracked against parity with jNormaliz (the Java GUI
being replaced) and beyond. Delivered features run today; planned features form
the roadmap toward a complete replacement.

## Delivered

- Native desktop GUI (Qt6 Widgets, C++); single window: input / goals / output.
- Engine in process: links `libnormaliz` statically, calls `Cone<mpz_class>`
  directly - no `.in`/`.out` file round-trip.
- Input editor for Normaliz `.in` text.
- Computation goals: Hilbert basis, extreme rays, support hyperplanes.
- Backend selector: Local (default, embedded) and Remote (shown, disabled) - the
  distributed backend is planned; the choice is already visible in the UI.
- Computation runs off the GUI thread (QtConcurrent); UI stays responsive,
  Compute disabled while running, status shows Computing / Ready / Error.
- Exception-safe: `libnormaliz` errors are caught and shown as text, never crash.
- Arbitrary-precision arithmetic (GMP `mpz_class`).
- Neutral high-contrast theme; no console window (WIN32 subsystem).
- Cross-platform native installers, produced by CI:
  Windows NSIS setup.exe, Linux AppImage, macOS .dmg.
- Standalone packaging: all runtime libraries bundled (double-click runs).

## Planned (parity with jNormaliz + beyond)

Parity targets are drawn from the jNormaliz 1.7 feature inventory.

- Wire the `.in` editor to the engine (`readNormalizInput`): compute on any
  input, not just the built-in example.
- File operations: Open, New (rows/cols dialog), Save, Save as, Print, Close.
- Run controls: Algorithm, Computational mode, Precision selectors; Stop/cancel
  with progress (`nmz_interrupted`).
- Console/log tab: real-time verbose engine output.
- Output options: `.out/.gen/.inv/.typ/.cst`, triangulation, Stanley
  decomposition; thread control; font size.
- Full computation goals (all `ConeProperty`): Hilbert/Ehrhart series,
  multiplicity, volume, lattice points, class group, and more.
- Structured output: matrices in tables, series as text; export.
- NmzIntegrate: generalized Ehrhart series, quasipolynomial leading
  coefficient, Lebesgue integral.
- Full `libnormaliz` build (non-NAKED): algebraic polyhedra (e-antic),
  integrals (CoCoALib), automorphism groups (nauty).
- Help: manual, website, mathematical background, about.

## Beyond jNormaliz

- In-process engine (no CLI/file round-trip): faster, no temporary files.
- Structured, typed result views instead of raw `.out` text.
- Single self-contained installer per OS.
