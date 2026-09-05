#!/usr/bin/env bash

# Collects comicpress.exe and everything it needs into a self-contained folder.

set -euo pipefail

build_dir=${1:-build}
dist=${2:-$HOME/comicpress-dist}

rm -rf "$dist"
mkdir -p "$dist/lib"

cp "$build_dir/comicpress.exe" "$dist"
cp -r /ucrt64/lib/vips-modules-* "$dist/lib/"
rm -f "$dist"/lib/vips-modules-*/vips-{magick,openslide,poppler}.dll

windeployqt.exe "$dist/comicpress.exe"

# The vips modules are dlopened, so they are absent from the executable's
# import table and have to be scanned separately.
for f in "$dist/comicpress.exe" "$dist"/lib/vips-modules-*/*.dll; do
    ldd "$f" | awk '/\/ucrt64\/bin\//{print $3}'
done | sort -u | xargs -r cp -t "$dist"

for f in WinSparkle.dll iconengines/qsvgicon.dll platforms/qwindows.dll; do
    test -f "$dist/$f" || { echo "bundle.sh: missing $f" >&2; exit 1; }
done

echo "bundled to $dist"
