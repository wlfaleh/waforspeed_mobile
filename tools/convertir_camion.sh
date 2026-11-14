#!/usr/bin/env bash
# WaForSpeed — WFS-542 (ANO-273) : camion en travers de la Poursuite, vrai camion porteur à caisse à l'échelle 1.
# Source (hors du dépôt, clone partiel en lecture seule, rien exécuté) : « '90 Light Commercial Truck - Low poly model » (aussi
# publié « LCT 3000 '95 ») de Daniel Zhabotinsky, CC BY 4.0, dépôt public MiguelJordans/LAPR5,
# Frontend/WebUI/ElectricGo/src/assets/truck (licence et SHA-256 : assets/licences/camion-poursuite.txt).
# Échelle : longueur 6,19 m, déduite de son pneu 215/75 R17.5 (767 mm) : references/reel/camion_poursuite.json.
# Badges (marque inventée « LCT ») retirés ; plaques américaines remplacées par une plaque neutre au format 520 × 110 mm ;
# support de la plaque avant retiré (trop petit pour la plaque européenne). Puis versions allégées (.lod).
# usage : CAMION=<dossier>/scene.gltf bash tools/convertir_camion.sh
set -euo pipefail
ICI=$(cd "$(dirname "$0")" && pwd)
: "${CAMION:?CAMION=<dossier du modèle>/scene.gltf}"
mkdir -p "$ICI/../assets/world/camion"
cd "$ICI"
WFS_EXCLURE_NOEUD='^(Badge_|Numberplate_Front_UCB)' WFS_PLAQUE='^Numberplates$' WFS_TEXTURES='Numberplates|couleur=plaques/camion.png' \
  node convert_car.mjs "$CAMION" ../assets/world/camion/porteur.wfm 6.19 auto '^bodycolor$'
node alleger_voiture.mjs ../assets/world/camion/porteur.wfm ../assets/world/camion/porteur.lod
