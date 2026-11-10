#!/usr/bin/env bash
# WaForSpeed — job « heure » des bancs du matin (matin.yml) : faut-il lancer les bancs à ce réveil ? (WFS-340, WFS-384)
# GitHub ne connaît que l'heure UTC et ses réveils programmés arrivent parfois des heures en retard, ou pas du tout
# (2026-11-09 : premier réveil à 09:44 heure de Paris ; 2026-11-10 : aucun avant 08:40). WFS-384 (RG-01, RG-03) :
#  - lancement à la main : toujours (il compte comme le banc du jour) ;
#  - réveil programmé avant 04:00 heure de Paris : rien ;
#  - réveil programmé à partir de 04:00 : bancs lancés s'il n'y a pas encore eu de banc ce jour-là (date de Paris), QUELLE QUE
#    SOIT L'HEURE (avant WFS-384 : seulement jusqu'à 10:59, d'où des jours entiers sans banc) ; sinon rien : jamais deux bancs
#    le même jour ;
#  - « retard=oui » quand les bancs partent après 07:00 (heure de la recette QA) : matin.yml prévient la QA (RG-06).
# Un run du jour « a lancé les bancs » si son job « compilation » a tourné ou tourne encore (ni absent ni sauté). Pour ménager le
# quota de l'API, un run terminé en moins de 2 minutes (réveil sauté : seul ce job a tourné) est écarté sans lire ses jobs.
# Entrées : EVENEMENT (github.event_name), RUN_ID, GH_REPO ; MAINTENANT (secondes depuis 1970, essais seulement).
# Sorties (GITHUB_OUTPUT, ou la sortie standard) : lancer=oui|non, retard=oui|non. Code 1 si la liste des runs est illisible.
set -uo pipefail
SORTIE="${GITHUB_OUTPUT:-/dev/stdout}"
maintenant="${MAINTENANT:-$(date +%s)}"
h=$(TZ=Europe/Paris date -d "@$maintenant" +%-H); jour=$(TZ=Europe/Paris date -d "@$maintenant" +%F)
ecrire() { printf 'lancer=%s\nretard=%s\n' "$1" "$2" >> "$SORTIE"; echo "$3"; exit 0; }

if [ "${EVENEMENT:?}" != schedule ]; then ecrire oui non "Lancement à la main ($h h à Paris) : bancs lancés"; fi
if [ "$h" -lt 4 ]; then ecrire non non "Il est $h h à Paris : avant 04:00, rien à faire"; fi

runs=$(gh api "repos/$GH_REPO/actions/workflows/matin.yml/runs?branch=main&per_page=50" \
       --jq '.workflow_runs[] | [.id, .created_at, .status, (.run_started_at // .created_at), .updated_at] | @tsv') \
    || { echo "::error::liste des runs du matin illisible"; exit 1; }
deja=""
while IFS=$'\t' read -r id cree etat debut fin; do
    [ -n "$id" ] && [ "$id" != "${RUN_ID:?}" ] || continue
    [ "$(TZ=Europe/Paris date -d "$cree" +%F)" = "$jour" ] || continue
    if [ "$etat" = completed ] && [ $(( $(date -d "$fin" +%s) - $(date -d "$debut" +%s) )) -lt 120 ]; then continue; fi
    c=$(gh api "repos/$GH_REPO/actions/runs/$id/jobs?per_page=100" \
        --jq '[.jobs[] | select(.name == "compilation") | (.conclusion // .status)] | first // ""') \
        || { echo "::error::jobs du run $id illisibles"; exit 1; }
    case "$c" in ''|skipped) ;; *) deja="$id ($c)"; break ;; esac
done <<< "$runs"
if [ -n "$deja" ]; then ecrire non non "Bancs du matin déjà lancés aujourd'hui ($jour) : run $deja, rien à faire"; fi
retard=non; [ "$h" -ge 7 ] && retard=oui
ecrire oui "$retard" "Bancs du matin lancés ($h h à Paris, premier réveil du $jour$([ $retard = oui ] && echo ', en retard sur la recette de 07:00'))"
