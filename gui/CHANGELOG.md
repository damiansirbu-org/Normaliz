# Changelog

All notable changes to the Normaliz GUI are documented here.
Format based on Keep a Changelog; versions track the GUI, not Normaliz.

## [0.6.1] - 2026-07-04

Polish and testability pass before the hand-over, after verifying the 0.6.0
release artifacts (CI green on all three platforms; the Windows installer
installs, starts on a clean PATH and uninstalls cleanly) and running a
GUI-vs-CLI parity matrix (GUI output byte-identical to `normaliz` on 11 inputs
spanning rational / algebraic / integral / automorphism goals and the 0.6.0
fixes).

### Added
- Hidden headless flags `--run <in> <out>` (goals from the .in) and
  `--rundefault <in> <out>` (DefaultMode): compute a file through the exact
  Compute-button worker path without a window, for automated GUI-vs-CLI parity
  testing (the same idea as the existing `--shot`).
- The About dialog shows the version (`NMZ_GUI_VERSION`, set from the CMake
  project version, kept in sync with the installer).

### Fixed
- A pathologically large result no longer risks freezing the window: the Output
  pane shows a bounded head with a note instead of pushing hundreds of MB
  through `setPlainText` (normal outputs are unaffected).
- New / Open / Close are refused while a computation is running (changing the
  input under the worker was a confusing workflow; jNormaliz disabled it too).

## [0.6.0] - 2026-07-04

Fixes from the full adversarial review (phd docs: Normaliz-GUI/review-report.md).

### Fixed
- CLI-parity for additional input: `add_inequalities` / `add_equations` /
  `add_cone` / `add_subspace` / `add_vertices` / inhomogeneous variants were
  silently ignored (the result differed from the CLI on the same `.in`); they
  are now stripped before the Cone is built and applied via `modifyCone` +
  recompute, exactly as `normaliz.cpp` does.
- Rendering now uses a unique per-run `QTemporaryDir` (system temp on all three
  platforms) instead of `TEMP`/`TMP` with a fixed file name: no more collisions
  between GUI instances, no orphaned files, and a working temp path on
  Linux/macOS where `TEMP` is typically unset.
- Side files written by Normaliz's `Output` (.tri/.tgn triangulation, .aut
  automorphisms, .fac face lattice, fusion files, ...) are appended to the
  Output pane instead of being stranded invisibly on disk.
- A Stop arriving after the computation finished no longer discards the results
  (the interrupt flag is cleared before rendering, as the CLI does before
  `write_files`); a Stop during input parsing is honored instead of being
  silently cleared by `Cone::compute`.
- `NotComputableException` now renders the available results with a note (CLI
  behavior: "Writing only available data") instead of showing only the error.
- The algebraic-goal filter asks the engine's own `check_Q_permissible` per
  property instead of a hand-kept whitelist that wrongly skipped
  `LatticePoints`, `Triangulation`, `Automorphisms`, `ModuleGenerators`,
  `EuclideanVolume`, ... on `number_field` input.
- Polynomial / numerical parameters are set on the algebraic path too (the CLI
  sets them for both cone types).
- The thread limit is restored to the engine default when the spinbox returns
  to 0 (set_thread_limit is sticky in the engine).
- Engine "ERROR: ..." lines (errorOutput/cerr) are captured into the Console
  tab; they were lost in a windowed app.
- Closing the window during a computation now asks, stops the engine and waits
  for the worker instead of tearing the process down under it.
- Print prints the visible Output/Console tab when it has content (jNormaliz
  printed the selected tab), not always the input.
- Stale UI text removed (the build HAS CoCoALib); the Help dialog documents
  typing any ConeProperty name in the editor.
- Build-instruction fixes: `NAKED=yes` removed from CMake messages and
  usage.md (it produces an ABI-mismatched archive); C++17 declared (Qt 6
  requirement); GUI compiles with the same optional-library defines as the
  archive (ODR); NSIS installer version aligned; windeployqt failures no
  longer swallowed by the deploy script; New-dialog sizes capped.

## [0.5.0] - 2026-07-04

Full engine access: request any goal by name, see the complete Normaliz output.

### Added
- The `.in` editor now honors computation goals typed by name, exactly like the
  CLI: `options.getToCompute()` is merged into the requested properties, so every
  one of libnormaliz's ~150 ConeProperties (triangulations, Stanley decomposition,
  quasi-polynomials, integer hull, automorphism variants, ...) is reachable without
  a dedicated checkbox. The checkboxes remain a convenience subset.
