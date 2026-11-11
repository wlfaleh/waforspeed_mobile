#!/usr/bin/env bash
# WaForSpeed — installation des paquets Ubuntu de la CI, robuste aux pannes passagères des miroirs (WFS-445).
# Usage (lanceur GitHub ubuntu-24.04) : bash tools/qa/installer_paquets.sh <paquet> [<paquet>...]
#
# Pourquoi (runs #1005 à #1009 de build.yml, 2026-11-11) : l'image du lanceur lit Ubuntu par la liste de miroirs
# « mirror+file:/etc/apt/apt-mirrors.txt », dont le 1er est azure.archive.ubuntu.com. Quand ce nom ne se résout plus,
# chaque fichier attend 20 s, « apt-get update » ne sort qu'en avertissement, l'installation échoue et les 2 essais
# de 5 min de WFS-237 / WFS-126 sont épuisés. Ici :
#   1. si azure.archive.ubuntu.com ne se résout pas, le serveur officiel archive.ubuntu.com est écrit directement ;
#   2. jusqu'à 3 essais, attente croissante (15 s, 30 s), serveur suivant à chaque essai (archive, puis miroir kernel.org) ;
#   3. « apt-get update --error-on=any » : une liste non lue fait échouer l'essai (plus d'installation sur des listes périmées) ;
#   4. 15 min au plus en tout (RG-05) ; après le dernier essai, code 1 : le job reste rouge (RG-04, rien d'affaibli).
set -u
[ $# -gt 0 ] || { echo "::error::installer_paquets.sh : aucun paquet demandé"; exit 2; }

LIMITE=${WFS_APT_LIMITE:-900}          # secondes, toutes tentatives comprises (RG-05)
SOURCES=/etc/apt/sources.list.d/ubuntu.sources
OPTS=(-o Acquire::Retries=3 -o Acquire::http::Timeout=20 -o Acquire::https::Timeout=20)
SERVEURS=(http://archive.ubuntu.com/ubuntu http://mirrors.edge.kernel.org/ubuntu)
debut=$SECONDS

# Écrit le serveur $1 à la place de la liste de miroirs (ou du serveur précédent) dans les sources Ubuntu.
ecrire_serveur() {
  if [ -f "$SOURCES" ]; then
    sudo sed -i -E "s#^URIs:.*#URIs: $1#" "$SOURCES"
  fi
  if [ -f /etc/apt/sources.list ]; then
    sudo sed -i -E "s#(mirror\+file:/etc/apt/apt-mirrors\.txt|https?://([a-z]+\.)?archive\.ubuntu\.com/ubuntu/?|https?://security\.ubuntu\.com/ubuntu/?|https?://mirrors\.edge\.kernel\.org/ubuntu/?)#$1#g" /etc/apt/sources.list
  fi
  echo "Serveur Ubuntu écrit : $1"
}

# Temps restant pour une commande (au plus 300 s, au moins 1 s).
reste() {
  local r=$(( LIMITE - (SECONDS - debut) ))
  [ "$r" -gt 300 ] && r=300
  [ "$r" -lt 1 ] && r=1
  echo "$r"
}

suivant=0
if ! timeout 10 getent hosts azure.archive.ubuntu.com > /dev/null; then
  echo "::warning::azure.archive.ubuntu.com ne se résout pas : serveur officiel d'Ubuntu utilisé directement"
  ecrire_serveur "${SERVEURS[0]}"; suivant=1
fi

for essai in 1 2 3; do
  if timeout "$(reste)" sudo apt-get "${OPTS[@]}" update --error-on=any \
     && timeout "$(reste)" sudo apt-get "${OPTS[@]}" install -y "$@"; then
    echo "Paquets installés à l'essai $essai en $(( SECONDS - debut )) s : $*"
    exit 0
  fi
  if [ "$essai" = 3 ] || [ $(( SECONDS - debut )) -ge "$LIMITE" ]; then
    echo "::error::apt-get : échec après $essai essai(s) en $(( SECONDS - debut )) s (paquets : $*)"
    exit 1
  fi
  attente=$(( 15 * essai ))
  echo "::warning::apt-get : essai $essai en échec, nouvel essai dans $attente s"
  sleep "$attente"
  if [ "$suivant" -lt "${#SERVEURS[@]}" ]; then ecrire_serveur "${SERVEURS[$suivant]}"; suivant=$(( suivant + 1 )); fi
done
