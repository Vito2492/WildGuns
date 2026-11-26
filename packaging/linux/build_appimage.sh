#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
APPDIR="$ROOT/build/WildGuns.AppDir"
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/lib"

cp "$ROOT/wildguns" "$APPDIR/usr/bin/wildguns"
chmod +x "$APPDIR/usr/bin/wildguns"

# Bundle snes9x_libretro.so next to the binary
if [ -f "$ROOT/snes9x_libretro.so" ]; then
    cp "$ROOT/snes9x_libretro.so" "$APPDIR/usr/bin/snes9x_libretro.so"
elif [ -n "${SNES9X_SO:-}" ] && [ -f "$SNES9X_SO" ]; then
    cp "$SNES9X_SO" "$APPDIR/usr/bin/snes9x_libretro.so"
else
    echo "WARNING: snes9x_libretro.so not found; set SNES9X_SO or place it in repo root."
fi

cp "$ROOT/packaging/linux/wild-guns.desktop" "$APPDIR/wild-guns.desktop"
cp "$ROOT/packaging/linux/wild-guns.svg" "$APPDIR/wild-guns.svg"

cat > "$APPDIR/AppRun" <<'EOF'
#!/usr/bin/env bash
HERE="$(dirname "$(readlink -f "${0}")")"
export LD_LIBRARY_PATH="$HERE/usr/lib:${LD_LIBRARY_PATH:-}"
exec "$HERE/usr/bin/wildguns" "$@"
EOF
chmod +x "$APPDIR/AppRun"

if command -v appimagetool >/dev/null 2>&1; then
    ARCH=x86_64 appimagetool "$APPDIR" "$ROOT/WildGuns-x86_64.AppImage"
else
    echo "appimagetool not installed; AppDir staged at $APPDIR"
fi
