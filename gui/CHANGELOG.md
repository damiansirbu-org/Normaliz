# Changelog

All notable changes to the Normaliz GUI are documented here.
Format based on Keep a Changelog; versions track the GUI, not Normaliz.

## [0.1.0] - 2026-07-02

First working skeleton with cross-platform delivery.

### Added
- Qt6/C++ desktop GUI as the `gui/` component of the Normaliz tree.
- In-process engine call: links `libnormaliz.a` statically, computes via
  `Cone<mpz_class>` (Hilbert basis, extreme rays, support hyperplanes).
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
- Input fixed to the built-in 2cone example (editor not yet wired to parser).
- `libnormaliz` built NAKED (GMP only): no e-antic / CoCoALib / nauty.
