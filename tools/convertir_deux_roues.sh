#!/bin/bash
# WaForSpeed — WFS-232 : conversion des 3 deux-roues du trafic (assets/cars/deux_roues/) : pour chacun, le corps sans ses roues,
# la roue avant et la roue arrière (tools/convert_car.mjs, WFS_ROUES_2R / WFS_PIECE), puis leurs niveaux de détail (alleger_voiture.mjs).
# Sources (clones partiels des dépôts GitHub publics, hors du dépôt ; licences et SHA-256 : assets/licences/deux-roues-*.txt et journal) :
#   SCOOTER=<Abhay-s-8/Portfolio>/models/ola_electric_scooter_red_color/scene.gltf          (OLA Electric Scooter, neshallads, CC BY 4.0)
#   ROUTIERE=<lxorb/mechanica>/web/store/models/generic/adventure/model.glb (Draco)        (Honda CB500X, DevanirGrau, CC BY 4.0)
#   SPORTIVE=<siddharth-vss/CAR-Camera-test>/static/kawasaki_ninja_zx-6r/scene.gltf       (Kawasaki ninja ZX-6R, valvetin, CC BY 4.0)
#   DRACO=<dossier où « npm install draco3dgltf » a été fait> (pour la routière seulement)
# usage : SCOOTER=… ROUTIERE=… SPORTIVE=… DRACO=… tools/convertir_deux_roues.sh [scooter|routiere|sportive …]   (sans nom : les 3)
set -e
ICI="$(cd "$(dirname "$0")" && pwd)"; SORTIE="$ICI/../assets/cars/deux_roues"; TMP="$(mktemp -d)"
cd "$ICI"
veut() { [ -z "$LISTE" ] || [[ " $LISTE " =~ " $1 " ]]; }
LISTE="$*"
# $1 id, $2 entrée, $3 longueur (m), $4 rotation (°, ou auto), $5 motif des feux arrière (vide = défaut) ; options dans l'environnement
conv() {
  local id=$1 src=$2 L=$3 rot=$4 tail=$5
  for piece in "" avant arriere; do
    local tmax=1024; [ -n "$piece" ] && tmax=512   # roues : textures de 512 px (petites à l'écran)
    local f="$SORTIE/$id${piece:+-roue-$piece}"
    if [ -n "$tail" ]; then WFS_TEXTURE_MAX=$tmax WFS_PIECE=$piece node convert_car.mjs "$src" "$f.wfm" "$L" "$rot" AUCUNE "$tail"
    else WFS_TEXTURE_MAX=$tmax WFS_PIECE=$piece node convert_car.mjs "$src" "$f.wfm" "$L" "$rot" AUCUNE; fi
    node alleger_voiture.mjs "$f.wfm" "$f.lod"
  done
}
if veut scooter; then   # roues dans un seul nœud « wheels_Wheel » ; logo « ola » repeint (couleur et métal / rugosité de la carrosserie)
  WFS_ROUES_2R='wheels_Wheel' WFS_OPAQUE='Second|Base' WFS_TEXTURES='Base|couleur=deux_roues/scooter-carrosserie-sans-logo.png|mr=deux_roues/scooter-carrosserie-metal-rugosite-sans-logo.png' \
    conv scooter "${SCOOTER:?}" 1.859 180 ''
fi
if veut routiere; then  # Draco décompressé, allégé (erreur 0,006), bagage / plaque / logos retirés, pièces sombres, pneus et chaîne allégés
  DRACO="${DRACO:?}" node deux_roues/dedraco.mjs "${ROUTIERE:?}" "$TMP/cb500x.glb"
  node simplifier_voiture.mjs "$TMP/cb500x.glb" "$TMP/cb500x_s.glb" AUCUNE 0.006
  WFS_EXCLURE='NylonBag|FabricBag|NylonStrap|HondaDecal|^logo$|Placa' WFS_EXCLURE_NOEUD='^Object_(143|145|147|149|151) ' WFS_PHARE='^Object_(175|259) ' \
  WFS_ROUES_2R='^Object_(20|22|24|26|28|30|265|267|269|271|273|275) ' \
  WFS_ALLEGER='HondaTire4_M=0.12!;Chain_M=0.15!;preto_fosco_l=0.45!;preto_semibrilho=0.45!;preto_fosco=0.5!;borracha=0.5!;disco_freio=0.5!;aco_parafuso=0.5!' \
    conv routiere "$TMP/cb500x_s.glb" 2.155 auto 'Lanterna|Stoplight'
fi
if veut sportive; then  # allégé (erreur 0,002) ; feu arrière et phare sortis du matériau partagé ; autocollants de marque retirés
  node simplifier_voiture.mjs "${SPORTIVE:?}" "$TMP/zx6r.glb" Panels 0.002   # carénages (Panels) 4 fois plus fins
  WFS_ALLEGER='Panels=0.55;Engine=0.5;Mesh=0.6' WFS_OPAQUE='Panels|Engine|Mesh|Decal' WFS_FEU='^Taillight' WFS_PHARE='^Headlight' \
  WFS_TEXTURES='Decal|couleur=deux_roues/sportive-autocollants-sans-marque.png' \
  WFS_ROUES_2R='^(Front Wheel|Front Rim|Brakes|Brakes Too|Rear Wheel|Rear Rim|Rear Brakes|Rear Brakes Too\.|Rear Main Brakes)_Mesh_0 ' \
    conv sportive "$TMP/zx6r.glb" 2.007 180 ''
fi
