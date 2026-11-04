#!/usr/bin/env python3
# WaForSpeed — icônes des stores (WFS-22 / WFS-25) dessinées depuis l'icône adaptative Android
# (android/app/src/main/res/drawable/ic_avant_plan.xml, fond #07070C) : même « W » orange incliné, mêmes lignes cyan.
#   python3 tools/qa/icones_store.py store/icones
# Produit icone-play-512.png (Google Play, 512 × 512, 32 bits) et icone-ios-1024.png (App Store, 1024 × 1024, opaque).
# Le cadrage reprend la zone sûre de l'icône adaptative (72 dp centraux sur 108) ; suréchantillonnage × 4 pour l'anticrénelage.
import sys
from PIL import Image, ImageDraw

FOND, ORANGE, CYAN = (0x07, 0x07, 0x0C), (0xFF, 0x6A, 0x00), (0x00, 0xE5, 0xFF)
W_PATH = [(40, 34), (48, 34), (49.5, 54), (57, 36), (63, 36), (64.5, 54), (72, 34), (80, 34), (67, 66), (60, 66), (58.5, 49), (51, 66), (44, 66)]
TRAIT = [(41, 71), (73, 71), (72, 74), (40, 74)]
LIGNES = [((25, 54), (39, 54), 1.0), ((29, 60), (43, 60), 0.65), ((23, 48), (35, 48), 0.4)]

def icone(taille, sortie):
    ss = 4
    t = taille * ss
    x0, cote = 18.0, 72.0   # zone sûre : 18..90 dp
    f = lambda p: ((p[0] - x0) / cote * t, (p[1] - x0) / cote * t)
    im = Image.new("RGB", (t, t), FOND)
    d = ImageDraw.Draw(im)
    for a, b, alpha in LIGNES:   # lignes de vitesse cyan (opacité mélangée au fond)
        c = tuple(round(FOND[i] + (CYAN[i] - FOND[i]) * alpha) for i in range(3))
        e = 2.6 / cote * t
        d.line([f(a), f(b)], fill=c, width=round(e))
        for p in (a, b):
            q = f(p); d.ellipse([q[0] - e / 2, q[1] - e / 2, q[0] + e / 2, q[1] + e / 2], fill=c)
    d.polygon([f(p) for p in W_PATH], fill=ORANGE)
    c = tuple(round(FOND[i] + (ORANGE[i] - FOND[i]) * 0.85) for i in range(3))
    d.polygon([f(p) for p in TRAIT], fill=c)
    im.resize((taille, taille), Image.LANCZOS).save(sortie, optimize=True)
    print(f"{sortie} : {taille} × {taille}")

if __name__ == "__main__":
    dossier = sys.argv[1] if len(sys.argv) > 1 else "store/icones"
    icone(512, f"{dossier}/icone-play-512.png")
    icone(1024, f"{dossier}/icone-ios-1024.png")
