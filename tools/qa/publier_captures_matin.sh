#!/usr/bin/env bash
# WaForSpeed — captures des bancs du matin dans la Release « captures-matin » (WFS-352, RG-01), appelé par matin.yml.
# Pourquoi : la Release « apercu » a atteint la limite de GitHub (1 000 fichiers par Release, run 946) et ~1 000 envois un par un
# dépassent le quota horaire du GITHUB_TOKEN (commun à tous les jobs du dépôt). Ici : UNE archive zip par groupe du banc
# (captures-<groupe>.zip, ≈ 17 fichiers) + « jour.txt » (date, run, commit, groupes et nombre d'images) : ≈ 20 envois.
# La QA télécharge l'archive du groupe voulu (adresse fixe …/releases/download/captures-matin/captures-<groupe>.zip) et la décompresse.
# Une archive d'un groupe absent du run est retirée : aucune capture périmée n'est présentée comme du jour (RG-01).
# Usage : bash tools/qa/publier_captures_matin.sh <dossier des artefacts captures-*>   (GH_TOKEN, GH_REPO, GITHUB_SHA,
#         GITHUB_RUN_NUMBER, GROUPES = groupes attendus, séparés par des espaces ; ESSAI=1 : rien n'est envoyé)
# Code de sortie : 0 si tous les groupes attendus sont publiés ; 1 sinon (groupe manquant ou envoi raté) : le message
# « Recette prête » le dit (RG-02).
set -uo pipefail
DL="${1:?dossier des artefacts manquant}"
TAG=captures-matin
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
manque=0
{
    echo "Captures des bancs du matin — run ${GITHUB_RUN_NUMBER:-?}, commit ${GITHUB_SHA:-?}, $(TZ=Europe/Paris date '+%Y-%m-%d %H:%M') heure de Paris"
    echo "Une archive par groupe : captures-<groupe>.zip (images et rapports du banc)."
} > "$TMP/jour.txt"
for g in ${GROUPES:?liste des groupes attendus manquante}; do
    d="$DL/captures-$g"
    n=$(find "$d" -type f 2>/dev/null | wc -l)
    if [ "$n" -eq 0 ]; then
        echo "::error::groupe $g : aucune capture (banc en échec ou artefact absent)"
        echo "- $g : ABSENT" >> "$TMP/jour.txt"; manque=1; continue
    fi
    (cd "$d" && zip -q -r -X "$TMP/captures-$g.zip" .) || { echo "::error::archive du groupe $g impossible"; manque=1; continue; }
    echo "- $g : $n fichier(s), $(du -h "$TMP/captures-$g.zip" | cut -f1)" >> "$TMP/jour.txt"
done
cat "$TMP/jour.txt"
[ "${ESSAI:-0}" = 1 ] && { echo "ESSAI=1 : rien n'est envoyé"; ls -l "$TMP"; exit "$manque"; }

gh release view "$TAG" > /dev/null 2>&1 || gh release create "$TAG" --prerelease --target "${GITHUB_SHA:-main}" \
    --title "Captures des bancs du matin" \
    --notes "Une archive par groupe du banc d'essai PC, remplacées chaque matin (WFS-352). Voir jour.txt (date, run, commit)." \
    || { echo "::error::Release $TAG impossible à créer"; exit 1; }
envoi=0
for f in "$TMP"/captures-*.zip "$TMP/jour.txt"; do
    [ -e "$f" ] || continue
    ok=0
    for attente in 0 15 60; do
        sleep "$attente"
        gh release upload "$TAG" "$f" --clobber && { ok=1; break; }
    done
    [ "$ok" = 1 ] || { echo "::error::envoi de $(basename "$f") raté"; envoi=1; }
done
# archives d'un groupe absent de ce run : retirées (jamais une capture d'un autre jour présentée comme du jour)
gh release view "$TAG" --json assets --jq '.assets[].name' 2>/dev/null | while read -r nom; do
    case "$nom" in captures-*.zip) [ -e "$TMP/$nom" ] || { gh release delete-asset "$TAG" "$nom" --yes && echo "archive retirée : $nom"; } ;; esac
done
{
    echo "### Captures du matin (Release \`$TAG\`)"
    sed 's/^/    /' "$TMP/jour.txt"
} >> "${GITHUB_STEP_SUMMARY:-/dev/null}"
[ "$manque" = 0 ] && [ "$envoi" = 0 ]
