# Normaliz GUI

Desktop GUI for Normaliz, built as the `gui/` component of the Normaliz tree.
It links `libnormaliz` statically and calls the compute engine in process,
without `.in`/`.out` files.

Status: skeleton. Computes the Hilbert basis of the `2cone` example.

## Requirements

- MSYS2 / MINGW64
- `mingw-w64-x86_64-{toolchain,cmake,ninja,qt6-base,gmp,mpfr,flint}`
- `libnormaliz.a` built in `../source`

## Build

Engine (once):

    cd ../source
    cp ../install_scripts_opt/header_files_for_Makefile.classic/version.h libnormaliz/
    cp ../install_scripts_opt/header_files_for_Makefile.classic/nmz_config.h libnormaliz/
    mingw32-make -f Makefile.classic NAKED=yes -j$(nproc)

GUI:

    cd ../gui
    cmake -G Ninja -B build -DCMAKE_PREFIX_PATH=/mingw64
    cmake --build build

`TMP`/`TEMP` must point to a writable directory; g++ writes temporaries there.

## Run

    ./build/normaliz-gui.exe

## Structure

    CMakeLists.txt    links libnormaliz.a, Qt6::Widgets, Qt6::Concurrent, gmp, OpenMP
    src/main.cpp      entry point
    src/MainWindow.*  UI; runs Cone<mpz_class> in a worker thread

## Design

- Qt6 Widgets, C++14. `libnormaliz` linked statically.
- `Cone<mpz_class>` is called in process: `Type::cone` -> `compute(HilbertBasis)`
  -> `getHilbertBasis`. GMP arithmetic.
- `compute()` runs off the GUI thread via `QtConcurrent`; the UI updates on
  `QFutureWatcher::finished`.
- `libnormaliz` throws `NormalizException`; the worker catches all exceptions so
  none propagate into the Qt event loop.

## Limitations

- Input fixed to the `2cone` example; no input editor yet.
- `libnormaliz` built without optional libraries (NAKED): no algebraic polyhedra
  (e-antic), integrals (CoCoALib), or automorphism groups (nauty).
- Debug build; binary not stripped.
