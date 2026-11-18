#!/usr/bin/env bash
# WaForSpeed — WFS-562 (RG-03) : refait les 3 photos de l'écran « Choisis ton mode » (assets/ui/modes/*.jpg) avec le banc d'essai
# PC (banc « modes-photos » : vrai rendu du moteur, carte Ville, scènes posées, sans interface), puis les recadre et les réduit à la
# taille d'une carte en 2400 × 1080 (720 × 546). Rendu en 1440 × 1092 puis réduction ×2 (lissage des bords) ; le banc vise sous
# les sujets : voitures, portique et police dans la moitié haute de la photo, au-dessus du voile sombre du nom du mode (RG-04).
# Usage, depuis la racine du dépôt, après la compilation (build/wfs_bench) : bash tools/ui/photos_modes.sh
# Besoin : ImageMagick (convert), lavapipe (même rendu que la CI). Aucune image venue d'ailleurs, aucune retouche à la main.
set -euo pipefail
RACINE=$(git rev-parse --show-toplevel)
cd "$RACINE"
TRAVAIL=$(mktemp -d "${TMPDIR:-/tmp}/photos_modes.XXXXXX")
trap 'rm -rf "$TRAVAIL"' EXIT
(cd build && WFS_BANC=modes-photos WFS_TAILLE=1440x1092 ./wfs_bench "$TRAVAIL" ../assets/ > "$TRAVAIL/journal.txt" 2>&1) || { tail -20 "$TRAVAIL/journal.txt"; echo "erreur : le banc modes-photos a échoué"; exit 1; }
mkdir -p assets/ui/modes
for m in checkpoint course5 poursuite; do
  src="$TRAVAIL/modes-photo-$m.png"
  [ -f "$src" ] || { echo "erreur : $src absente"; exit 1; }
  convert "$src" -resize 720x546! -strip -sampling-factor 4:2:0 -quality 86 "assets/ui/modes/$m.jpg"
  echo "assets/ui/modes/$m.jpg : $(stat -c %s "assets/ui/modes/$m.jpg") octets, SHA-256 $(sha256sum "assets/ui/modes/$m.jpg" | cut -c1-16)…"
done
