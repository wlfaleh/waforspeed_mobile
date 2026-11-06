#!/bin/bash
# WaForSpeed — WFS-149 (RG-02) : télécharge les encodeurs libres de tools/textures/convertir.py (licence Apache-2.0),
# versions épinglées, dans $WFS_ENCODEURS (défaut : tools/textures/.encodeurs, ignoré par git) :
#   - astcenc 5.2.0 (Arm Limited, https://github.com/ARM-software/astc-encoder) : binaire officiel, empreinte SHA-256 vérifiée
#   - etc2comp (Google, https://github.com/google/etc2comp), révision 39422c1 : compilé depuis les sources (g++)
# Aucun encodeur n'est copié dans le dépôt ni livré dans l'APK : ce sont des outils de conversion.
set -euo pipefail
ICI=$(cd "$(dirname "$0")" && pwd)
DEST=${WFS_ENCODEURS:-$ICI/.encodeurs}
mkdir -p "$DEST"
ASTC_URL=https://github.com/ARM-software/astc-encoder/releases/download/5.2.0/astcenc-5.2.0-linux-x64.zip
ASTC_SHA=b8c805b3c4a32680287ad5e5fc2a8b5ddf78c80f24cc0ec7ed1e4eb03c6cc5d2
ETC_REV=39422c1aa2f4889d636db5790af1d0be6ff3a226
if [ ! -x "$DEST/astcenc/bin/astcenc-avx2" ]; then
  curl -sSfL -o "$DEST/astcenc.zip" "$ASTC_URL"
  echo "$ASTC_SHA  $DEST/astcenc.zip" | sha256sum -c --quiet
  rm -rf "$DEST/astcenc" && mkdir -p "$DEST/astcenc" && (cd "$DEST/astcenc" && unzip -q ../astcenc.zip) && chmod +x "$DEST"/astcenc/bin/*
fi
if [ ! -x "$DEST/etc2comp/EtcTool" ]; then
  rm -rf "$DEST/etc2comp" && git clone -q https://github.com/google/etc2comp.git "$DEST/etc2comp"
  (cd "$DEST/etc2comp" && git checkout -q "$ETC_REV" &&
   g++ -O2 -std=c++11 -pthread -w -include cstdint -include cstring -include cmath -IEtcLib/Etc -IEtcLib/EtcCodec -Ithird_party/lodepng \
       EtcLib/Etc/*.cpp EtcLib/EtcCodec/*.cpp EtcTool/*.cpp third_party/lodepng/lodepng.cpp -o EtcTool)
fi
echo "encodeurs prêts dans $DEST : astcenc 5.2.0, etc2comp $ETC_REV"
