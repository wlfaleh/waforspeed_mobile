#!/bin/bash
# WaForSpeed — WFS-149 (RG-02, RG-03) : textures d'essai KTX2 du banc du réalisme et rapport de compression (CI, groupe
# realisme). usage : bash tools/textures/ci_textures.sh <dossier des captures>
# → assets/textures/essai/*.ktx2 (lus par le banc), <captures>/textures-psnr.txt (publié), <captures>/textures-ktx2/ (artefact).
# Code de sortie 1 si une texture est refusée (PSNR sous le seuil) : la CI devient rouge.
set -uo pipefail
ICI=$(cd "$(dirname "$0")" && pwd); RACINE=$(cd "$ICI/../.." && pwd); CAP=${1:-captures}
mkdir -p "$CAP" "$RACINE/assets/textures/essai"
python3 -c "import numpy, PIL" 2>/dev/null || sudo apt-get install -y -qq python3-numpy python3-pil > /dev/null || python3 -m pip install -q --break-system-packages numpy pillow
bash "$ICI/encodeurs.sh" || exit 1
RAP="$CAP/textures-psnr.txt"
{ echo "WaForSpeed — rapport de compression des textures (WFS-149, RG-03) : PSNR des niveaux décompressés contre l'original"
  echo "Commit ${GITHUB_SHA:-local} ; seuils : couleur et ORM 38 dB, normales 35 dB ; « chaîne » = tous les niveaux (pondérés par les pixels)"
  echo "texture | type | variante | taille | octets | PSNR niveau 0 | PSNR chaîne | seuil | verdict"; } > "$RAP"
python3 "$ICI/essai/generer.py" > /dev/null
code=0
for t in couleur normale orm; do
  python3 "$ICI/convertir.py" "$t" "$RACINE/assets/textures/essai/essai-$t" "$ICI/essai/essai-$t.png" --rapport "$RAP" || code=1
done
mkdir -p "$CAP/textures-ktx2" && cp "$RACINE"/assets/textures/essai/*.ktx2 "$CAP/textures-ktx2/" 2>/dev/null
cat "$RAP"
exit $code
