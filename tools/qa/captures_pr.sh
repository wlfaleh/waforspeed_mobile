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
#   SANS_CACHE=1         refaire les captures « avant » comme avant WFS-555 : arbre de travail temporaire, compilation de main
#                        depuis zéro, bancs rejoués ; le cache n'est ni lu ni écrit
#   WFS_CACHE_CAPTURES=<dossier>   dossier du cache (par défaut ${XDG_CACHE_HOME:-$HOME/.cache}/wfs-captures, hors du dépôt)
# Variable OBLIGATOIRE (règle de l'utilisateur, 2026-11-07) :
#   AGENT='Claude, agent 2 (01UY1Vv5)'   nom de l'agent tel qu'il est écrit dans track-agent.md : il est écrit en tête de pr.md
#
# Ce que fait le script :
#   1. compile la branche (build/) et lance chaque banc → captures « après » ;
#   2. captures « avant » de origin/main (WFS-555, cache par commit de main) :
#      - chaque banc (nom + taille) déjà capturé pour ce commit de main (SHA complet) est relu dans le cache, sans rien recompiler ;
#      - sinon, l'arbre de travail de main gardé dans le cache passe sur ce commit, se recompile en incrémental (Ninja),
#        rejoue le banc et range ses captures dans le cache (dossier temporaire, témoin « .complet », puis renommage) ;
#      - le cache ne sert que pour le même SHA de main : un nouveau commit de main donne un nouveau dossier ;
#        seuls les 3 derniers commits sont gardés ; un verrou (flock) protège l'arbre de main et le cache ;
#   3. réduit chaque capture en JPG (1280 px de large, qualité 85) dans qa/pr/WFS-XX/ : avant-<nom>.jpg, apres-<nom>.jpg ;
#   4. écrit qa/pr/WFS-XX/README.md (tableau avant / après) et qa/pr/WFS-XX/pr.md (texte à coller dans la description de la PR) ;
#   5. s'arrête en erreur s'il y a moins de 3 captures « après ».
# Durée mesurée (2026-11-07, avant WFS-555) : banc « chargement » avant + après ≈ 1 min 30 ; les bancs en 2400x1080 (hud, camera) sont bien plus lents : préférer 1280x720 sauf besoin.
# Le rendu est le même que la CI (lavapipe) : paquets mesa-vulkan-drivers et glslc (voir track-agent.md, « Info pour les développeurs »).
set -euo pipefail

TICKET=${1:?usage : captures_pr.sh WFS-XX <banc>[:LxH] ...}
shift
[ $# -ge 1 ] || { echo "erreur : au moins un banc (ex. camera:2400x1080)"; exit 2; }
[[ "$TICKET" =~ ^WFS-[0-9]+$ ]] || { echo "erreur : ticket attendu sous la forme WFS-XX"; exit 2; }
FILTRE=${FILTRE:-*}
AGENT=${AGENT:-}
[ -n "$AGENT" ] || { echo "erreur : AGENT='<nom de l'agent dans track-agent.md>' obligatoire (règle de l'utilisateur : nom de l'agent dans chaque PR)"; exit 2; }
RACINE=$(git rev-parse --show-toplevel)
cd "$RACINE"
BRANCHE=$(git rev-parse --abbrev-ref HEAD)
SORTIE="qa/pr/$TICKET"
TRAVAIL=$(mktemp -d "${TMPDIR:-/tmp}/captures_pr.XXXXXX")
AVANT_SRC="$TRAVAIL/main"
nettoyer() { git worktree remove --force "$AVANT_SRC" >/dev/null 2>&1 || true; }
trap nettoyer EXIT
# Cache des captures « avant » (WFS-555), hors du dépôt. L'arbre de travail de main est propre à chaque clone
# (son fichier .git pointe vers ce clone) : son nom porte une empreinte du dossier git commun du clone.
CACHE=${WFS_CACHE_CAPTURES:-${XDG_CACHE_HOME:-$HOME/.cache}/wfs-captures}
GIT_COMMUN=$(cd "$(git rev-parse --git-common-dir)" && pwd -P)
MAIN_SRC="$CACHE/main-src-$(printf '%s' "$GIT_COMMUN" | sha256sum | cut -c1-12)"
CACHE_GARDES=3          # nombre de commits de main gardés dans le cache
SOURCE_AVANT="—"        # écrit dans README.md et pr.md : d'où viennent les captures « avant »

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
      (cd "$src" && WFS_BANC=$nom WFS_TAILLE=$taille ./build/wfs_bench "$dest/$nom" assets/ > "$dest/$nom.journal" 2>&1) || { echo "  attention : le banc $nom a rendu une erreur (voir $dest/$nom.journal)"; touch "$dest/.echec"; }
    else
      (cd "$src" && WFS_BANC=$nom ./build/wfs_bench "$dest/$nom" assets/ > "$dest/$nom.journal" 2>&1) || { echo "  attention : le banc $nom a rendu une erreur (voir $dest/$nom.journal)"; touch "$dest/.echec"; }
    fi
  done
}
reduire() {   # $1 = png source, $2 = jpg cible
  if command -v convert > /dev/null; then convert "$1" -resize '1280x>' -quality 85 "$2"
  elif command -v ffmpeg > /dev/null; then ffmpeg -v error -y -i "$1" -vf "scale='min(1280,iw)':-2" -q:v 3 "$2"
  else cp "$1" "${2%.jpg}.png"; fi
}

