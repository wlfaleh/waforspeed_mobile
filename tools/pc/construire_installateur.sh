#!/usr/bin/env bash
# WaForSpeed — version PC Windows (WFS-602) : construit WaForSpeed.exe (MinGW, depuis Linux) puis WaForSpeed-Setup.exe (NSIS).
#   bash tools/pc/construire_installateur.sh            → build-windows/WaForSpeed-Setup.exe
# Prérequis (Ubuntu) : apt-get install mingw-w64 nsis glslc libvulkan-dev cmake ninja-build
# Contenu de l'installateur = contenu des assets de l'APK (android/app/build.gradle) : assets/ + configgame.json + textes
# des CGU, sans les voitures retirées (WFS-404 : VOITURES_RETIREES de src/game/cars.h, licence non commerciale).
set -euo pipefail
cd "$(dirname "$0")/../.."
B=build-windows
VERSION="${VERSION:-$(grep -o 'versionName *"[^"]*"' android/app/build.gradle | head -1 | grep -o '"[^"]*"' | tr -d '"' || true)}"
VERSION="$(echo "${VERSION:-1.0.0}" | grep -oE '^[0-9]+(\.[0-9]+){0,2}' || echo 1.0.0)"
while [ "$(echo "$VERSION" | tr -cd . | wc -c)" -lt 2 ]; do VERSION="$VERSION.0"; done
mkdir -p "$B/vk"
for d in vulkan vk_video; do [ -d "/usr/include/$d" ] && rm -rf "$B/vk/$d" && cp -r "/usr/include/$d" "$B/vk/"; done
cmake -S . -B "$B" -G Ninja -DCMAKE_TOOLCHAIN_FILE=tools/pc/mingw-w64.cmake -DWFS_VULKAN_INCLUDE="$PWD/$B/vk" >/dev/null
ninja -C "$B" WaForSpeed
x86_64-w64-mingw32-strip "$B/WaForSpeed.exe"

# dossier préparé : ce que l'installateur copie dans Program Files\WaForSpeed
P="$B/installateur/WaForSpeed"
rm -rf "$B/installateur"; mkdir -p "$P/assets/cgu"
cp "$B/WaForSpeed.exe" "$P/"
retirees="$(grep 'VOITURES_RETIREES\[\] =' src/game/cars.h | grep -o '"[A-Za-z0-9_-]*"' | tr -d '"')"
cp -r assets/. "$P/assets/"
find "$P/assets" -name '.*' -prune -exec rm -rf {} +
for v in $retirees; do rm -f "$P/assets/cars/$v.wfm" "$P/assets/cars/$v.lod" "$P/assets/cars/$v.txt"; done
cp configgame.json "$P/assets/"
for f in cgu.md confidentialite.md cgu-en.md confidentialite-en.md; do [ -f "store/$f" ] && cp "store/$f" "$P/assets/cgu/"; done
for v in $retirees; do if ls "$P/assets/cars/$v".* >/dev/null 2>&1; then echo "voiture retirée encore présente : $v" >&2; exit 1; fi; done

makensis -V2 -DSOURCE="$PWD/$P" -DSORTIE="$PWD/$B/WaForSpeed-Setup.exe" -DVERSION="$VERSION" tools/pc/installateur.nsi
ls -la "$B/WaForSpeed-Setup.exe"
echo "Installateur prêt : $B/WaForSpeed-Setup.exe (version $VERSION)"
