#!/usr/bin/env bash
# WaForSpeed — captures avant / après d'une PR, pour le lead dev (règle de l'utilisateur, 2026-11-07 : pas de fusion sans
# au moins 3 captures de la fonctionnalité dans la PR ; c'est le lead dev qui regarde les photos et qui fusionne).
#
# Usage, depuis la racine du dépôt, sur la branche du ticket (synchronisée avec main) :
#   bash tools/qa/captures_pr.sh WFS-XX <banc>[:LxH] [<banc>[:LxH] ...]
#   ex. : bash tools/qa/captures_pr.sh WFS-235 camera:2400x1080 nuit
# Variables facultatives :
#   FILTRE='cockpit-*'   ne garder que les captures dont le nom correspond (motif du shell, sans .png)
#   SANS_AVANT=1         pas de captures « avant » (nouvel écran qui n'existe pas sur main)
#
# Ce que fait le script :
#   1. compile la branche (build/) et lance chaque banc → captures « après » ;
#   2. compile origin/main dans un dossier à part (arbre de travail git temporaire) et lance les mêmes bancs → captures « avant » ;
#   3. réduit chaque capture en JPG (1280 px de large, qualité 85) dans qa/pr/WFS-XX/ : avant-<nom>.jpg, apres-<nom>.jpg ;
#   4. écrit qa/pr/WFS-XX/README.md (tableau avant / après) et qa/pr/WFS-XX/pr.md (texte à coller dans la description de la PR) ;
#   5. s'arrête en erreur s'il y a moins de 3 captures « après ».
# Durée mesurée (2026-11-07) : banc « chargement » avant + après ≈ 1 min 30 ; les bancs en 2400x1080 (hud, camera) sont bien plus lents : préférer 1280x720 sauf besoin.
# Le rendu est le même que la CI (lavapipe) : paquets mesa-vulkan-drivers et glslc (voir track-agent.md, « Info pour les développeurs »).
set -euo pipefail

TICKET=${1:?usage : captures_pr.sh WFS-XX <banc>[:LxH] ...}
shift
[ $# -ge 1 ] || { echo "erreur : au moins un banc (ex. camera:2400x1080)"; exit 2; }
[[ "$TICKET" =~ ^WFS-[0-9]+$ ]] || { echo "erreur : ticket attendu sous la forme WFS-XX"; exit 2; }
FILTRE=${FILTRE:-*}
RACINE=$(git rev-parse --show-toplevel)
cd "$RACINE"
BRANCHE=$(git rev-parse --abbrev-ref HEAD)
SORTIE="qa/pr/$TICKET"
TRAVAIL=$(mktemp -d "${TMPDIR:-/tmp}/captures_pr.XXXXXX")
AVANT_SRC="$TRAVAIL/main"
nettoyer() { git worktree remove --force "$AVANT_SRC" >/dev/null 2>&1 || true; }
trap nettoyer EXIT

compiler() {   # $1 = dossier source
  (cd "$1" && cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release > /dev/null && cmake --build build --target wfs_bench > /dev/null)
}
lancer_bancs() {   # $1 = dossier source, $2 = dossier des captures
  local src=$1 dest=$2 b nom taille
  for b in "${BANCS[@]}"; do
    nom=${b%%:*}; taille=""
    [ "$b" != "$nom" ] && taille=${b#*:}
    mkdir -p "$dest/$nom"
    echo "  banc $nom ${taille:+($taille)}"
    if [ -n "$taille" ]; then
      (cd "$src" && WFS_BANC=$nom WFS_TAILLE=$taille ./build/wfs_bench "$dest/$nom" assets/ > "$dest/$nom.journal" 2>&1) || echo "  attention : le banc $nom a rendu une erreur (voir $dest/$nom.journal)"
    else
      (cd "$src" && WFS_BANC=$nom ./build/wfs_bench "$dest/$nom" assets/ > "$dest/$nom.journal" 2>&1) || echo "  attention : le banc $nom a rendu une erreur (voir $dest/$nom.journal)"
    fi
  done
}
reduire() {   # $1 = png source, $2 = jpg cible
  if command -v convert > /dev/null; then convert "$1" -resize '1280x>' -quality 85 "$2"
  elif command -v ffmpeg > /dev/null; then ffmpeg -v error -y -i "$1" -vf "scale='min(1280,iw)':-2" -q:v 3 "$2"
  else cp "$1" "${2%.jpg}.png"; fi
}

BANCS=("$@")
echo "1/4 compilation de la branche $BRANCHE"
compiler "$RACINE"
echo "2/4 captures « après »"
lancer_bancs "$RACINE" "$TRAVAIL/apres"
if [ "${SANS_AVANT:-0}" != 1 ]; then
  echo "3/4 compilation de origin/main et captures « avant »"
  git fetch -q origin main
  git worktree add -q --detach "$AVANT_SRC" origin/main
  compiler "$AVANT_SRC"
  lancer_bancs "$AVANT_SRC" "$TRAVAIL/avant"
else
  echo "3/4 pas de captures « avant » (SANS_AVANT=1)"
fi

echo "4/4 réduction des captures dans $SORTIE"
mkdir -p "$SORTIE"
n=0
LIGNES=""
while IFS= read -r png; do
  rel=${png#"$TRAVAIL/apres/"}; nom=$(basename "$rel" .png)
  # shellcheck disable=SC2053
  [[ "$nom" == $FILTRE ]] || continue
  reduire "$png" "$SORTIE/apres-$nom.jpg"; n=$((n + 1))
  avant="—"
  if [ -f "$TRAVAIL/avant/$rel" ]; then reduire "$TRAVAIL/avant/$rel" "$SORTIE/avant-$nom.jpg"; avant="![avant](avant-$nom.jpg)"; fi
  LIGNES+="| \`$nom\` | $avant | ![après](apres-$nom.jpg) |"$'\n'
done < <(find "$TRAVAIL/apres" -name '*.png' | sort)

{
  echo "# $TICKET — captures avant / après (tools/qa/captures_pr.sh)"
  echo
  echo "Branche \`$BRANCHE\` comparée à \`origin/main\` ; bancs : ${BANCS[*]} ; filtre : \`$FILTRE\`."
  echo
  echo "| Capture | Avant (main) | Après (branche) |"
  echo "|---|---|---|"
  printf '%s' "$LIGNES"
} > "$SORTIE/README.md"
URL="https://github.com/wlfaleh/waforspeed_mobile/blob/$BRANCHE/$SORTIE"
{
  echo "## Captures pour le lead dev ($n après)"
  echo
  echo "Dossier : [\`$SORTIE\`]($URL) — produit par \`tools/qa/captures_pr.sh\`."
  echo
  echo "| Capture | Avant (main) | Après (branche) |"
  echo "|---|---|---|"
  printf '%s' "$LIGNES" | sed "s#](avant-#]($URL/avant-#g; s#](apres-#]($URL/apres-#g; s#\.jpg)#.jpg?raw=true)#g"
} > "$SORTIE/pr.md"

echo "$n capture(s) « après » dans $SORTIE (README.md, pr.md)"
if [ "$n" -lt 3 ]; then echo "erreur : il faut au moins 3 captures de la fonctionnalité (règle de l'utilisateur)"; exit 1; fi
