#!/usr/bin/env bash
# WaForSpeed — installation des paquets du SDK Android de la CI (NDK, CMake, plateformes), robuste aux téléchargements
# abîmés (WFS-445). Usage : bash tools/qa/installer_sdk_android.sh "ndk;27.2.12479018" "cmake;3.22.1" ...
#
# Pourquoi (run #1007 de build.yml, 2026-11-11) : le NDK 27.2 est arrivé abîmé (« Error on ZipFile unknown archive ») et
# le job android est tombé au 1er essai. Ici : jusqu'à 3 essais ; après un échec, les fichiers partiels (.temp, dossier
# du paquet à moitié écrit) sont effacés, attente croissante (15 s, 30 s) ; 15 min au plus en tout (RG-05) ;
# après le dernier essai, code 1 : le job reste rouge (RG-04).
set -u
[ $# -gt 0 ] || { echo "::error::installer_sdk_android.sh : aucun paquet demandé"; exit 2; }
SDK=${ANDROID_HOME:?ANDROID_HOME absent}
OUTIL="$SDK/cmdline-tools/latest/bin/sdkmanager"
LIMITE=${WFS_SDK_LIMITE:-900}
debut=$SECONDS

# Dossier d'un paquet : « ndk;27.2.12479018 » → $SDK/ndk/27.2.12479018
dossier() { echo "$SDK/${1//;//}"; }

# Vrai si chaque paquet demandé est en place (source.properties écrit à la fin de l'installation).
complets() {
  local p
  for p in "$@"; do [ -f "$(dossier "$p")/source.properties" ] || return 1; done
}

for essai in 1 2 3; do
  r=$(( LIMITE - (SECONDS - debut) )); [ "$r" -lt 1 ] && r=1
  yes | timeout "$r" "$OUTIL" "$@" > /dev/null
  code=${PIPESTATUS[1]}
  if [ "$code" = 0 ] && complets "$@"; then
    echo "SDK Android installé à l'essai $essai en $(( SECONDS - debut )) s : $*"
    exit 0
  fi
  if [ "$essai" = 3 ] || [ $(( SECONDS - debut )) -ge "$LIMITE" ]; then
    echo "::error::sdkmanager : échec après $essai essai(s) (code $code) en $(( SECONDS - debut )) s : $*"
    exit 1
  fi
  # Téléchargement abîmé : on efface les restes avant de recommencer.
  sudo rm -rf "$SDK/.temp" 2> /dev/null || rm -rf "$SDK/.temp"
  for p in "$@"; do
    complets "$p" || { d=$(dossier "$p"); sudo rm -rf "$d" 2> /dev/null || rm -rf "$d"; }
  done
  attente=$(( 15 * essai ))
  echo "::warning::sdkmanager : essai $essai en échec (code $code), fichiers partiels effacés, nouvel essai dans $attente s"
  sleep "$attente"
done