- Results are rendered with Normaliz's own Output writer - the same text as the
  `.out` file - so every computed property is shown, not only the checkbox goals.
  (Rendered via a temp file internally; the user's input still comes from the editor.)

### Removed
- The per-goal formatters (formatGoal / formatGoalRenf); the full-output renderer
  supersedes them and covers all properties uniformly, including the algebraic path.

## [0.4.0] - 2026-07-04

CoCoALib (integrals / weighted Ehrhart) enabled - the last optional library.

### Added
- `libnormaliz` now built with NMZ_COCOA on top of nauty + e-antic. CoCoALib is
  built statically into `<repo>/local` by `install_nmz_cocoa.sh` (the MSYS
  prerelease + patches on Windows, 0.99818 elsewhere) and linked into the GUI - no
  ABI define or headers needed, only the archive on the link line.
- Three goals: Integral (of a polynomial), virtual multiplicity, weighted Ehrhart
  series. They need a `polynomial` field in the input; the worker now applies it
  via setPolyParams()/setNumericalParams(). Verified: the integral of x1*x2*x3
  over the standard simplex is 1/120.
- CI builds CoCoALib per platform (MSYS2 needs diffutils for CoCoA's header script).

## [0.3.0] - 2026-07-03

e-antic (algebraic / real embedded number fields) enabled in the engine and build.

### Added
- `libnormaliz` now built with ENFNORMALIZ (e-antic) + FLINT, on top of nauty. The
  pinned FLINT 3.0.1 + e-antic 2.0.2 are built into `<repo>/local` by
  `install_nmz_flint.sh` + `install_nmz_e-antic.sh`; a newer system FLINT (MSYS2
  ships 3.5) is deliberately not used - its differing layout corrupts the archive.
- Algebraic computation verified on the engine: the dodecahedron over Q(sqrt5)
  yields lattice-normalized volume `-1056*a+2400`.
- GUI links e-antic (ABI-critical: it compiles with ENFNORMALIZ to match the
  archive's `renf_elem_class` layout). Dynamic e-antic on Windows (DLLs bundled by
  win-deploy.sh); static on Linux/macOS (nothing extra to bundle).
- CI builds the FLINT + e-antic chain per platform (MSYS2 / apt+scripts / brew+scripts).
- GUI algebraic-input path: `number_field` input auto-dispatches (on
  NumberFieldInputException, as the CLI does) to `Cone<renf_elem_class>` and
  computes the geometric goals (volume incl. the algebraic value, support
  hyperplanes, extreme rays, ...). Non-applicable goals are listed as skipped.
  Verified: the dodecahedron over Q(sqrt5) yields volume `-1056*a+2400`.

### Fixed
- `install_nmz_e-antic.sh` (MSYS path): removed a stale `cp` of a patch file deleted
  upstream; rename e-antic's `fmpz_poly_randtest_irreducible` to avoid a
  multiple-definition link error against FLINT; add `--disable-dependency-tracking`
  for MSYS2 automake; guard the empty "hide" restore.

## [0.2.0] - 2026-07-03

First step beyond the NAKED engine: optional-library computations begin with nauty.

### Added
- Automorphism-group computation goal (`ConeProperty::Automorphisms`): reports the
  group order and the generating permutations of the extreme rays. Requires
  `libnormaliz` built non-NAKED with nauty; verified on the R^3 positive orthant
  (order 6 = |S_3|).
- `libnormaliz` now built with nauty enabled (cocoa / e-antic / flint / hash-library
  still off). CMake links nauty; the GUI must link it because the archive references
  nauty symbols.
- CI builds nauty per platform: MSYS2 package on Windows; the canonical
  `install_scripts_opt/install_nmz_nauty.sh` (TLS build) into `source/local` on
  Linux and macOS. The Windows installer bundles `libnauty*.dll` via `win-deploy.sh`.

### Changed
- Toolbar Algorithm / Mode / Precision selectors now render the native drop-down
  arrow and a beveled button, so they read as dropdowns instead of flat fields.

## [0.1.0] - 2026-07-02

First working skeleton with cross-platform delivery.

### Added
- Qt6/C++ desktop GUI as the `gui/` component of the Normaliz tree.
- In-process engine call: links `libnormaliz.a` statically, computes via
  `Cone<mpz_class>` (Hilbert basis, extreme rays, support hyperplanes).
- Input editor wired to Normaliz's parser (`readNormalizInput`): computes on any
  `.in` input (cone, vertices, inequalities, equations, congruences, grading,
  ...), not just the built-in example.
- File menu: New (size dialog), Open, Save, Save As; the window title tracks the
  current file and unsaved changes.
- Unsaved-changes guard: New / Open / Exit prompt to Save / Discard / Cancel when
  the input was edited (no silent data loss).
- Computation goals: Hilbert series and multiplicity added (alongside Hilbert
  basis, extreme rays, support hyperplanes).
- More computation goals: volume, lattice points, class group.
- Edit menu (Undo/Redo/Cut/Copy/Paste/Select All) and Help menu (Normaliz
  website, manual, About).
- Stop button: cancels a running computation (nmz_interrupted); the result shows
  "Computation stopped".
- Status bar shows elapsed time during and after a computation.
- Toolbar with Algorithm (Primal/Dual), Mode (DefaultMode) and Precision
  (BigInt) selectors, mapped to ConeProperty flags.
- Tabbed panel: Output / Console / Options. The Console tab shows the captured
  libnormaliz verbose output; the Options tab has thread-count and font-size
  controls. Help gains a Mathematical-background dialog.
- File Close and Print; a Help usage dialog; a memory gauge in the status bar
  (Windows/Linux). This completes essentially the full jNormaliz feature set.
- Computation goals expanded to 21 via a goal table: module generators, degree-1
  elements, maximal subspace, Ehrhart series, triangulation size, grading,
  dehomogenization, rank, embedding dimension, recession rank, and the
  is-pointed / is-Gorenstein / deg-1-extreme-rays tests, in a scrollable list.
- Off-thread computation (QtConcurrent) with responsive UI and status line.
- Exception-safe worker: `NormalizException` shown as text, no crash.
- Neutral high-contrast theme; WIN32 GUI subsystem (no console window).
- Windows standalone packaging via `scripts/win-deploy.sh` (bundled DLLs).
- NSIS installer (`installer/normaliz-gui.nsi`).
- Cross-platform CI (`.github/workflows/gui-release.yml`): Windows setup.exe,
  Linux AppImage, macOS .dmg, uploaded as run artifacts.
- Backend selector in the UI: Local (default, embedded engine) and Cloud
  (WIP, shown but disabled) - placeholder for the planned distributed backend.
- Documentation: `doc/architecture.md`, `doc/usage.md`, `doc/features.md`.

### Known limitations
- `libnormaliz` built NAKED (GMP only): no e-antic / CoCoALib / nauty.
- No file operations, run controls, console/log, or cancel yet (jNormaliz parity
  is the next milestone).
