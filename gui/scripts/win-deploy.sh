#!/usr/bin/env bash
# Bundle Qt (windeployqt) + every transitive MinGW DLL next to the exe so it runs
# standalone on Windows (double-click) without /mingw64/bin on PATH.
# Usage: win-deploy.sh <path-to-exe>
set -e
exe="$1"
dir="$(dirname "$exe")"
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# A missing/failing windeployqt must fail the deploy loudly, not produce a
# bundle without Qt (stdout is muted - windeployqt is chatty - stderr is not).
command -v windeployqt >/dev/null 2>&1 || { echo "win-deploy: windeployqt not found on PATH" >&2; exit 1; }
windeployqt --no-translations --no-compiler-runtime "$exe" >/dev/null

# e-antic runtime DLLs live in <repo>/local/bin, not /mingw64/bin, so the
# ldd/grep recursion below would miss them. Copy them in first; copy_deps then
# pulls their own /mingw64 dependencies (gmp, mpfr, libstdc++, ...).
for d in "$script_dir/../../local/bin"/libeantic*.dll; do
    [ -f "$d" ] && cp -f "$d" "$dir"/
done

cd "$dir"
copy_deps() {
    ldd "$1" 2>/dev/null | grep -oiE '/mingw64/bin/[^ ]+\.dll' | while read -r m; do
        n="$(basename "$m")"
        if [ ! -f "./$n" ]; then cp "$m" . && copy_deps "$n"; fi
    done
}

shopt -s nullglob
exe_name="$(basename "$exe")"
for f in "$exe_name" *.dll \
         platforms/*.dll styles/*.dll tls/*.dll \
         imageformats/*.dll networkinformation/*.dll generic/*.dll iconengines/*.dll; do
    [ -f "$f" ] && copy_deps "$f"
done
