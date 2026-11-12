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
  "tcross|suzuki-brezza|3.995|auto|^primary$||^(left|right)_rear_light$"   # WFS-359 : Suzuki Vitara Brezza 2022 (id gardé), voir BREZZA
  "compass|jeep-compass|4.40|180|^00_-_BODY$||"
  "s3|audi-a3|4.47|180|^carpaint$||"
  "x5|bmw-x5|4.92|180|^CARPAINT$||"
  "giulia|alfa-giulia|4.64|178.16|^EXT_carpaint$||"   # WFS-77 : modèle d'origine tourné de 1,84° (essieux de travers)
  "a45|mercedes-a45|4.36|0|^Material\.004$||^Material\.013$"
  "model3|tesla-2018-model-3|4.69|auto|^primary$||^(breaklight_l|left_rear_light|right_rear_light)$"   # WFS-158 (RG-D) : autre source, voir MODELE3
  "corsa|opel-corsa|4.06|180|^body$||^(r_glass|r_lights)$"
  "gt86|toyota-gt86|4.24|180|^body\.001$|forme|"
  "elantra|hyundai-elantra-n|4.68|180|^HHyundai_ElantraNRewardRecycled_2022Paint_Material1$||"
  "v60|volvo-v60|4.63|0|^body_paint$||^lamp_backglass$"
  "rrsport|range-rover-sport|4.95|180|^2023_Land_Rover_Range_Rover_Sport_Firenze_Red$||"
  "mustang|corvette-c7|4.493|180|^Car_Paint(_2)?$||^Tail_Lights_Red_Cover(_2)?$"   # WFS-359 : Chevrolet Corvette C7 (id gardé), voir CORVETTE
  "sf90|ferrari-sf90|4.85|180|^PLASTIC_INT_10$||"
)
# WFS-158 (RG-A, ANO-79) : plaques neutres de tools/plaques_neutres.mjs à la place des plaques au nom de l'auteur du modèle
# (V60, A 45, Range Rover Sport) ; plaques remises au format européen 520 × 110 mm (la V60 l'est déjà : 515 × 110 mm)
declare -A PLAQUE=( [a45]='^Material\.029$' [rrsport]='^material$' )
declare -A TEXTURES=( [v60]='plate_tex|couleur=plaques/v60.png|normale=plaques/normale-plate.png'
                      [a45]='Material.029|couleur=plaques/a45.png' [rrsport]='material|couleur=plaques/rrsport.png' )
