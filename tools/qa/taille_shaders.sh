#!/usr/bin/env bash
# WaForSpeed — WFS-611 : taille du shader de scène (scene.frag) pour chaque famille de pipelines (src/render/familles_pipeline.h).
# Le pilote du téléphone retire au chargement le code que la constante MASQUE_FAMILLE rend inutile ; ici on fait la même chose avec
# spirv-opt (constantes figées puis optimisation) et on compte les instructions SPIR-V restantes : moins d'instructions = moins de
# registres réservés par pixel sur le GPU mobile, donc plus de pixels calculés en même temps.
# Usage : bash tools/qa/taille_shaders.sh   (paquets glslc et spirv-tools)
set -euo pipefail
RACINE=$(cd "$(dirname "$0")/../.." && pwd)
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
glslc -O "$RACINE/shaders/scene.frag" -o "$T/scene.spv"
# masques recopiés de familles_pipeline.h (le test test_familles_pipeline vérifie les valeurs des drapeaux)
C=$((2 + 16 + 32 + 4096 + 8192 + 524288 + 4194304))
printf '%-10s %-8s %s\n' famille masque instructions
for f in "complet 4294967295" "route $((C + 256))" "trottoir $((C + 512))" "sol $((C + 1024))" \
         "decor $((C + 16384 + 262144 + 1048576 + 2097152))" "voiture $((C + 1 + 4 + 8 + 128 + 262144 + 131072))" \
         "vitre $((C + 64 + 1 + 262144))"; do
  set -- $f
  spirv-opt --set-spec-const-default-value "0:true 2:$2" --freeze-spec-const -O "$T/scene.spv" -o "$T/o.spv"
  printf '%-10s %-8s %s\n' "$1" "$2" "$(spirv-dis "$T/o.spv" | grep -c ' Op')"
done
