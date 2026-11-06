#!/usr/bin/env python3
# WaForSpeed — WFS-149 : sources de la texture d'essai du banc (realisme-texture-essai), générées par ce script
# (œuvre du projet, CC0 1.0, assets/licences/textures-essai.txt). Ce n'est PAS une texture du jeu : elle sert à vérifier
# la chaîne KTX2 (couleur sRGB, normales et ORM linéaires, mipmaps, filtrage anisotrope) sur un panneau du banc.
#   python3 tools/textures/essai/generer.py   → essai-couleur.png, essai-normale.png, essai-orm.png (256 × 256)
# Motif : 4 × 4 carreaux de 64 px séparés par des joints de 4 px, un rivet hémisphérique au centre de chaque carreau,
# rayures fines (période 2 et 4 px) dans 4 carreaux : sans mipmaps, elles scintillent au loin (moiré).
import pathlib
import numpy as np
from PIL import Image

N, C, J = 256, 64, 4   # taille, carreau, joint (px)
ICI = pathlib.Path(__file__).resolve().parent
# teintes des carreaux (codes sRGB) : les 16 premières cases de la charte ColorChecker, valeurs approchées (œil seulement)
TEINTES = [(115, 82, 68), (194, 150, 130), (98, 122, 157), (87, 108, 67), (133, 128, 177), (103, 189, 170), (214, 126, 44), (80, 91, 166),
           (193, 90, 99), (94, 60, 108), (157, 188, 64), (224, 163, 46), (56, 61, 150), (70, 148, 73), (175, 54, 60), (231, 199, 31)]

y, x = np.mgrid[0:N, 0:N].astype(np.float64) + 0.5
tx, ty = (x // C).astype(int), (y // C).astype(int)
lx, ly = x - tx * C, y - ty * C                         # position dans le carreau (px)
joint = (lx < J) | (ly < J)
k = ty * 4 + tx
r = np.hypot(lx - (C + J) / 2, ly - (C + J) / 2)
R = 14.0
rivet = (r < R) & ~joint
hauteur = np.where(joint, -1.0, 0.0) + np.where(rivet, np.sqrt(np.maximum(R * R - r * r, 0)) / R, 0.0)   # en unités de R

# couleur
col = np.array(TEINTES, np.float64)[k]
for t, per in ((5, 2), (6, 4), (9, 2), (10, 4)):         # rayures fines claires / sombres
    sel = (k == t) & ~joint & ~rivet
    rayure = ((x // per) % 2 == 0)[..., None]
    col = np.where(sel[..., None], np.where(rayure, col * 1.25, col * 0.75), col)
col = np.where(joint[..., None], np.array([60, 60, 62.0]), col)
col = np.where(rivet[..., None], np.array([170, 170, 175.0]), col)
Image.fromarray(np.clip(np.rint(col), 0, 255).astype(np.uint8), 'RGB').save(ICI / 'essai-couleur.png')

# normales, convention « OpenGL » des sources libres (vert vers le haut de l'image) : convertir.py la passe au moteur
gx = (np.roll(hauteur, -1, 1) - np.roll(hauteur, 1, 1)) / 2
gy = (np.roll(hauteur, -1, 0) - np.roll(hauteur, 1, 0)) / 2
force = 3.0                                             # pente en px de hauteur par px (relief marqué, visible à 4 m)
n = np.stack([-gx * force, gy * force, np.ones_like(gx)], -1)   # vert = −∂h/∂(haut) = +∂h/∂(ligne)
n /= np.linalg.norm(n, axis=-1, keepdims=True)
Image.fromarray(np.clip(np.rint((n * 0.5 + 0.5) * 255), 0, 255).astype(np.uint8), 'RGB').save(ICI / 'essai-normale.png')

# ORM : occlusion (joints 0,55, pied du rivet 0,8), rugosité (carreaux 0,8 ; rivet 0,3), métal (rivet 1, reste 0)
ao = np.where(joint, 0.55, 1.0) * np.where(rivet & (r > R * 0.8), 0.8, 1.0)
rough = np.where(rivet, 0.3, np.where(joint, 0.9, 0.8))
metal = np.where(rivet, 1.0, 0.0)
orm = np.stack([ao, rough, metal], -1)
Image.fromarray(np.clip(np.rint(orm * 255), 0, 255).astype(np.uint8), 'RGB').save(ICI / 'essai-orm.png')
print('sources écrites :', ', '.join(f.name for f in sorted(ICI.glob('essai-*.png'))))
