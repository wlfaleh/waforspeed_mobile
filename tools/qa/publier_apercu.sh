#!/usr/bin/env bash
# WaForSpeed — publication de la Release d'aperçu (ticket WFS-87, appelé par le job « publication » de build.yml).
# Main : Release « apercu » ; branche feature/XX : « apercu-XX » (claude/XX : idem). Adresses inchangées pour la QA :
#   https://github.com/<dépôt>/releases/download/<tag>/WaForSpeed.apk et …/<capture>.png
# Règles :
#   RG-01 jamais de moment sans APK : la Release n'est plus supprimée ; le nouvel APK est téléversé sous un nom temporaire,
#         puis remplace l'ancien (suppression + renommage, deux appels courts) ; le tag est ensuite déplacé sur le commit ;
#   RG-02 l'APK (et l'AAB) d'abord, les captures ensuite ;
#   RG-03 chaque appel à l'API retenté 4 fois (15, 30, 60, 120 s : limite secondaire 403, 429, erreurs 5xx) ;
#   RG-04 une seule lecture de la Release au début, captures téléversées par lots ;
#   RG-05 jamais remplacer une Release faite d'un commit plus récent (les builds de main tournent en parallèle) ;
#   RG-06 APK non publié : échec (job rouge) ; captures non publiées : avertissement, job vert.
# Entrées : GH_TOKEN, GITHUB_REF_NAME, GITHUB_SHA, GITHUB_RUN_NUMBER ; dossiers dl/apk et dl/captures (artefacts).
set -uo pipefail

APK_DIR="${APK_DIR:-dl/apk}"
CAPTURES_DIR="${CAPTURES_DIR:-dl/captures}"
LOT="${LOT:-20}"                          # captures par appel de téléversement
ATTENTES=(15 30 60 120)                   # RG-03
[ -n "${ATTENTES_ESSAI:-}" ] && read -r -a ATTENTES <<< "$ATTENTES_ESSAI"   # essais locaux : attentes courtes

# ---------------------------------------------------------------- outils
essayer() {   # essayer <description> <commande…> : 1 essai + 4 nouvelles tentatives
    local desc="$1"; shift
    local n=0
    while true; do
        if "$@"; then return 0; fi
        if [ "$n" -ge "${#ATTENTES[@]}" ]; then echo "ÉCHEC : $desc (après $n nouvelles tentatives)"; return 1; fi
        echo "nouvelle tentative $((n + 1))/${#ATTENTES[@]} dans ${ATTENTES[$n]} s : $desc"
        sleep "${ATTENTES[$n]}"; n=$((n + 1))
    done
}

