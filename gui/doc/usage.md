# Normaliz GUI - Usage

## Install

Download the native package for your OS from the CI run artifacts
(`gui-release.yml`) and install it:

    Windows   normaliz-gui-...-setup.exe   double click, follow the wizard
    Linux     normaliz-gui-....AppImage    chmod +x, then run
    macOS     normaliz-gui-....dmg         open, drag to Applications

The package is self-contained; no separate Qt or GMP install is required.

## Window

One window, three areas plus a status bar:

    Input (.in)          text editor for the Normaliz input
    Computation goals    checkboxes: Hilbert basis, Extreme rays, Support hyperplanes
    Output               results (matrices as text)

## Run a computation

1. Edit the input (the 2cone example is preloaded as a starting point).
2. Tick the goals you want.
3. Press Compute.

The editor text is parsed by Normaliz's own parser, so any input type works
(cone, vertices, inequalities, equations, congruences, grading, ...). The
computation runs in the background; the status bar shows Computing... then Ready.
Results appear in the Output area. On invalid input the status bar shows Error
and the message is printed in Output (the app does not crash).

## Build from source

MSYS2 / MINGW64, packages:
`mingw-w64-x86_64-{toolchain,cmake,ninja,qt6-base,gmp,mpfr,nauty,boost}` plus
`make` and `diffutils`.

The GUI compiles with `ENFNORMALIZ`, so `libnormaliz.a` MUST be the full build
(nauty + e-antic + CoCoALib) - a NAKED archive has a different `Cone` layout
and does not link.

    # 1. optional libraries into <repo>/local (once), from repo root
    export OSTYPE=msys NMZ_PREFIX="$PWD/local"
    ./install_scripts_opt/install_nmz_flint.sh      # pinned FLINT 3.0.1
    ./install_scripts_opt/install_nmz_e-antic.sh    # e-antic 2.0.2
    ./install_scripts_opt/install_nmz_cocoa.sh      # CoCoALib

    # 2. engine (once)
    cd source
    cp ../install_scripts_opt/header_files_for_Makefile.classic/version.h libnormaliz/
    cp ../install_scripts_opt/header_files_for_Makefile.classic/nmz_config.h libnormaliz/
    mingw32-make -f Makefile.classic HASHLIBRARY=no lib -j$(nproc)

    # 3. gui
    cd ../gui
    cmake -G Ninja -B build -DCMAKE_PREFIX_PATH=/mingw64
    cmake --build build
    ./build/normaliz-gui.exe

Running from the build tree needs the e-antic DLLs on PATH (`<repo>/local/bin`);
the post-build deploy step copies them next to the exe, so a double-click on
`build/normaliz-gui.exe` works too.
