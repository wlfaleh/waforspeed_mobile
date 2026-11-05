#!/bin/bash
# WaForSpeed — conversion des 16 voitures du jeu web ajoutées par WFS-13 (golf, m4, p911, gallardo : déjà en .wfm).
# Valeurs reprises de js/cars/catalog.js : longueur (CARS[].d.L), rotateY / paint / tail (CAR_MODELS).
# usage : tools/convertir_voitures.sh <chemin de waaps>/waforspeed [id …]   (sans id : les 16 voitures)
# Rejouable : réécrit assets/cars/<id>.wfm et assets/licences/<id>.txt. WFS-77 : peinture unie (toutes les voitures
# reconverties), roues reconnues à leur forme pour la GT86 (colonne « roues »).
set -e
WEB="${1:?chemin du dossier waforspeed du jeu web}"; shift || true
ICI="$(cd "$(dirname "$0")" && pwd)"; RACINE="$ICI/.."
# id | dossier | longueur (m) | rotateY | motif de peinture | roues (WFS-77 : « forme ») | motif des feux arrière (vide = défaut ;
# dernière colonne : il peut contenir des « | »)
VOITURES=(
  "p208|peugeot-208|4.06|180|^Carro_Pintura$||"
  "abarth|abarth-500|3.66|180|^CAR_Livrea$||"
  "tcross|vw-t-cross|4.11|180|^carpaint$||"
  "compass|jeep-compass|4.40|180|^00_-_BODY$||"
  "s3|audi-a3|4.47|180|^carpaint$||"
  "x5|bmw-x5|4.92|180|^CARPAINT$||"
  "giulia|alfa-giulia|4.64|178.16|^EXT_carpaint$||"   # WFS-77 : modèle d'origine tourné de 1,84° (essieux de travers)
  "a45|mercedes-a45|4.36|0|^Material\.004$||^Material\.013$"
  "model3|tesla-model-3|4.69|180|^primary$||^breaklight_l$"
  "corsa|opel-corsa|4.06|180|^body$||^(r_glass|r_lights)$"
  "gt86|toyota-gt86|4.24|180|^body\.001$|forme|"
  "elantra|hyundai-elantra-n|4.68|180|^HHyundai_ElantraNRewardRecycled_2022Paint_Material1$||"
  "v60|volvo-v60|4.63|0|^body_paint$||^lamp_backglass$"
  "rrsport|range-rover-sport|4.95|180|^2023_Land_Rover_Range_Rover_Sport_Firenze_Red$||"
  "mustang|ford-mustang|4.82|180|^fFord_MustangShelbyGT500RewardRecycled_2019Paint_Material1$||"
  "sf90|ferrari-sf90|4.85|180|^PLASTIC_INT_10$||"
)
cd "$ICI"
for v in "${VOITURES[@]}"; do
  IFS='|' read -r id dossier L rot peinture roues feux <<< "$v"
  export WFS_ROUES="$roues"
  if [ $# -gt 0 ] && [[ ! " $* " =~ " $id " ]]; then continue; fi
  echo "== $id ($dossier)"
  if [ -n "$feux" ]; then node convert_car.mjs "$WEB/models/$dossier/$dossier.json" "$RACINE/assets/cars/$id.wfm" "$L" "$rot" "$peinture" "$feux"
  else node convert_car.mjs "$WEB/models/$dossier/$dossier.json" "$RACINE/assets/cars/$id.wfm" "$L" "$rot" "$peinture"; fi
  cp "$WEB/models/$dossier/license.txt" "$RACINE/assets/licences/$id.txt"
done
# WFS-134 (RG-11, RG-12) : versions allégées (assets/cars/<id>.lod : indices seuls, 30 % des triangles au moins) des voitures
# du trafic, de la police et du camion de la poursuite ; vues au-delà de 60 m, avec fondu (Renderer::draw)
for id in golf m4 p911 gallardo p208 x5; do node alleger_voiture.mjs "$RACINE/assets/cars/$id.wfm" "$RACINE/assets/cars/$id.lod" 0.3; done