# WFS-158 (RG-D, ordre de l'utilisateur du 2026-11-06 19:52) : la Model 3 vient d'un autre modèle, « Tesla 2018 Model 3 »
# d'Ameer Studio (Sketchfab, CC BY 4.0), lu dans le dépôt GitHub public tomfanhm/tesla, dossier public/gltf/tesla
# (scene.gltf SHA-256 80911d58…e3b5, scene.bin c87f3de6…18b7 ; méthode : .claude/skills/waforspeed-natif/references/
# modeles-3d-github.md). Il n'est pas dans le jeu web : MODELE3=<dossier>/scene.gltf (clone partiel, hors du dépôt) ; trop
# lourd (684 000 triangles), il est d'abord allégé par tools/simplifier_voiture.mjs (erreur bornée, peinture 4 fois plus fine).
# Sa licence (assets/licences/model3.txt) est écrite à la main (dépôt GitHub et modifications), pas recopiée.
declare -A SOURCE=( [model3]="${MODELE3:-}" [mustang]="${CORVETTE:-}" [tcross]="${BREZZA:-}" )
# WFS-359 (modèles CC BY 4.0 à la place des modèles CC BY-NC-SA de Ddiaz Design, décision du PO du 2026-11-09) ; lus dans des dépôts
# GitHub publics (clone partiel, hors du dépôt, rien exécuté), SHA-256 au journal du 2026-11-09 :
#  - mustang : « Chevrolet Corvette (C7) » de Martin Trafas, dépôt JoseJ55/car-showcase-v2, public/models/corvette_c7 :
#    CORVETTE=<dossier>/scene.gltf ; 1,07 M triangles allégés (erreur 0,002) ; emblèmes des jantes et de la caisse retirés (nœuds) ;
#  - tcross : « 2022 Maruti Suzuki Vitara Brezza » de BHP3D, dépôt kriteesh/threejs-model, model-2 : BREZZA=<dossier>/scene.gltf ;
#    478 000 triangles allégés (erreur 0,003), habitacle allégé (WFS_ALLEGER) ; feux arrière et phares sortis du matériau partagé avec les voyants
#    de l'habitacle (WFS_FEU, WFS_PHARE : noms des nœuds) ; barres de toit gardées (la hauteur de la fiche, 1685 mm, les compte).
declare -A ERREUR=( [mustang]=0.002 [tcross]=0.003 )
declare -A EXCLURE_NOEUD=( [mustang]='CorvetteWingEmblem' )
declare -A ALLEGER=( [mustang]='Reflectors=0.4' [tcross]='int_dark.0=0.3 ; wireless_cahrger_conso_le.2=0.3 ; cons_ac_alu.0=0.4 ; int_roof_pads.0=0.4 ; wireless_cahrger_conso_le.3=0.4 ; headsup_plas.0=0.5' )
declare -A FEU=( [tcross]='^(reverse brake|brake rear main|boot brake light|boot brk lit|rear fog)' )
# phares de la Brezza sortis du matériau partagé avec les voyants de l'habitacle (sinon ni phares reconnus ni halos à leur place)
declare -A PHARE=( [tcross]='^(frnt_lits_plas|signal front|fog front|front main lights|frnt_lightsss)' )
cd "$ICI"
for v in "${VOITURES[@]}"; do
  IFS='|' read -r id dossier L rot peinture roues feux <<< "$v"
  export WFS_ROUES="$roues" WFS_PLAQUE="${PLAQUE[$id]:-}" WFS_TEXTURES="${TEXTURES[$id]:-}" WFS_EXCLURE_NOEUD="${EXCLURE_NOEUD[$id]:-}" \
         WFS_ALLEGER="${ALLEGER[$id]:-}" WFS_FEU="${FEU[$id]:-}" WFS_PHARE="${PHARE[$id]:-}"
  if [ $# -gt 0 ] && [[ ! " $* " =~ " $id " ]]; then continue; fi
  echo "== $id ($dossier)"
  src="$WEB/models/$dossier/$dossier.json"
  if [ -n "${SOURCE[$id]+x}" ]; then
    [ -f "${SOURCE[$id]}" ] || { echo "⚠️  $id : source absente (MODELE3, CORVETTE ou BREZZA=<dossier>/scene.gltf, voir plus haut)" >&2; exit 1; }
    src="$(mktemp -d)/$id.glb"; node simplifier_voiture.mjs "${SOURCE[$id]}" "$src" "$peinture" ${ERREUR[$id]:-}
  fi
  if [ -n "$feux" ]; then node convert_car.mjs "$src" "$RACINE/assets/cars/$id.wfm" "$L" "$rot" "$peinture" "$feux"
  else node convert_car.mjs "$src" "$RACINE/assets/cars/$id.wfm" "$L" "$rot" "$peinture"; fi
  [ -n "${SOURCE[$id]+x}" ] || cp "$WEB/models/$dossier/license.txt" "$RACINE/assets/licences/$id.txt"
done
# WFS-158 (ST-2, RG-01, RG-02) : échelle globale, diamètre des roues, voies et assise d'après le référentiel du réel
# (references/reel/voitures.json) : tools/corriger_voitures.cpp, cible CMake « corriger_voitures » ; rejouable (une voiture
# déjà corrigée ne bouge plus). Sans id : les 20 voitures (golf, m4, p911 et gallardo comprises).
CORR="$RACINE/build/corriger_voitures"
if [ -x "$CORR" ]; then "$CORR" "$RACINE" "$@"
else echo "⚠️  $CORR absent : construire la cible corriger_voitures (ninja -C build corriger_voitures), puis la lancer" >&2; exit 1; fi
# WFS-158 (ST-3, M-04) : plaques européennes 520 × 110 mm des voitures dont la plaque est une pièce à part (après la
# correction : la plaque est posée sur la caisse corrigée)
node plaques_voitures.mjs "$@"
node vitres_voitures.mjs "$@"   # WFS-215 : vitres avant claires (RR Sport, A 45), verre de l'A 45 sombre, plaque arrière de la Model 3 en rectangle plat
# une plaque posée devant le bouclier allonge la voiture de quelques millimètres (longueur hors tout, plaques comprises) :
# 2e passage de la correction (sans effet si la voiture reste dans ses tolérances)
"$CORR" "$RACINE" "$@"
# WFS-192 (RG-06) : logos et noms de marque effacés ou retirés d'après tools/logos.json (rejouable ; arrêt sans rien écrire si un modèle a changé)
node effacer_logos.mjs "$@"
# WFS-193 : retouches de forme (calandres de marque remplacées par une grille générique, …) d'après tools/formes.json
# (rejouable ; arrêt sans rien écrire si un modèle a changé ; .lod refait ici aussi, puis par la boucle ci-dessous)
node retoucher_formes.mjs "$@"
# WFS-479 (RG-01, RG-04) : roues braquées dans le modèle source remises droites autour de leur moyeu (sinon elles battent en
# tournant) : la M4 seulement (roues avant modélisées braquées de 20°) ; rejouable (une roue droite ne bouge plus)
RED="$RACINE/build/redresser_roues"
for id in m4; do
  if [ $# -gt 0 ] && [[ ! " $* " =~ " $id " ]]; then continue; fi
  if [ -x "$RED" ]; then "$RED" "$RACINE" "$id"
  else echo "⚠️  $RED absent : construire la cible redresser_roues (ninja -C build redresser_roues), puis la lancer" >&2; exit 1; fi
done
# WFS-134 (RG-11, RG-12), WFS-158 (RG-04) : versions allégées des 20 voitures (assets/cars/<id>.lod : indices seuls, niveaux
# de détail 1 à 3, de 30 % à 5 % des triangles environ) ; choisies par la taille de la voiture à l'écran, avec fondu
# (src/render/vehicule_lod.h) ; avec une liste d'id, seules les voitures de la liste sont allégées
for id in golf m4 p911 gallardo $(for v in "${VOITURES[@]}"; do echo "${v%%|*}"; done); do
  if [ $# -gt 0 ] && [[ ! " $* " =~ " $id " ]]; then continue; fi
  node alleger_voiture.mjs "$RACINE/assets/cars/$id.wfm" "$RACINE/assets/cars/$id.lod" 0.3; done
