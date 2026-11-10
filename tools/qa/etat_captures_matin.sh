#!/usr/bin/env bash
# WaForSpeed — état de la Release « captures-matin » pour la QA (WFS-384, RG-06), lu par recette_prete.sh et retard_matin.sh.
# Lit « jour.txt » (écrit par publier_captures_matin.sh : « Captures des bancs du matin — run N, commit …, AAAA-MM-JJ hh:mm heure
# de Paris ») à son adresse publique (dépôt public : hors quota de l'API) et le compare à la date du jour à Paris.
# Sortie : une phrase pour la QA (où lire les captures). Code : 0 = captures du jour ; 1 = captures d'un autre jour ;
# 2 = Release absente ou jour.txt illisible.
# Entrées : GH_REPO ; GITHUB_SERVER_URL ; MAINTENANT (secondes depuis 1970, essais) ; JOUR_TXT (fichier local, essais : « absent »
# simule une Release absente).
set -uo pipefail
SERVEUR="${GITHUB_SERVER_URL:-https://github.com}"
REL="$SERVEUR/$GH_REPO/releases/tag/captures-matin"
ANCIEN="$SERVEUR/$GH_REPO/releases/tag/apercu-ancien-20261109-0925"   # captures d'avant WFS-352 (jusqu'au 2026-11-09 02:52)
aujourdhui=$(TZ=Europe/Paris date -d "@${MAINTENANT:-$(date +%s)}" +%F)
if [ -n "${JOUR_TXT:-}" ]; then
    [ "$JOUR_TXT" != absent ] && ligne=$(head -n 1 "$JOUR_TXT" 2>/dev/null) || ligne=""
else
    ligne=$(curl -fsSL --retry 2 --max-time 30 "$SERVEUR/$GH_REPO/releases/download/captures-matin/jour.txt" 2>/dev/null | head -n 1) || ligne=""
fi
date_captures=$(grep -oE '[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}' <<< "$ligne" | tail -n 1)
if [ -z "$date_captures" ]; then
    echo "la Release captures-matin n'existe pas encore ou n'a pas de jour.txt : aucune capture du matin publiée ; les dernières captures sont dans $ANCIEN (jusqu'au 09/11/2026 02:52)"
    exit 2
fi
if [ "${date_captures%% *}" = "$aujourdhui" ]; then
    echo "captures du jour ($date_captures heure de Paris) dans $REL (captures-<groupe>.zip ; jour.txt)"
    exit 0
fi
echo "PAS les captures du jour : $REL contient encore celles du $date_captures (heure de Paris)"
exit 1
