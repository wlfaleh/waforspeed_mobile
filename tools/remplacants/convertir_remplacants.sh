#!/bin/bash
# WaForSpeed — WFS-397 : conversion des remplaçants de l'A 45 et de l'Elantra N (voitures sans marque, ±1 %), dans
# assets/cars/remplacants/<id>.wfm + .lod. Rien n'est branché dans le jeu (WFS-360). Outils du dépôt lus, pas modifiés.
# Sources (clone partiel hors du dépôt, rien exécuté ; licences, adresses et SHA-256 : assets/licences/remplacants/<id>.txt) :
#   W212=<dossier>/scene.gltf   « Mercedes E Class W212 » de Peter_D (CC BY 4.0), dépôt bujue600-arch/carVison
#   VIRTUS=<dossier>/scene.gltf « 2022 Volkswagen Virtus GT » de BHP3D (CC BY 4.0), dépôt PrathamSatpute/VehiclePredictiveMaintenance
# usage (depuis la racine du dépôt) : W212=… VIRTUS=… bash tools/remplacants/convertir_remplacants.sh [w212] [virtus]
# Étapes, pour chaque voiture :
#   0. repeindre_textures.mjs : logos imprimés dans les textures repeints, sur une copie de la source (textures.json) ;
#   1. tools/simplifier_voiture.mjs à erreur très faible (soudure, presque sans perte : la carrosserie reste lisse ; à 0,002 et
#      même à 0,0004 elle sortait froissée) ; l'allègement se fait à l'étape 2, matériau par matériau, hors carrosserie ;
#   2. tools/convert_car.mjs : pièces de logo séparées retirées par nom de nœud, habitacle et roues allégés, textures ≤ 640 px (8 Mo) ;
#   3. relever_caisse.mjs : caisse remontée par rapport aux roues (modèles posés bas, RG-01 / RG-02) ;
#   4. build/corriger_voitures sur une racine temporaire (référentiel + fiches de WFS-396) : échelle globale et assise ;
#   5. retirer_pieces.mjs : pièces de logo prises dans un matériau partagé (pieces.json) ;
#   6. tools/alleger_voiture.mjs : niveaux de détail (.lod).
set -euo pipefail
ICI="$(cd "$(dirname "$0")" && pwd)"; RACINE="$(cd "$ICI/../.." && pwd)"; OUTILS="$RACINE/tools"
SORTIE="$RACINE/assets/cars/remplacants"; mkdir -p "$SORTIE"
CORR="$RACINE/build/corriger_voitures"
[ -x "$CORR" ] || { echo "⚠️  $CORR absent : ninja -C build corriger_voitures" >&2; exit 1; }
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
# racine temporaire pour corriger_voitures : référentiel du dépôt + fiches réelles des remplaçants (WFS-396), voitures = SORTIE
mkdir -p "$TMP/racine/references" "$TMP/racine/assets"
cp -r "$RACINE/references/reel" "$TMP/racine/references/reel"
ln -s "$SORTIE" "$TMP/racine/assets/cars"
node -e '
const fs = require("fs"); const [p, q] = process.argv.slice(1);
const v = JSON.parse(fs.readFileSync(p, "utf8")), r = JSON.parse(fs.readFileSync(q, "utf8"));
v.mesures.push(...r.mesures); Object.assign(v.sources, r.sources); fs.writeFileSync(p, JSON.stringify(v));
' "$TMP/racine/references/reel/voitures.json" "$RACINE/references/reel/voitures/remplacants-wfs396.json"

# id | variable de la source | longueur (m) | rotateY | peinture | feux arrière | erreur de simplification | relèvement (mm)
VOITURES=(
  "w212|W212|4.868|auto|^mat_body\$|^mat_lights_glass2\$|0.00002|40"
)
declare -A EXCLURE_NOEUD=( [w212]='^mesh_Hood_mat_chrome_0$' )   # étoile du capot (pièce à part)
declare -A ALLEGER=( [w212]='mat_wheels=0.2! ; mat_rubber=0.25 ; mat_salon=0.12 ; mat_bottom=0.15 ; mat_inside_panel=0.15 ; mat_leather=0.15 ; mat_plastic=0.3 ; mat_inside_gauges=0.25 ; mat_tires=0.4 ; mat_brakes=0.25 ; mat_lights=0.4 ; mat_lights_glass=0.6' )
declare -A TEXTURE_MAX=( [w212]=640 )

cd "$OUTILS"
for v in "${VOITURES[@]}"; do
  IFS='|' read -r id var L rot peinture feux erreur mm <<< "$v"
  if [ $# -gt 0 ] && [[ ! " $* " =~ " $id " ]]; then continue; fi
  src="${!var:-}"; [ -f "$src" ] || { echo "⚠️  $id : source absente ($var=<dossier>/scene.gltf)" >&2; exit 1; }
  echo "== $id"
  node "$ICI/repeindre_textures.mjs" "$id" "$(dirname "$src")" "$TMP/$id-source"
  node simplifier_voiture.mjs "$TMP/$id-source/$(basename "$src")" "$TMP/$id.glb" "$peinture" "$erreur"
  WFS_EXCLURE_NOEUD="${EXCLURE_NOEUD[$id]:-}" WFS_ALLEGER="${ALLEGER[$id]:-}" WFS_TEXTURE_MAX="${TEXTURE_MAX[$id]:-1024}" \
    node convert_car.mjs "$TMP/$id.glb" "$SORTIE/$id.wfm" "$L" "$rot" "$peinture" "$feux"
  node "$ICI/relever_caisse.mjs" "$SORTIE/$id.wfm" "$SORTIE/$id.wfm" "$mm"
  "$CORR" "$TMP/racine" "$id"
  node "$ICI/retirer_pieces.mjs" "$id" "$SORTIE/$id.wfm" "$SORTIE/$id.wfm"
  WFS_DETAIL= "$CORR" "$TMP/racine" "$id" --essai
  node alleger_voiture.mjs "$SORTIE/$id.wfm" "$SORTIE/$id.lod" 0.3
done