tag=apercu
case "$GITHUB_REF_NAME" in feature/*) tag="apercu-${GITHUB_REF_NAME#feature/}";; claude/*) tag="apercu-${GITHUB_REF_NAME#claude/}";; esac
titre="Aperçu — construction ${GITHUB_RUN_NUMBER}"
notes="APK Android de test (arm64). Installer : télécharger WaForSpeed.apk sur le téléphone, autoriser l'installation depuis cette source. Branche ${GITHUB_REF_NAME}, commit ${GITHUB_SHA::7}."
echo "Release $tag pour ${GITHUB_SHA::7} (construction ${GITHUB_RUN_NUMBER})"

# fichiers à publier : APK (+ AAB) puis captures non vides (une capture ratée n'empêche pas la publication)
[ -s "$APK_DIR/WaForSpeed.apk" ] || { echo "::error::$APK_DIR/WaForSpeed.apk absent ou vide"; exit 1; }
paquets=("$APK_DIR/WaForSpeed.apk")
[ -s "$APK_DIR/WaForSpeed.aab" ] && paquets+=("$APK_DIR/WaForSpeed.aab")   # WFS-22 : AAB signé (Google Play)
captures=()
if [ -d "$CAPTURES_DIR" ]; then
    for f in "$CAPTURES_DIR"/*.png "$CAPTURES_DIR"/*.txt; do [ -s "$f" ] && captures+=("$f"); done
fi
echo "${#paquets[@]} paquet(s) et ${#captures[@]} capture(s) à publier"

# ---------------------------------------------------------------- RG-04 : une seule lecture de la Release
existe=0; cible=""; anciens=""
lire_release() {
    local sortie code
    sortie=$(gh release view "$tag" --json targetCommitish,assets -q '.targetCommitish + "\n" + ([.assets[].name] | join("\n"))' 2>&1); code=$?
    if [ $code -eq 0 ]; then existe=1; cible=$(head -n1 <<< "$sortie"); anciens=$(tail -n +2 <<< "$sortie"); return 0; fi
    if grep -qiE "release not found|HTTP 404" <<< "$sortie"; then existe=0; return 0; fi   # pas encore de Release
    echo "$sortie"; return 1   # 403, 429, 5xx… : nouvelle tentative
}
essayer "lecture de la Release $tag" lire_release || { echo "::error::Release $tag illisible"; exit 1; }

# RG-05 : une Release déjà faite d'un commit plus récent n'est jamais remplacée
if [ "$existe" = 1 ] && [ -n "$cible" ] && [ "$cible" != "$GITHUB_SHA" ] && git merge-base --is-ancestor "$GITHUB_SHA" "$cible" 2>/dev/null; then
    echo "La Release $tag vient déjà d'un commit plus récent ($cible) : rien à publier pour ${GITHUB_SHA::7}"; exit 0
fi

# ---------------------------------------------------------------- RG-01 / RG-02 : l'APK d'abord, sans moment sans APK
if [ "$existe" = 0 ]; then
    # première publication (ou Release disparue) : création avec les seuls paquets
    essayer "création de la Release $tag avec l'APK" \
        gh release create "$tag" "${paquets[@]}" --prerelease --target "$GITHUB_SHA" --title "$titre" --notes "$notes" \
        || { echo "::error::APK non publié (création de la Release $tag impossible)"; exit 1; }
    echo "APK publié : Release $tag créée"
else
    tmp=$(mktemp -d)
    for p in "${paquets[@]}"; do
        nom=$(basename "$p"); provisoire="${nom%.*}-${GITHUB_SHA::7}.${nom##*.}"
        cp "$p" "$tmp/$provisoire"
        # 1) le nouveau paquet sous un nom provisoire (long : l'ancien reste téléchargeable pendant ce temps)
        essayer "téléversement de $provisoire" gh release upload "$tag" "$tmp/$provisoire" --clobber \
            || { echo "::error::$nom non publié"; exit 1; }
        # 2) l'ancien supprimé, 3) le nouveau renommé (deux appels courts)
        if grep -qx "$nom" <<< "$anciens"; then
            essayer "suppression de l'ancien $nom" gh release delete-asset "$tag" "$nom" -y || { echo "::error::ancien $nom non remplacé"; exit 1; }
        fi
        id_nouveau() { gh api "repos/{owner}/{repo}/releases/tags/$tag" -q ".assets[] | select(.name == \"$provisoire\") | .id"; }
        renommer() { local id; id=$(id_nouveau) && [ -n "$id" ] && gh api -X PATCH "repos/{owner}/{repo}/releases/assets/$id" -f name="$nom" > /dev/null; }
        essayer "renommage de $provisoire en $nom" renommer || { echo "::error::$nom non publié (renommage impossible)"; exit 1; }
        echo "$nom publié (remplacé)"
    done
    rm -rf "$tmp"
    # tag déplacé sur le nouveau commit, titre et notes à jour (mêmes textes qu'avant)
    deplacer_tag() { git tag -f "$tag" "$GITHUB_SHA" && git push -f origin "refs/tags/$tag"; }
    essayer "déplacement du tag $tag" deplacer_tag || echo "::warning::tag $tag non déplacé sur ${GITHUB_SHA::7}"
    essayer "mise à jour de la Release $tag" gh release edit "$tag" --target "$GITHUB_SHA" --title "$titre" --notes "$notes" --prerelease \
        || echo "::warning::titre et notes de la Release $tag non mis à jour"
fi

# ---------------------------------------------------------------- captures, par lots (RG-04) ; échec = avertissement (RG-06)
ratees=()
for ((i = 0; i < ${#captures[@]}; i += LOT)); do
    lot=("${captures[@]:i:LOT}")
    if ! essayer "captures $((i + 1)) à $((i + ${#lot[@]}))" gh release upload "$tag" "${lot[@]}" --clobber; then
        for f in "${lot[@]}"; do ratees+=("$(basename "$f")"); done
    fi
done
# captures d'une ancienne construction qui n'existent plus : retirées (elles tromperaient la QA)
if [ "$existe" = 1 ] && [ -n "$anciens" ]; then
    declare -A garde=()
    for f in "${paquets[@]}" "${captures[@]}"; do garde["$(basename "$f")"]=1; done
    while IFS= read -r a; do
        [ -z "$a" ] && continue
        case "$a" in WaForSpeed-*.apk|WaForSpeed-*.aab) ;; *) [ -n "${garde[$a]:-}" ] && continue;; esac
        essayer "retrait de l'ancienne capture $a" gh release delete-asset "$tag" "$a" -y || echo "::warning::$a (ancienne) non retirée"
    done <<< "$anciens"
fi
if [ ${#ratees[@]} -gt 0 ]; then
    echo "::warning::${#ratees[@]} capture(s) non publiée(s) : ${ratees[*]}"
fi
echo "Publication terminée : Release $tag, APK en ligne, $(( ${#captures[@]} - ${#ratees[@]} ))/${#captures[@]} capture(s)"
