#!/usr/bin/env bash
# WaForSpeed — publication des paquets de cartes (WFS-302), appelée par le job « publication » de paquets.yml.
# Entrées : deux dossiers de cuisson indépendante (CUISSON_A, CUISSON_B : <carte>_<format>.wfcarte), GH_TOKEN, GH_REPO, GITHUB_SHA,
#           VERSION_MIN_APK, PUBLIER (oui : envoie dans la Release ; non : vérifie seulement, essai local ou branche).
# Règles :
#   RG-01 : les deux cuissons ont les mêmes fichiers, identiques octet par octet ; sinon rouge, rien n'est publié ;
#   RG-02 : Release à étiquette fixe « paquets » (créée si absente, jamais supprimée), GITHUB_TOKEN seulement ;
#   RG-03 : cartes.json écrit par tools/qa/cartes_json.py, adresses directes (pas l'API) ; noms publiés versionnés par le SHA-256
#           (<carte>_<format>-<sha12>.wfcarte) : un contenu nouveau a une adresse nouvelle ;
#   ordre : nouveaux paquets d'abord, cartes.json ensuite (la liste ne montre jamais un fichier absent), puis retrait des paquets
#           qui ne sont ni dans la nouvelle liste ni dans la précédente (un téléphone en plein téléchargement de l'ancienne liste finit
#           son fichier) ; mêmes paquets que la liste publiée → rien n'est envoyé ;
#   RG-05 : résumé (taille de chaque fichier, poids de l'APK d'aperçu, lien de cartes.json) dans GITHUB_STEP_SUMMARY.
set -euo pipefail

A="${CUISSON_A:?}"; B="${CUISSON_B:?}"
TAG=paquets
SERVEUR="${GITHUB_SERVER_URL:-https://github.com}"
BASE="$SERVEUR/${GH_REPO:?}/releases/download/$TAG"
RESUME="${GITHUB_STEP_SUMMARY:-/dev/stdout}"
PUBLIER="${PUBLIER:-non}"
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
ICI=$(cd "$(dirname "$0")" && pwd)

# RG-01 : deux cuissons identiques
la=$(find "$A" -maxdepth 1 -name "*.wfcarte" -printf "%f\n" | sort); lb=$(find "$B" -maxdepth 1 -name "*.wfcarte" -printf "%f\n" | sort)
[ -n "$la" ] || { echo "::error::aucun paquet dans la cuisson A ($A)"; exit 1; }
[ "$la" = "$lb" ] || { echo "::error::les deux cuissons n'ont pas les mêmes fichiers"; echo "A : $la"; echo "B : $lb"; exit 1; }
for f in $la; do
    cmp -s "$A/$f" "$B/$f" || { echo "::error::deux cuissons différentes : $f (RG-01)"; exit 1; }
done
echo "Deux cuissons identiques octet par octet : $(echo "$la" | wc -l) fichier(s)"

# RG-03 : liste
python3 "$ICI/cartes_json.py" "$A" "$TMP/cartes.json" --adresse-base "$BASE" --version-min-apk "${VERSION_MIN_APK:?}" \
    --commit "${GITHUB_SHA:-}" --nom ville=Ville --nom plage=Corniche --versionne --publies "$TMP/publies.txt"
cp "$TMP/cartes.json" "${SORTIE_LISTE:-cartes.json}"

# liste publiée aujourd'hui (adresse publique, hors quota de l'API) ; absente → première publication
curl -fsSL --max-time 60 "$BASE/cartes.json" -o "$TMP/ancienne.json" 2>/dev/null || : > "$TMP/ancienne.json"
paquets() { python3 -c 'import json,sys
try: print(json.dumps(json.load(open(sys.argv[1]))["paquets"], sort_keys=True))
except Exception: print("")' "$1"; }
nouveau=oui
[ -s "$TMP/ancienne.json" ] && [ "$(paquets "$TMP/ancienne.json")" = "$(paquets "$TMP/cartes.json")" ] && nouveau=non

{
    echo "## Paquets de cartes (WFS-302)"
    echo
    echo "| Fichier publié | Taille | SHA-256 |"
    echo "|---|---|---|"
    while read -r local publie; do
        echo "| \`$publie\` | $(python3 -c "print(f'{$(stat -c %s "$A/$local")/1048576:.2f} Mo')") | \`$(sha256sum "$A/$local" | cut -c1-16)…\` |"
    done < "$TMP/publies.txt"
    echo
    echo "Liste : $BASE/cartes.json"
    if [ "$nouveau" = non ]; then echo; echo "Mêmes paquets que la liste déjà publiée : rien n'est envoyé."; fi
} >> "$RESUME"

[ "$PUBLIER" = oui ] || { echo "PUBLIER=$PUBLIER : vérification seulement, rien n'est envoyé"; exit 0; }
[ "$nouveau" = oui ] || { echo "Mêmes paquets que la liste publiée : rien à envoyer"; exit 0; }

# RG-02 : Release « paquets » (jamais supprimée)
gh release view "$TAG" > /dev/null 2>&1 || gh release create "$TAG" --prerelease --target "${GITHUB_SHA:-main}" --title "Paquets de cartes" \
    --notes "Paquets de cartes téléchargés par le jeu (WFS-302). Liste : cartes.json. Fichiers publiés par la CI (paquets.yml), ne pas modifier à la main."

mkdir -p "$TMP/envoi"
while read -r local publie; do cp "$A/$local" "$TMP/envoi/$publie"; done < "$TMP/publies.txt"
gh release upload "$TAG" "$TMP"/envoi/*.wfcarte --clobber
gh release upload "$TAG" "$TMP/cartes.json" --clobber
echo "Paquets puis cartes.json envoyés"

# retrait des paquets qui ne sont ni dans la nouvelle liste ni dans la précédente
garder=$( { cut -d' ' -f2 "$TMP/publies.txt"; python3 -c 'import json,sys
try: [print(p["adresse"].rsplit("/",1)[1]) for p in json.load(open(sys.argv[1]))["paquets"]]
except Exception: pass' "$TMP/ancienne.json"; } | sort -u)
gh release view "$TAG" --json assets --jq '.assets[].name' | while read -r nom; do
    case "$nom" in *.wfcarte) ;; *) continue ;; esac
    grep -qxF "$nom" <<< "$garder" || { gh release delete-asset "$TAG" "$nom" --yes && echo "Ancien paquet retiré : $nom"; }
done

# RG-05 : poids de l'APK d'aperçu (aucune carte dedans : les paquets ne sont que dans cette Release)
apk=$(gh release view apercu --json assets --jq '.assets[] | select(.name == "WaForSpeed.apk") | .size' 2>/dev/null || true)
[ -n "$apk" ] && echo "APK d'aperçu (Release apercu) : $(python3 -c "print(f'{$apk/1048576:.1f} Mo')"), sans paquet de carte" >> "$RESUME"
echo "Publié : $BASE/cartes.json" >> "$RESUME"
