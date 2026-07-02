cmake_minimum_required(VERSION 3.16)
project(normaliz-gui CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)
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

# libnormaliz headers were compiled in the "classic makefile" mode (no nmz_config.h,
# no optional libraries). Match that when including cone.h here.
target_compile_definitions(normaliz-gui PRIVATE NMZ_MAKEFILE_CLASSIC)

target_include_directories(normaliz-gui PRIVATE "${NORMALIZ_SRC}")

target_link_libraries(normaliz-gui PRIVATE
    Qt6::Widgets
    Qt6::Concurrent
    "${NORMALIZ_LIB}"
    gmpxx
    gmp
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(normaliz-gui PRIVATE OpenMP::OpenMP_CXX)
endif()

# On Windows, bundle Qt + all transitive MinGW DLLs next to the exe after each
# build so it runs standalone (double-click) without /mingw64/bin on PATH.
if(WIN32)
    add_custom_command(TARGET normaliz-gui POST_BUILD
        COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/scripts/win-deploy.sh" "$<TARGET_FILE:normaliz-gui>"
        VERBATIM
        COMMENT "Bundling Qt + MinGW DLLs for standalone run")
endif()
