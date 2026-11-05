#!/usr/bin/env bash
# WaForSpeed — vérification des pages mémoire de 16 Ko (WFS-129, RG-05), lancée par le job « android » de la CI.
# Usage : tools/qa/pages_16k.sh <APK> [AAB]
# Vérifie :
#   1) chaque segment LOAD de chaque bibliothèque native (.so) de l'APK (et de l'AAB s'il est donné) a un alignement
#      d'au moins 2**14 (16 Ko) : `llvm-objdump -p` du NDK ;
#   2) l'APK passe `zipalign -c -P 16 -v 4` (bibliothèques non compressées alignées sur 16 Ko dans l'archive).
# Échec franc (code 1) avec un message en français si l'un des deux manque.
# Outils : $NDK (sinon $ANDROID_HOME/ndk/27.2.12479018) et le zipalign le plus récent de $ANDROID_HOME/build-tools.
set -u
APK=$1; AAB=${2:-}
NDK=${NDK:-${ANDROID_HOME:-}/ndk/27.2.12479018}
OBJDUMP=${OBJDUMP:-$(ls "$NDK"/toolchains/llvm/prebuilt/*/bin/llvm-objdump 2>/dev/null | head -1)}
ZIPALIGN=${ZIPALIGN:-$(ls -d "${ANDROID_HOME:-}"/build-tools/*/zipalign 2>/dev/null | sort -V | tail -1)}
[ -x "$OBJDUMP" ] || { echo "::error::pages 16 Ko : llvm-objdump du NDK introuvable ($NDK)"; exit 1; }
[ -x "$ZIPALIGN" ] || { echo "::error::pages 16 Ko : zipalign introuvable (build-tools)"; exit 1; }
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
ECHEC=0
verifier_archive() {   # $1 archive, $2 étiquette
  local dossier="$TMP/$2" n=0
  mkdir -p "$dossier"
  unzip -q -o "$1" '*.so' -d "$dossier" 2>/dev/null
  while IFS= read -r so; do
    n=$((n + 1))
    local rel=${so#"$dossier"/}
    # une ligne « LOAD off … align 2**N » par segment chargé
    local aligns
    aligns=$("$OBJDUMP" -p "$so" | awk '$1 == "LOAD" { for (i = 1; i <= NF; i++) if ($i == "align") print $(i + 1) }')
    if [ -z "$aligns" ]; then echo "::error::pages 16 Ko : $2 $rel : aucun segment LOAD lu"; ECHEC=1; continue; fi
    while IFS= read -r a; do
      local p=${a#'2**'}
      if [ "$p" -ge 14 ] 2>/dev/null; then echo "  $2 $rel : segment LOAD aligné sur $a (OK)"
      else echo "::error::pages 16 Ko : $2 $rel : segment LOAD aligné sur $a, moins de 2**14 (16 Ko)"; ECHEC=1; fi
    done <<< "$aligns"
  done < <(find "$dossier" -name '*.so' | sort)
  if [ "$n" = 0 ]; then echo "::error::pages 16 Ko : aucune bibliothèque native dans $2"; ECHEC=1; fi
  echo "$2 : $n bibliothèque(s) native(s) vérifiée(s)"
}
echo "Pages de 16 Ko : $OBJDUMP, $ZIPALIGN"
verifier_archive "$APK" APK
[ -n "$AAB" ] && verifier_archive "$AAB" AAB
# 2) alignement des fichiers dans l'APK (bibliothèques non compressées sur 16 Ko, le reste sur 4 octets)
if "$ZIPALIGN" -c -P 16 -v 4 "$APK" > "$TMP/zipalign.txt" 2>&1; then
  grep -E '\.so|Verification' "$TMP/zipalign.txt"
  echo "zipalign -c -P 16 -v 4 : OK"
else
  grep -vE '\(OK' "$TMP/zipalign.txt" | tail -20
  echo "::error::pages 16 Ko : l'APK ne passe pas « zipalign -c -P 16 -v 4 » (bibliothèques non alignées sur 16 Ko dans l'archive)"; ECHEC=1
fi
if [ "$ECHEC" = 0 ]; then echo "Pages de 16 Ko : tout est aligné (2**14 ou plus), l'APK se charge sur un téléphone à pages de 16 Ko"
else echo "Pages de 16 Ko : ÉCHEC"; fi
exit $ECHEC