# Nom du dossier de cache d'un banc : <nom> ou <nom>_<LxH> (deux tailles d'un même banc ne se mélangent pas).
cle_banc() { local nom=${1%%:*}; if [ "$1" != "$nom" ]; then echo "${nom}_${1#*:}"; else echo "$nom"; fi; }

# Met l'arbre de travail de main gardé sur le commit $1 ; le recrée s'il est absent, cassé ou lié à un autre clone.
preparer_main_src() {
  local sha=$1
  if [ -d "$MAIN_SRC" ] && [ "$(cd "$MAIN_SRC" 2>/dev/null && cd "$(git rev-parse --git-common-dir 2>/dev/null)" 2>/dev/null && pwd -P)" = "$GIT_COMMUN" ] \
     && git -C "$MAIN_SRC" checkout -q --force --detach "$sha" 2>/dev/null; then
    return 0
  fi
  echo "  arbre de travail de main (re)créé dans $MAIN_SRC"
  git worktree remove --force "$MAIN_SRC" > /dev/null 2>&1 || true
  rm -rf "$MAIN_SRC"
  git worktree prune
  git worktree add -q --detach "$MAIN_SRC" "$sha"
}

# Captures « avant » par le cache : $1 = SHA complet de main, $2 = dossier des captures « avant » du lancement.
avant_par_cache() {
  local sha=$1 dest=$2 b cle dossier tmp manquants=() compile=0 lus=0 faits=0
  mkdir -p "$CACHE/$sha"
  rm -rf "$CACHE/$sha"/.en-cours.*    # restes d'un lancement interrompu (le verrou est tenu : personne d'autre n'écrit)
  for b in "${BANCS[@]}"; do
    [ -f "$CACHE/$sha/$(cle_banc "$b")/.complet" ] || manquants+=("$b")
  done
  if [ ${#manquants[@]} -gt 0 ]; then
    preparer_main_src "$sha"
    echo "  compilation incrémentale de main ($sha) dans $MAIN_SRC"
    # Repli : dossier build/ abîmé ou d'une autre version de CMake → recompilation depuis zéro, une fois.
    compiler "$MAIN_SRC" || { echo "  compilation incrémentale en échec : build/ de main recréé"; rm -rf "$MAIN_SRC/build"; compiler "$MAIN_SRC"; }
    compile=1
  fi
  for b in "${BANCS[@]}"; do
    cle=$(cle_banc "$b"); dossier="$CACHE/$sha/$cle"
    if [ ! -f "$dossier/.complet" ]; then
      # Banc rejoué dans un dossier temporaire ; témoin écrit à la fin, puis renommage : un banc interrompu ne laisse
      # jamais de cache « complet ».
      tmp=$(mktemp -d "$CACHE/$sha/.en-cours.$cle.XXXXXX")
      (BANCS=("$b"); lancer_bancs "$MAIN_SRC" "$tmp")
      faits=$((faits + 1))
      if [ -f "$tmp/.echec" ]; then
        # Banc en erreur sur main : captures utilisées pour ce lancement, mais jamais gardées dans le cache.
        echo "  banc $b : erreur sur main, résultat non gardé dans le cache"
        mkdir -p "$dest"; cp -r "$tmp/." "$dest/"; rm -rf "$tmp" "$dest/.echec"
        continue
      fi
      date -u "+%Y-%m-%d %H:%M:%S" > "$tmp/.complet"
      rm -rf "$dossier"; mv "$tmp" "$dossier"
    else
      echo "  banc $b : captures « avant » relues dans le cache"; lus=$((lus + 1))
    fi
    # Même disposition que lancer_bancs (dest/<nom>/, dest/<nom>.journal) ; deux tailles d'un même banc se
    # remplacent dans le même ordre qu'avant WFS-555.
    mkdir -p "$dest"
    cp -r "$dossier/." "$dest/"
    rm -f "$dest/.complet"
  done
  touch "$CACHE/$sha"
  SOURCE_AVANT="cache : $lus banc(s) relu(s), $faits banc(s) rejoué(s) ; main compilé : $([ $compile = 1 ] && echo oui || echo non)"
  # Ménage : on garde les CACHE_GARDES derniers commits de main (date du dernier usage) ; le verrou est tenu.
  find "$CACHE" -mindepth 1 -maxdepth 1 -type d -regextype posix-extended -regex '.*/[0-9a-f]{40}' -printf '%T@ %p\n' \
    | sort -rn | tail -n +$((CACHE_GARDES + 1)) | cut -d' ' -f2- | while IFS= read -r vieux; do rm -rf "$vieux"; done
}

BANCS=("$@")
echo "1/4 compilation de la branche $BRANCHE"
compiler "$RACINE"
echo "2/4 captures « après »"
lancer_bancs "$RACINE" "$TRAVAIL/apres"
if [ "${SANS_AVANT:-0}" != 1 ]; then
  git fetch -q origin main
  SHA_MAIN=$(git rev-parse --verify origin/main)   # SHA complet : seule clé du cache (jamais un autre commit)
  if [ "${SANS_CACHE:-0}" = 1 ]; then
    echo "3/4 compilation de origin/main ($SHA_MAIN) depuis zéro et captures « avant » (SANS_CACHE=1)"
    git worktree add -q --detach "$AVANT_SRC" "$SHA_MAIN"
    compiler "$AVANT_SRC"
    lancer_bancs "$AVANT_SRC" "$TRAVAIL/avant"
    SOURCE_AVANT="sans cache (SANS_CACHE=1), main compilé depuis zéro"
  else
    echo "3/4 captures « avant » de origin/main ($SHA_MAIN), cache $CACHE"
    mkdir -p "$CACHE"
    exec 9> "$CACHE/.verrou"
    flock 9            # un seul lancement à la fois touche l'arbre de main et le cache
    avant_par_cache "$SHA_MAIN" "$TRAVAIL/avant"
    flock -u 9; exec 9>&-
  fi
  SOURCE_AVANT="\`origin/main\` = \`$SHA_MAIN\` ; $SOURCE_AVANT"
else
  echo "3/4 pas de captures « avant » (SANS_AVANT=1)"
  SOURCE_AVANT="aucune (SANS_AVANT=1)"
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
  echo "Captures « avant » : $SOURCE_AVANT."
  echo
  echo "| Capture | Avant (main) | Après (branche) |"
  echo "|---|---|---|"
  printf '%s' "$LIGNES"
} > "$SORTIE/README.md"
URL="https://github.com/wlfaleh/waforspeed_mobile/blob/$BRANCHE/$SORTIE"
{
  echo "## Agent : $AGENT"
  echo
  echo "**Ticket** : $TICKET — règles : \`regles-de-gestion/$TICKET.md\`"
  echo
  echo "## Description (à remplir par le développeur avant d'ouvrir la PR)"
  echo
  echo "- **Demande de l'utilisateur** (en une phrase) : À REMPLIR"
  echo "- **Ce qui change** (fichiers, comportement, à l'écran) : À REMPLIR"
  echo "- **Comment c'est vérifié** (tests CPU, parcours, bancs, mesures) : À REMPLIR"
  echo "- **Limites et risques connus** : À REMPLIR"
  echo
  echo "## Captures pour le lead dev ($n après)"
  echo
  echo "Dossier : [\`$SORTIE\`]($URL) — produit par \`tools/qa/captures_pr.sh\`."
  echo
  echo "Captures « avant » : $SOURCE_AVANT."
  echo
  echo "| Capture | Avant (main) | Après (branche) |"
  echo "|---|---|---|"
  printf '%s' "$LIGNES" | sed "s#](avant-#]($URL/avant-#g; s#](apres-#]($URL/apres-#g; s#\.jpg)#.jpg?raw=true)#g"
} > "$SORTIE/pr.md"

echo "$n capture(s) « après » dans $SORTIE (README.md, pr.md)"
if [ "$n" -lt 3 ]; then echo "erreur : il faut au moins 3 captures de la fonctionnalité (règle de l'utilisateur)"; exit 1; fi
