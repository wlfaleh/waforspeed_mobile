#!/usr/bin/env python3
# WaForSpeed — WFS-142 (RG-08) : part du reflet dans la luminance de la chaussée.
# Pour chaque capture <nom>.png du banc « route » (WFS_BANC=route ou nuit), compare avec <nom>-sansreflet.png (même image,
# reflet de la chaussée coupé par WFS_REFLETS=0) : rapport = luminance moyenne avec reflet / sans reflet, sur la chaussée
# seulement (trois rectangles entre les trottoirs, hors voiture et hors HUD, image 1280 x 720 mise à l'échelle).
# Critères de RG-08 : route sèche ≤ 1,10 ; route mouillée entre 1,15 et 1,8.
# Netteté du reflet (complément, constat du 2026-11-05 : sur main le rapport n'est que de 1,3 à 1,7, le défaut est surtout
# un reflet NET, l'image de la rue recopiée) : écart moyen entre pixels voisins (horizontal et vertical) de l'image
# « reflet seul » (avec − sans), en luminance × 1000. Un miroir (contours d'immeubles, d'arbres) donne une valeur élevée,
# des traînées floues une valeur basse.
# Usage : python3 -I tools/qa/luminance_route.py <dossier des captures>
import sys, os
from PIL import Image

# rectangles (x0, y0, x1, y1) en pixels d'une image 1280 x 720, vue arrière : loin (au-dessus du toit de la voiture),
# à gauche et à droite de la voiture (devant le volant et le bouton nitro)
ZONES = [(470, 200, 810, 330), (300, 380, 560, 480), (720, 380, 940, 480)]


def lineaire(c):
    return ((c / 255.0 + 0.055) / 1.055) ** 2.4 if c > 10 else c / 255.0 / 12.92


def plan(chemin):
    """luminance linéaire de chaque pixel de chaque zone : liste de zones, chacune liste de lignes"""
    im = Image.open(chemin).convert("RGB")
    sx, sy = im.width / 1280.0, im.height / 720.0
    zones = []
    for x0, y0, x1, y1 in ZONES:
        z = im.crop((int(x0 * sx), int(y0 * sy), int(x1 * sx), int(y1 * sy)))
        px = list(z.getdata()) if not hasattr(z, "get_flattened_data") else list(z.get_flattened_data())
        lum = [0.2126 * lineaire(r) + 0.7152 * lineaire(g) + 0.0722 * lineaire(b) for r, g, b in px]
        zones.append([lum[y * z.width:(y + 1) * z.width] for y in range(z.height)])
    return zones


def nettete(avec, sans):
    total, n = 0.0, 0
    for za, zs in zip(avec, sans):
        d = [[a - b for a, b in zip(la, ls)] for la, ls in zip(za, zs)]
        for y in range(len(d) - 1):
            for x in range(len(d[y]) - 1):
                total += abs(d[y][x + 1] - d[y][x]) + abs(d[y + 1][x] - d[y][x])
                n += 2
    return total / max(n, 1) * 1000


def luminance(chemin):
    im = Image.open(chemin).convert("RGB")
    sx, sy = im.width / 1280.0, im.height / 720.0
    total, n = 0.0, 0
    for x0, y0, x1, y1 in ZONES:
        zone = im.crop((int(x0 * sx), int(y0 * sy), int(x1 * sx), int(y1 * sy)))
        px = zone.get_flattened_data() if hasattr(zone, "get_flattened_data") else zone.getdata()
        for r, g, b in px:
            # sRGB → linéaire, puis luminance Rec. 709 (la lumière réellement renvoyée, pas la valeur affichée)
            lin = [((c / 255.0 + 0.055) / 1.055) ** 2.4 if c > 10 else c / 255.0 / 12.92 for c in (r, g, b)]
            total += 0.2126 * lin[0] + 0.7152 * lin[1] + 0.0722 * lin[2]
            n += 1
    return total / max(n, 1)


def main():
    if len(sys.argv) != 2:
        print("usage : luminance_route.py <dossier des captures>")
        return 2
    d = sys.argv[1]
    noms = sorted(f[:-4] for f in os.listdir(d) if f.startswith("route-") and f.endswith(".png") and "-sansreflet" not in f)
    for nom in noms:
        sans = os.path.join(d, nom + "-sansreflet.png")
        if not os.path.exists(sans):
            continue
        a, b = luminance(os.path.join(d, nom + ".png")), luminance(sans)
        k = nettete(plan(os.path.join(d, nom + ".png")), plan(sans))
        print(f"{nom} : luminance {a:.4f} avec reflet, {b:.4f} sans, rapport {a / max(b, 1e-6):.2f}, netteté du reflet {k:.2f}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
