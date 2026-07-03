cmake_minimum_required(VERSION 3.16)
project(normaliz-gui CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent PrintSupport)
# Optional: libnormaliz uses OpenMP on Windows/Linux, but on macOS (Apple clang
# has no -fopenmp) it is built without it, so the GUI must not require it either.
find_package(OpenMP)

# This GUI lives inside the Normaliz tree (Normaliz/gui). The engine headers and
# the static libnormaliz.a are the sibling source/ directory.
# Build the library first: cd ../source && mingw32-make -f Makefile.classic NAKED=yes
set(NORMALIZ_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../source" CACHE PATH "Normaliz source dir")

# libnormaliz.a on Windows/Linux, libnormaliz.so/.dylib on macOS (no static link there).
find_library(NORMALIZ_LIB NAMES normaliz PATHS "${NORMALIZ_SRC}/libnormaliz" NO_DEFAULT_PATH)
if(NOT NORMALIZ_LIB)
    message(FATAL_ERROR
        "libnormaliz not found in ${NORMALIZ_SRC}/libnormaliz.\n"
        "Build it first (in ../source): make -f Makefile.classic NAKED=yes")
endif()

# WIN32 builds a GUI-subsystem app on Windows (no console window; Qt provides the
# WinMain entry point). MACOSX_BUNDLE builds a .app on macOS. Both ignored elsewhere.
add_executable(normaliz-gui WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/MainWindow.cpp
)

# Match the optional-library defines libnormaliz.a was built with. ENFNORMALIZ is
# ABI-critical: with it, general.h maps renf_elem_class to eantic's class; without,
# to `long`. Cone<Integer> has a renf_elem_class member, so the GUI MUST compile
# with the same define as the archive or the object layout mismatches.
target_compile_definitions(normaliz-gui PRIVATE NMZ_MAKEFILE_CLASSIC ENFNORMALIZ NMZ_FLINT)

# <repo>/local/include first: it holds e-antic's headers and the pinned FLINT 3.0.1
# headers, which must win over any newer system FLINT.
target_include_directories(normaliz-gui PRIVATE
    "${NORMALIZ_SRC}/../local/include"
    "${NORMALIZ_SRC}")

# nauty provides the graph-automorphism engine. libnormaliz (built non-NAKED with
# NMZ_NAUTY) references nauty symbols, so the GUI must link it too.
# Windows/MSYS2: nauty is a system package under CMAKE_PREFIX_PATH (/mingw64).
# Linux/macOS CI: nauty is built by install_scripts_opt/install_nmz_nauty.sh into
# <repo>/local (= OPT_LIB_ROOT = source/../local, where the classic Makefile links
# from). NORMALIZ_SRC is <repo>/source, so its parent is the repo root.
find_library(NAUTY_LIB NAMES nauty
    PATHS "${CMAKE_PREFIX_PATH}/lib" "${NORMALIZ_SRC}/../local/lib")
if(NOT NAUTY_LIB)
    message(FATAL_ERROR
        "nauty not found. Install it (MSYS2: pacman -S mingw-w64-x86_64-nauty; "
        "Debian: libnauty2-dev; brew: nauty) or rebuild libnormaliz with NAUTY=no.")
endif()

# e-antic (algebraic / renf) and its FLINT chain, built into <repo>/local by
# install_nmz_flint.sh + install_nmz_e-antic.sh. libnormaliz (ENFNORMALIZ) links
# eanticxx -> eantic -> flint -> mpfr -> gmp, so the GUI must link them in order.
# e-antic on Windows marks its API dllimport (no static escape), so link the
# import libs and let win-deploy.sh bundle libeantic*-3.dll. FLINT is static
# (built --disable-shared), so find_library picks libflint.a on its own.
# local-only (NO_DEFAULT_PATH): libnormaliz and e-antic were built against the
# pinned FLINT 3.0.1 in <repo>/local; a newer system FLINT (MSYS2 ships 3.5) must
# not be picked, or its differing struct layout corrupts the shared archive.
# On Linux/macOS e-antic's headers use visibility attributes (not dllimport), so
# link it statically -> the AppImage/dmg need no libeantic .so/.dylib bundled.
# On Windows e-antic marks its API dllimport, so the import lib + DLL are required.
if(NOT WIN32)
    set(_nmz_saved_suffixes ${CMAKE_FIND_LIBRARY_SUFFIXES})
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
endif()
find_library(EANTICXX_LIB NAMES eanticxx PATHS "${NORMALIZ_SRC}/../local/lib" NO_DEFAULT_PATH)
find_library(EANTIC_LIB   NAMES eantic   PATHS "${NORMALIZ_SRC}/../local/lib" NO_DEFAULT_PATH)
find_library(FLINT_LIB    NAMES flint    PATHS "${NORMALIZ_SRC}/../local/lib" NO_DEFAULT_PATH)
if(NOT WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${_nmz_saved_suffixes})
endif()
find_library(MPFR_LIB     NAMES mpfr     PATHS "${CMAKE_PREFIX_PATH}/lib")
if(NOT (EANTICXX_LIB AND EANTIC_LIB AND FLINT_LIB AND MPFR_LIB))
    message(FATAL_ERROR
        "e-antic / FLINT / MPFR not found. Build them into <repo>/local first: "
        "install_scripts_opt/install_nmz_flint.sh + install_nmz_e-antic.sh.")
endif()

target_link_libraries(normaliz-gui PRIVATE
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::PrintSupport
    "${NORMALIZ_LIB}"
    "${NAUTY_LIB}"
    "${EANTICXX_LIB}"
    "${EANTIC_LIB}"
    "${FLINT_LIB}"
    "${MPFR_LIB}"
    gmpxx
    gmp
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(normaliz-gui PRIVATE OpenMP::OpenMP_CXX)
endif()

# Windows: psapi provides GetProcessMemoryInfo for the status-bar memory gauge.
if(WIN32)
    target_link_libraries(normaliz-gui PRIVATE psapi)
endif()

# On Windows, bundle Qt + all transitive MinGW DLLs next to the exe after each
# build so it runs standalone (double-click) without /mingw64/bin on PATH.
if(WIN32)
    add_custom_command(TARGET normaliz-gui POST_BUILD
        COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/scripts/win-deploy.sh" "$<TARGET_FILE:normaliz-gui>"
        VERBATIM
        COMMENT "Bundling Qt + MinGW DLLs for standalone run")
endif()
