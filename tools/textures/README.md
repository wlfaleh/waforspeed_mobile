# Textures du moteur : PNG / JPG / EXR → KTX2 (WFS-149)

## Commande unique (5 lignes)
```
bash tools/textures/encodeurs.sh                       # une fois : astcenc 5.2.0 et etc2comp (Apache-2.0), versions épinglées
python3 tools/textures/convertir.py couleur assets/textures/<dossier>/<nom> source.png      # couleur : sRGB
python3 tools/textures/convertir.py normale assets/textures/<dossier>/<nom>-n source.png    # normales (vert vers le haut, sinon --vert-bas)
python3 tools/textures/convertir.py orm assets/textures/<dossier>/<nom>-orm ao.png rugosite.png metal.png   # ou une image ORM
# ajouter --rapport rapport.txt pour garder les PSNR ; code de sortie 1 = texture refusée (précision insuffisante)
```

## Ce que fait l'outil
- **Mipmaps** : chaîne complète jusqu'à 1×1, moyenne 2×2 **en linéaire** (la couleur sRGB est décodée, moyennée, recodée) ;
  normales renormalisées à chaque niveau ; ORM = occlusion, rugosité, métal dans R, G, B (linéaire).
- **Trois fichiers** par texture, lus par `src/render/ktx2.h` (le premier que le GPU sait lire) :
  `<nom>.astc.ktx2` (ASTC 6x6, ou 4x4 si 6x6 est trop imprécis), `<nom>.etc2.ktx2` (ETC2 RGBA, repli),
  `<nom>.rgba8.ktx2` (l'ASTC décompressé : repli du PC, même image que le téléphone).
- **Précision (RG-03)** : PSNR de chaque niveau décompressé contre l'original, niveau 0 et chaîne entière ;
  refus sous **38 dB** (couleur, ORM) ou **35 dB** (normales). Les décodeurs : astcenc (ASTC), `etc2.py` (ETC2, d'après la
  Khronos Data Format Specification 1.3).
- **Normales** : convention du moteur « vert = +v » (bitangente de `src/render/mesh.h`) ; une source « OpenGL » (vert vers
  le haut : ambientCG `NormalGL`, Poly Haven `nor_gl`) est inversée par défaut, une source « DirectX » prend `--vert-bas`.
- EXR : module Python `OpenEXR` (BSD-3) à installer à part.

## Licences des encodeurs (outils de conversion, jamais livrés dans l'APK)
- astcenc 5.2.0, Arm Limited, Apache License 2.0 : https://github.com/ARM-software/astc-encoder
- etc2comp (révision 39422c1), Google, Apache License 2.0 : https://github.com/google/etc2comp

## Texture d'essai du banc
`essai/generer.py` crée les sources (CC0, `assets/licences/textures-essai.txt`) ; `ci_textures.sh` (CI, groupe `realisme`)
les convertit dans `assets/textures/essai/` (non versionné), écrit `textures-psnr.txt` (publié dans la Release d'aperçu)
et le banc les montre sur un panneau (`realisme-texture-essai`, `realisme-texture-essai-basse`).
