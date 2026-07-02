#!/usr/bin/env bash
# Bundle Qt (windeployqt) + every transitive MinGW DLL next to the exe so it runs
# standalone on Windows (double-click) without /mingw64/bin on PATH.
# Usage: win-deploy.sh <path-to-exe>
set -e
exe="$1"
dir="$(dirname "$exe")"

windeployqt --no-translations --no-compiler-runtime "$exe" >/dev/null 2>&1 || true

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
