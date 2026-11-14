#!/usr/bin/env python3
# WaForSpeed — WFS-492 (épic E18) : textures d'habillage des skins (src/render/vehicule_skin.h), une par couple voiture × skin.
#
#   python3 tools/skins/generer.py [--skin rallye] [--apercu <dossier>] [<voiture> ...]
#     → assets/cars/skins/<voiture>-<skin>.etc2.ktx2 (EAC RG11, 1024 × 1024, chaîne complète de mipmaps)
#     --apercu : écrit aussi <dossier>/<voiture>-<skin>.png (masques en couleurs, pour relire le dessin ; jamais dans assets/)
#
# Dépliage par projection (même cadre que vehicule_skin::atlas et shaders/vehicule.glsl, SKIN_*) : en mètres du repère de la
# voiture, flanc gauche, flanc droit, dessus, avant et arrière ; chaque motif est dessiné à sa taille réelle, d'après les mesures
# de tools/skins/voitures.json (roues, bas de caisse, ligne de ceinture). Deux masques de couverture : R = couleur A, G = couleur B
# (par-dessus A) ; ailleurs, la peinture choisie par le joueur reste visible. Couleurs dans le shader (SKIN_COULEURS).
#
# Étapes : dessin sur-échantillonné × 4 (bords nets sans crénelage) → réduction en moyenne 4 × 4 → marges entre zones remplies
# par répétition du bord (aucune fuite de couleur d'une zone à l'autre dans les petits niveaux) → mipmaps (moyenne 2 × 2 : des
# couvertures sont des grandeurs linéaires) → encodage EAC R11 de chaque canal (recherche exhaustive des 16 tables × 15
# multiplicateurs, Khronos Data Format Specification 1.3, § 21.3) → contrôle : PSNR du décodé contre l'original (≥ 38 dB, comme
# tools/textures/convertir.py ; sinon refus, code de sortie 1) → KTX2 (tools/textures/ktx2.py).
#
# Rallye (RG-05, dessin original, aucun logo, aucun sponsor, aucune livrée d'écurie connue) : deux larges bandes parallèles du
# capot au hayon (couleur A) bordées d'un filet (couleur B), qui descendent sur les boucliers avant et arrière ; sur chaque
# portière avant, une plaque de course (A, filet B) portant le numéro « 07 » (B, police Rajdhani Bold, SIL OFL 1.1) ; le long du
# bas de caisse, une bande (A) et un filet (B).
import argparse, json, math, pathlib, sys
import numpy as np
from PIL import Image, ImageDraw, ImageFont

ICI = pathlib.Path(__file__).resolve().parent
RACINE = ICI.parent.parent
sys.path.insert(0, str(RACINE / 'tools' / 'textures'))
import ktx2   # noqa: E402

# ---- cadre (vehicule_skin.h, namespace atlas)
DX, DZ, H = 1.10, 2.50, 1.60
T = 1024
ZONES = {'gauche': (0, 272), 'droite': (280, 552), 'dessus': (560, 864), 'faces': (872, 1024)}
S = 4                          # sur-échantillonnage du dessin
EAC_RG11 = 155                 # VK_FORMAT_EAC_R11G11_UNORM_BLOCK
SEUIL_DB = 38.0
POLICE = RACINE / 'assets' / 'fonts' / 'Rajdhani-Bold.ttf'

TABLE = np.array([
    (-3, -6, -9, -15, 2, 5, 8, 14), (-3, -7, -10, -13, 2, 6, 9, 12), (-2, -5, -8, -13, 1, 4, 7, 12), (-2, -4, -6, -13, 1, 3, 5, 12),
    (-3, -6, -8, -12, 2, 5, 7, 11), (-3, -7, -9, -11, 2, 6, 8, 10), (-4, -7, -8, -11, 3, 6, 7, 10), (-3, -5, -8, -11, 2, 4, 7, 10),
    (-2, -6, -8, -10, 1, 5, 7, 9), (-2, -5, -8, -10, 1, 4, 7, 9), (-2, -4, -8, -10, 1, 3, 7, 9), (-2, -5, -7, -10, 1, 4, 6, 9),
    (-3, -4, -7, -10, 2, 3, 6, 9), (-1, -2, -3, -10, 0, 1, 2, 9), (-4, -6, -8, -9, 3, 5, 7, 8), (-3, -5, -7, -9, 2, 4, 6, 8)], np.int64)

# ---------------------------------------------------------------- passage mètres → pixels du dessin (× S)
def px_flanc(zone, z, y):
    """flanc vu de dehors : gauche, u de l'avant (z = −DZ) à l'arrière ; droite, u de l'arrière à l'avant ; haut de zone = y = H"""
    v0, v1 = ZONES[zone]
    u = (z + DZ) / (2 * DZ) if zone == 'gauche' else (DZ - z) / (2 * DZ)
    return u * T * S, (v0 + (H - y) / H * (v1 - v0)) * S

def px_dessus(x, z):
    v0, v1 = ZONES['dessus']
    return (z + DZ) / (2 * DZ) * T * S, (v0 + (x + DX) / (2 * DX) * (v1 - v0)) * S

def px_face(arriere, x, y):
    """avant : u 0..0,5, vu de face (droite de la voiture à gauche) ; arrière : u 0,5..1, vu de dos"""
    v0, v1 = ZONES['faces']
    u = 0.5 + (x + DX) / (2 * DX) * 0.5 if arriere else (DX - x) / (2 * DX) * 0.5
    return u * T * S, (v0 + (H - y) / H * (v1 - v0)) * S

def rect(draw, p0, p1, valeur=255):
    (x0, y0), (x1, y1) = p0, p1
    draw.rectangle([min(x0, x1), min(y0, y1), max(x0, x1) - 1, max(y0, y1) - 1], fill=valeur)

# ---------------------------------------------------------------- dessin du Rallye
BANDE_X = (0.055, 0.255)       # |x| des deux larges bandes (m) : 20 cm chacune, 11 cm de peinture entre elles
FILET_X = (0.272, 0.287)       # |x| du filet extérieur (15 mm, à 17 mm de la bande)
PLAQUE_L, PLAQUE_H, PLAQUE_R = 0.50, 0.36, 0.05   # plaque de course (m) : largeur, hauteur au plus, rayon des coins
NUMERO = '07'                  # numéro de course (chiffres de l'utilisateur plus tard, RG-05)

def rallye(v):
    """v = mesures de la voiture (voitures.json) → deux images L (R = couleur A, G = couleur B), T·S × T·S"""
    A = Image.new('L', (T * S, T * S), 0); B = Image.new('L', (T * S, T * S), 0)
    da, db = ImageDraw.Draw(A), ImageDraw.Draw(B)
    # larges bandes et filets, de l'avant à l'arrière (dessus), puis sur les faces avant et arrière (mêmes x : continues)
    for signe in (-1, 1):
        for (x0, x1), d in ((BANDE_X, da), (FILET_X, db)):
            rect(d, px_dessus(signe * x0, -DZ), px_dessus(signe * x1, DZ))
            for arr in (False, True):
                rect(d, px_face(arr, signe * x0, H), px_face(arr, signe * x1, 0.0))
    # flancs : bande du bas de caisse (6 cm) et filet (15 mm) au-dessus, sur toute la longueur
    bas, ceinture = v['bas'], v['ceinture']
    for zone in ('gauche', 'droite'):
        rect(da, px_flanc(zone, -DZ, bas + 0.11), px_flanc(zone, DZ, bas + 0.05))
        rect(db, px_flanc(zone, -DZ, bas + 0.14), px_flanc(zone, DZ, bas + 0.125))
        # plaque de course sur la portière avant, entre la bande et la ligne de ceinture
        zc = v['roue_av'] + v['porte'] * (v['roue_ar'] - v['roue_av'])
        y0, y1 = bas + 0.17, ceinture - 0.06
        h = min(PLAQUE_H, y1 - y0); yc = (y0 + y1) / 2
        (ax, ay), (bx, by) = px_flanc(zone, zc - PLAQUE_L / 2, yc + h / 2), px_flanc(zone, zc + PLAQUE_L / 2, yc - h / 2)
        box = [min(ax, bx), ay, max(ax, bx), by]
        r = PLAQUE_R / H * (ZONES[zone][1] - ZONES[zone][0]) * S
        da.rounded_rectangle(box, radius=r, fill=255)
        # filet intérieur (12 mm) à 18 mm du bord
        e = 0.018 / H * (ZONES[zone][1] - ZONES[zone][0]) * S; l = 0.012 / H * (ZONES[zone][1] - ZONES[zone][0]) * S
        db.rounded_rectangle([box[0] + e, box[1] + e, box[2] - e, box[3] - e], radius=max(r - e, 1), outline=255, width=max(1, round(l)))
        # numéro : hauteur des chiffres 62 % de la plaque, centré (le cadre est moins large que haut en pixels : échelle propre)
        echY = (ZONES[zone][1] - ZONES[zone][0]) * S / H        # pixels par mètre, verticalement
        echX = T * S / (2 * DZ)                                  # pixels par mètre, horizontalement
        taille = 0.62 * h * echY
        police = ImageFont.truetype(str(POLICE), max(8, round(taille * 1.32)))   # corps ≈ 1,32 × hauteur des chiffres (Rajdhani)
        g = police.getbbox(NUMERO)
        txt = Image.new('L', (g[2] - g[0] + 8, g[3] - g[1] + 8), 0)
        ImageDraw.Draw(txt).text((4 - g[0], 4 - g[1]), NUMERO, fill=255, font=police)
        # la police est dessinée à l'échelle verticale ; largeur ramenée à l'échelle horizontale du cadre (pas d'étirement à l'écran)
        k = (taille / txt.height)
        txt = txt.resize((max(1, round(txt.width * k * echX / echY)), max(1, round(txt.height * k))), Image.LANCZOS)
        cx, cy = (box[0] + box[2]) / 2, (box[1] + box[3]) / 2
        B.paste(255, (round(cx - txt.width / 2), round(cy - txt.height / 2)), txt)
    return A, B

# ---------------------------------------------------------------- WFS-493 : outils communs aux skins dessinées en 3D
def points(v):
    """point 3D (m, repère de la voiture) de chaque pixel de la texture (T × T), par zone : flancs à |x| = largeur, dessus à la
    hauteur de la ceinture, faces avant / arrière aux z des boucliers (voitures.json) ; dedans = pixels d'une zone (pas les marges).
    Les projections se rejoignent : arête d'épaule (flanc à y = ceinture ↔ dessus à |x| = largeur) et coins (flanc à z du bouclier
    ↔ face à |x| = largeur) donnent le même point, d'où des motifs continus d'une zone à l'autre (RG-04 de WFS-492)."""
    X = np.zeros((T, T)); Y = np.zeros((T, T)); Z = np.zeros((T, T)); dedans = np.zeros((T, T), bool)
    u = (np.arange(T) + 0.5) / T
    for zone, (v0, v1) in ZONES.items():
        t = (np.arange(v0, v1) + 0.5 - v0) / (v1 - v0)
        uu, tt = np.meshgrid(u, t)
        sl = slice(v0, v1)
        if zone in ('gauche', 'droite'):
            Z[sl] = -DZ + uu * 2 * DZ if zone == 'gauche' else DZ - uu * 2 * DZ
            Y[sl] = H * (1 - tt); X[sl] = -v['largeur'] if zone == 'gauche' else v['largeur']
        elif zone == 'dessus':
            Z[sl] = -DZ + uu * 2 * DZ; X[sl] = -DX + tt * 2 * DX; Y[sl] = v['ceinture']
        else:
            arr = uu >= 0.5
            X[sl] = np.where(arr, (uu - 0.5) * 4 * DX - DX, DX - uu * 4 * DX)
            Y[sl] = H * (1 - tt); Z[sl] = np.where(arr, v['arriere'], v['avant'])
        dedans[sl] = True
    return X, Y, Z, dedans

def poly(draw, pts):
    draw.polygon([(round(a), round(b)) for a, b in pts], fill=255)

def flanc_poly(draw, zone, pts_zy):
    """polygone donné en (z, y) mètres sur un flanc"""
    poly(draw, [px_flanc(zone, z, y) for z, y in pts_zy])

def dessus_poly(draw, pts_xz):
    poly(draw, [px_dessus(x, z) for x, z in pts_xz])

def face_poly(draw, arriere, pts_xy):
    poly(draw, [px_face(arriere, x, y) for x, y in pts_xy])

def lisse(a, b, x):
    t = np.clip((x - a) / (b - a), 0.0, 1.0)
    return t * t * (3 - 2 * t)

# ---------------------------------------------------------------- Drift Tokyo (RG-02, dessin original : aucun texte réel, aucune marque)
# Couleur A (vermillon) en dégradé : pleine à l'arrière, fondue vers l'avant le long d'une ligne penchée vers l'avant (35°), sur
# 1,1 m ; couleur B (noir) : trois entailles penchées dans le dégradé, deux entailles sur le capot, et trois signes décoratifs
# inventés (pochoirs anguleux : ni lettre, ni idéogramme, ni chiffre) sur l'aile arrière, au-dessus de la roue.
DRIFT_PENTE = math.tan(math.radians(35))
DRIFT_FONDU = 1.10
# signes inventés, en unités d'une grille 5 × 6 (x vers l'arrière, y vers le haut) : chaque signe = polygones
SIGNES = [
    [[(0, 0), (1.2, 0), (2.6, 6), (1.4, 6)], [(2.2, 2.4), (5, 3.2), (5, 4.3), (2.5, 3.5)]],
    [[(0, 1.4), (2.5, 0), (5, 1.4), (5, 2.6), (2.5, 1.2), (0, 2.6)], [(0, 3.6), (2.5, 2.2), (5, 3.6), (5, 4.8), (2.5, 3.4), (0, 4.8)]],
    [[(3.8, 0), (5, 0), (5, 6), (0.4, 6), (0.4, 4.9), (3.8, 4.9)], [(0, 0), (2.6, 2.4), (2.6, 3.6), (0, 1.2)]],
]

def drift_couverture(v, Y, Z):
    zc = v['roue_av'] + 0.55 * (v['roue_ar'] - v['roue_av'])
    y = np.clip(Y, v['bas'], v['ceinture'])
    return lisse(0.0, 1.0, (Z - (zc - (y - v['bas']) * DRIFT_PENTE)) / DRIFT_FONDU)

def drift_tokyo(v):
    X, Y, Z, dedans = points(v)
    A = np.where(dedans, drift_couverture(v, Y, Z), 0.0)
    B = Image.new('L', (T * S, T * S), 0); db = ImageDraw.Draw(B)
    zc = v['roue_av'] + 0.55 * (v['roue_ar'] - v['roue_av'])
    y0, y1 = v['bas'] + 0.04, v['ceinture'] - 0.05
    for zone in ('gauche', 'droite'):
        # entailles : bas à zc + décalage, penchées comme le dégradé ; largeurs 14, 8 et 4 cm
        for dz, l in ((0.18, 0.14), (0.42, 0.08), (0.58, 0.04)):
            za = zc + dz
            flanc_poly(db, zone, [(za, y0), (za + l, y0), (za + l - (y1 - y0) * DRIFT_PENTE, y1), (za - (y1 - y0) * DRIFT_PENTE, y1)])
        # signes : hauteur 11 cm, entre le haut de l'arche arrière et la ceinture, derrière le centre de la roue arrière
        h = 0.11; e = h / 6
        ybas = max(2 * v['rayon'] + 0.06, v['ceinture'] - 0.05 - h - 0.03)
        for k, signe in enumerate(SIGNES):
            z0 = v['roue_ar'] - 0.22 + k * (5 * e + 0.035)
            for p in signe:
                flanc_poly(db, zone, [(z0 + a * e, ybas + b * e) for a, b in p])
    # capot : deux entailles en travers, côté gauche (dessin asymétrique), loin des arêtes d'épaule (|x| ≤ largeur − 0,18 m)
    xm = v['largeur'] - 0.18
    zh = v['avant'] + 0.40   # du milieu du capot vers le pare-brise (fin à ≈ avant + 1,2 m)
    for dz, l in ((0.0, 0.12), (0.20, 0.06)):
        dessus_poly(db, [(-xm, zh + dz), (-xm, zh + dz + l), (0.20, zh + dz + l + 0.55), (0.20, zh + dz + 0.55)])
    return A, B

# ---------------------------------------------------------------- Néon nuit (RG-03) : filets cyan (A) et magenta (B), émissifs la nuit
# (shaders/vehicule.glsl, SKIN_NEON) ; de jour, filets de couleur normaux. Flancs : filet magenta bas sur toute la longueur, filet
# cyan qui monte en biais (25°) au-dessus de la roue arrière ; mêmes hauteurs sur les faces (continus aux coins) ; dessus : deux
# paires de filets du capot au coffre, qui descendent sur les faces jusqu'au filet cyan.
NEON_CYAN_L, NEON_MAG_L = 0.035, 0.025

def neon_nuit(v):
    A = Image.new('L', (T * S, T * S), 0); B = Image.new('L', (T * S, T * S), 0)
    da, db = ImageDraw.Draw(A), ImageDraw.Draw(B)
    ym = v['bas'] + 0.10                     # filet magenta
    yc = v['bas'] + 0.17                     # filet cyan, avant
    y2 = v['ceinture'] - 0.06                # filet cyan, arrière (monté)
    zk = v['roue_ar'] - 0.90
    zk2 = zk + (y2 - yc) / math.tan(math.radians(25))
    c, m = NEON_CYAN_L / 2, NEON_MAG_L / 2
    for zone in ('gauche', 'droite'):
        flanc_poly(db, zone, [(-DZ, ym - m), (DZ, ym - m), (DZ, ym + m), (-DZ, ym + m)])
        flanc_poly(da, zone, [(-DZ, yc - c), (zk, yc - c), (zk2, y2 - c), (DZ, y2 - c), (DZ, y2 + c), (zk2, y2 + c), (zk, yc + c), (-DZ, yc + c)])
    for arr, ycy in ((False, yc), (True, y2)):
        face_poly(db, arr, [(-DX, ym - m), (DX, ym - m), (DX, ym + m), (-DX, ym + m)])
        face_poly(da, arr, [(-DX, ycy - c), (DX, ycy - c), (DX, ycy + c), (-DX, ycy + c)])
        for signe in (-1, 1):
            for (x0, x1), d in (((0.30, 0.335), da), ((0.36, 0.385), db)):
                face_poly(d, arr, [(signe * x0, H), (signe * x1, H), (signe * x1, ycy + c + 0.04), (signe * x0, ycy + c + 0.04)])
    for signe in (-1, 1):
        for (x0, x1), d in (((0.30, 0.335), da), ((0.36, 0.385), db)):
            dessus_poly(d, [(signe * x0, -DZ), (signe * x1, -DZ), (signe * x1, DZ), (signe * x0, DZ)])
    return A, B

# ---------------------------------------------------------------- Camouflage (RG-04) : urbain gris, noir et blanc, à taches de 20 à 60 cm
# Habillage complet : A (blanc) couvre toute la carrosserie ; B (noir) posé à 55 % donne le gris, à 100 % le noir. Taches : formes
# organiques allongées (rayon qui ondule avec la direction, 4 ondes au hasard ; rapport 1,4 à 2,2) de 20 à 60 cm de diamètre, posées en 3D sur l'enveloppe de la
# voiture (flancs, dessus, faces) par tirage avec distance minimale (graine fixe) ; chaque tache est grise ou noire. Évaluées au point
# 3D de chaque pixel (points()) : continues d'une zone à l'autre. Taille contrôlée à chaque génération (taches_cm).
CAMO_GRIS, CAMO_NOIR = 0.55, 1.0
CAMO_D = (0.20, 0.60)          # diamètre des taches (m)
CAMO_GRAINE = 4931
CAMO_PIXEL = 0.0055            # taille d'un pixel sur la carrosserie (m) : bord adouci sur un pixel

def taches(v):
    """liste (centre, rayon, ondes, tonalité, axe d'allongement, rapport) ; centres sur l'enveloppe (|x| = largeur, y = ceinture, z = boucliers)"""
    rng = np.random.default_rng(CAMO_GRAINE)
    l, av, ar = v['largeur'], v['avant'], v['arriere']
    faces = [  # (aire, tirage d'un point)
        (H * (ar - av), lambda: (-l, rng.uniform(0, H), rng.uniform(av, ar))),
        (H * (ar - av), lambda: (l, rng.uniform(0, H), rng.uniform(av, ar))),
        (2 * l * (ar - av), lambda: (rng.uniform(-l, l), v['ceinture'], rng.uniform(av, ar))),
        (2 * l * H, lambda: (rng.uniform(-l, l), rng.uniform(0, H), av)),
        (2 * l * H, lambda: (rng.uniform(-l, l), rng.uniform(0, H), ar))]
    aires = np.array([f[0] for f in faces]); aires /= aires.sum()
    liste = []
    for _ in range(20000):
        if len(liste) >= 600: break
        c = np.array(faces[rng.choice(len(faces), p=aires)][1]())
        r = rng.uniform(*CAMO_D) / 2
        ton = CAMO_NOIR if rng.random() < 0.4 else CAMO_GRIS
        # écart minimal : deux taches de même ton se touchent à peine (pas de grande nappe de plus de 60 cm) ; une noire mord
        # largement sur une grise (taches imbriquées, comme un vrai camouflage urbain, pas des pois sur fond blanc)
        if any(np.linalg.norm(c - t[0]) < (0.95 if t[3] == ton else 0.45) * (r + t[1]) for t in liste): continue
        u = rng.normal(size=(4, 3)); u /= np.linalg.norm(u, axis=1)[:, None]
        ondes = (u, rng.uniform(0.8, 1.8, 4), rng.uniform(0, 2 * math.pi, 4), rng.uniform(0.07, 0.13, 4))
        # tache allongée (rapport 1,4 à 2,2) dans une direction au hasard, même aire : formes de camouflage, pas des ronds
        a = rng.normal(size=3); a /= np.linalg.norm(a)
        liste.append((c, r, ondes, ton, a, rng.uniform(1.4, 2.2)))
    return liste

def couverture_taches(X, Y, Z, liste, ton):
    P = np.stack([X, Y, Z], -1)
    cov = np.zeros(X.shape)
    for c, r, (u, k, ph, amp), t, a, e in liste:
        if t != ton: continue
        m = 1.6 * r * math.sqrt(e)   # demi-côté de la boîte englobante (tache allongée, ondulée)
        boite = (np.abs(X - c[0]) < m) & (np.abs(Y - c[1]) < m) & (np.abs(Z - c[2]) < m)
        if not boite.any(): continue
        d = P[boite] - c
        pa = d @ a                                                     # allongement : × √e le long de a, ÷ √e en travers
        d = (d - pa[:, None] * a[None, :]) * math.sqrt(e) + pa[:, None] * a[None, :] / math.sqrt(e)
        n = np.maximum(np.linalg.norm(d, axis=1), 1e-9)
        rd = r * (1 + (amp[None, :] * np.sin(k[None, :] * ((d / n[:, None]) @ u.T) * math.pi + ph[None, :])).sum(1))
        cov[boite] = np.maximum(cov[boite], np.clip((rd - n) / CAMO_PIXEL + 0.5, 0.0, 1.0))
    return cov

def camouflage(v):
    X, Y, Z, dedans = points(v)
    liste = taches(v)
    gris = couverture_taches(X, Y, Z, liste, CAMO_GRIS)
    noir = couverture_taches(X, Y, Z, liste, CAMO_NOIR)
    A = np.where(dedans, 1.0, 0.0)
    B = np.where(dedans, np.maximum(CAMO_GRIS * gris * (1 - noir), CAMO_NOIR * noir), 0.0)
    return A, B

def etiqueter(masque):
    """composantes 4-connexes d'un masque booléen (étiquettes 1..n, 0 = fond) ; sans dépendance en plus de numpy"""
    h, w = masque.shape
    lab = np.zeros((h, w), np.int32); n = 0
    m = masque.tolist(); L = lab.tolist()
    for a in range(h):
        for b in range(w):
            if not m[a][b] or L[a][b]: continue
            n += 1; L[a][b] = n; pile = [(a, b)]
            while pile:
                p, q = pile.pop()
                for r, t in ((p + 1, q), (p - 1, q), (p, q + 1), (p, q - 1)):
                    if 0 <= r < h and 0 <= t < w and m[r][t] and not L[r][t]:
                        L[r][t] = n; pile.append((r, t))
    return np.array(L, np.int32), n

def taches_cm(v, B):
    """taille des taches visibles sur le flanc gauche de la texture (zone entière, 0 à 1,60 m) : taches grises (partie visible) et
    noires séparées, celles qui touchent le bord du cadre exclues ; renvoie pour chaque tonalité les diamètres équivalents (cm) et la
    part de sa surface portée par des taches de 20 à 60 cm"""
    v0, v1 = ZONES['gauche']
    zone = B[v0:v1]
    aire_px = (2 * DZ / T) * (H / (v1 - v0)) * 1e4        # cm² par pixel
    res = []
    for masque in ((zone > 0.3) & (zone < 0.8), zone >= 0.8):
        lab, nb = etiqueter(masque)
        bord = np.unique(np.concatenate([lab[0], lab[-1], lab[:, 0], lab[:, -1]]))
        aires = np.bincount(lab.ravel(), minlength=nb + 1).astype(float) * aire_px
        garde = np.ones(nb + 1, bool); garde[0] = False; garde[bord] = False
        garde &= aires >= math.pi * 2.5 ** 2   # liserés d'adoucissement (gris entre noir et blanc, < 5 cm) : pas des taches
        d = 2 * np.sqrt(aires[garde] / math.pi)
        part = aires[garde][(d >= 20) & (d <= 60)].sum() / max(aires[garde].sum(), 1)
        res.append((d, part))
    return res

# couleurs A et B de chaque skin (codes sRGB, mêmes valeurs que SKIN_COULEURS de shaders/vehicule.glsl) : pour l'aperçu seulement
COULEURS = {'rallye': ((0.91, 0.91, 0.89), (0.10, 0.11, 0.125)), 'drifttokyo': ((0.93, 0.30, 0.10), (0.06, 0.06, 0.07)),
            'neonnuit': ((0.0, 0.90, 1.0), (1.0, 0.12, 0.85)), 'camouflage': ((0.85, 0.86, 0.84), (0.09, 0.09, 0.10))}

DESSINS = {'rallye': rallye, 'drifttokyo': drift_tokyo, 'neonnuit': neon_nuit, 'camouflage': camouflage}

# ---------------------------------------------------------------- réduction, marges, mipmaps
def reduire_dessin(img):
    a = np.asarray(img, np.float64) / 255.0
    return a.reshape(T, S, T, S).mean(axis=(1, 3))

def remplir_marges(c):
    """lignes hors zones (marges de 8 px) : copie de la ligne de bord la plus proche (au-dessus, sinon au-dessous)"""
    dedans = np.zeros(T, bool)
    for v0, v1 in ZONES.values(): dedans[v0:v1] = True
    lignes = np.where(dedans)[0]
    for y in np.where(~dedans)[0]:
        c[y] = c[lignes[np.argmin(np.abs(lignes - y) + (lignes > y) * 0.5)]]
    return c

def mipmaps(c):
    niv = [c]
    while niv[-1].shape[0] > 1:
        a = niv[-1]; h, w = a.shape
        niv.append(a.reshape(h // 2, 2, w // 2, 2).mean(axis=(1, 3)))
    return niv

# ---------------------------------------------------------------- EAC R11
def eac_decoder(bloc8):
    """8 octets → 16 valeurs 11 bits, ordre x + 4 y (même calcul que vehicule_skin::decoderEacR11)"""
    base, mult, t = bloc8[0], bloc8[1] >> 4, TABLE[bloc8[1] & 15]
    bits = int.from_bytes(bytes(bloc8[2:8]), 'big')
    out = np.zeros(16, np.int64)
    for i in range(16):
        x, y = i // 4, i % 4
        m = t[(bits >> (45 - 3 * i)) & 7]
        out[x + 4 * y] = min(2047, max(0, base * 8 + 4 + (m * mult * 8 if mult else m)))
    return out

def chercher(blocs, tables, mults):
    """meilleur (base, multiplicateur, table) et indices de chaque bloc (n × 16 valeurs 11 bits), erreur quadratique minimale"""
    n = len(blocs)
    meilleur = np.full(n, np.inf); choix = np.zeros((n, 3), np.int64); idx = np.zeros((n, 16), np.int64)
    tmin, tmax = blocs.min(1), blocs.max(1)
    for ti in tables:
        mods = TABLE[ti]
        for mult in mults:
            off = mods * mult * 8                                   # 8 décalages
            centre = ((tmin + tmax) / 2 - 4 - (off.min() + off.max()) / 2) / 8
            for d in (-1, 0, 1):
                base = np.clip(np.round(centre) + d, 0, 255).astype(np.int64)
                vals = np.clip(base[:, None] * 8 + 4 + off[None, :], 0, 2047)             # n × 8
                err = (blocs[:, :, None] - vals[:, None, :]) ** 2                         # n × 16 × 8
                k = err.argmin(2); e = np.take_along_axis(err, k[:, :, None], 2)[:, :, 0].sum(1)
                mieux = e < meilleur
                if mieux.any():
                    meilleur[mieux] = e[mieux]; choix[mieux, 0] = base[mieux]; choix[mieux, 1] = mult; choix[mieux, 2] = ti; idx[mieux] = k[mieux]
    return choix, idx

def eac_encoder(niveau):
    """niveau h × w (couverture 0..1) → octets EAC R11 (blocs 4 × 4 en lignes), et le niveau décodé (0..1)"""
    h, w = niveau.shape
    H4, W4 = max(4, h), max(4, w)
    a = np.zeros((H4, W4)); a[:h, :w] = niveau
    if h < 4: a[h:, :] = a[h - 1:h, :]
    if w < 4: a[:, w:] = a[:, w - 1:w]
    by, bx = H4 // 4, W4 // 4
    # blocs : pixel i = x × 4 + y (colonnes d'abord, ordre des indices EAC)
    blocs = a.reshape(by, 4, bx, 4).transpose(0, 2, 3, 1).reshape(-1, 16) * 2047.0
    n = len(blocs)
    choix = np.zeros((n, 3), np.int64); idx = np.zeros((n, 16), np.int64)
    # blocs unis (la plupart : peinture seule, ou motif plein) : table 13, multiplicateur 1 (décalages 0, ±8 … : valeur exacte
    # à 4 / 2047 près, 0 et 2047 exacts) ; les autres (bords des motifs) : recherche exhaustive
    unis = blocs.max(1) - blocs.min(1) < 0.5
    for masque, tables, mults in ((unis, (13,), (1,)), (~unis, range(16), range(1, 16))):
        if not masque.any(): continue
        c, k = chercher(blocs[masque], tables, mults)
        choix[masque] = c; idx[masque] = k
    bits = np.zeros(n, np.uint64)
    for i in range(16): bits |= (idx[:, i].astype(np.uint64) << np.uint64(45 - 3 * i))
    out = bytearray()
    for b in range(n):
        out += bytes([int(choix[b, 0]), (int(choix[b, 1]) << 4) | int(choix[b, 2])]) + int(bits[b]).to_bytes(6, 'big')
    # décodé (même formule que le décodeur) pour le contrôle
    dec = np.zeros((n, 16))
    for i in range(16):
        dec[:, i] = np.clip(choix[:, 0] * 8 + 4 + TABLE[choix[:, 2], idx[:, i]] * choix[:, 1] * 8, 0, 2047)
    deco = dec.reshape(by, bx, 4, 4).transpose(0, 3, 1, 2).reshape(H4, W4)[:h, :w] / 2047.0
    return bytes(out), deco

def psnr(ref, dec):
    e = np.mean((ref - dec) ** 2)
    return float('inf') if e == 0 else 10 * math.log10(1.0 / e)

# ---------------------------------------------------------------- principal
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('voitures', nargs='*')
    ap.add_argument('--skin', default='rallye', choices=sorted(DESSINS))
    ap.add_argument('--apercu', default='')
    arg = ap.parse_args()
    mesures = {k: v for k, v in json.load(open(ICI / 'voitures.json')).items() if not k.startswith('_')}
    ids = arg.voitures or list(mesures)
    sortie = RACINE / 'assets' / 'cars' / 'skins'; sortie.mkdir(parents=True, exist_ok=True)
    code = 0
    for vid in ids:
        if vid not in mesures: print(f'{vid} : mesures absentes de voitures.json'); code = 1; continue
        A, B = DESSINS[arg.skin](mesures[vid])
        canaux = [remplir_marges(reduire_dessin(c) if isinstance(c, Image.Image) else c.copy()) for c in (A, B)]   # WFS-493 : dessin en tableau (T × T)
        if arg.skin == 'camouflage':   # WFS-493 (RG-04) : taille des taches, contrôlée sur le flanc gauche
            for nom, (d, part) in zip(('grises', 'noires'), taches_cm(mesures[vid], canaux[1])):
                print(f'{vid}-camouflage : taches {nom} du flanc gauche : {len(d)}, diamètre équivalent médiane {np.median(d):.0f} cm, '
                      f'quartiles {np.percentile(d, 25):.0f} à {np.percentile(d, 75):.0f} cm ; {100 * part:.0f} % de la surface en taches de 20 à 60 cm')
        nivR, nivG = mipmaps(canaux[0]), mipmaps(canaux[1])
        niveaux, pires = [], []
        for k, (r, g) in enumerate(zip(nivR, nivG)):
            er, dr = eac_encoder(r); eg, dg = eac_encoder(g)
            nb = len(er) // 8
            octets = bytearray()
            for b in range(nb): octets += er[b * 8:b * 8 + 8] + eg[b * 8:b * 8 + 8]
            niveaux.append(bytes(octets))
            pires.append(min(psnr(r, dr), psnr(g, dg)))
        if arg.apercu:
            ap_dir = pathlib.Path(arg.apercu); ap_dir.mkdir(parents=True, exist_ok=True)
            img = np.zeros((T, T, 3)); img[:] = (0.11, 0.25, 0.56)                          # peinture Bleu (aperçu)
            for c, col in zip(canaux, COULEURS[arg.skin]):
                img = img * (1 - c[:, :, None]) + np.array(col) * c[:, :, None]
            Image.fromarray((img * 255).astype(np.uint8)).save(ap_dir / f'{vid}-{arg.skin}.png')
        p0, pmin = pires[0], min(pires)
        fichier = sortie / f'{vid}-{arg.skin}.etc2.ktx2'
        if p0 < SEUIL_DB:
            print(f'{vid} : refusée, PSNR niveau 0 {p0:.2f} dB < {SEUIL_DB} dB'); code = 1; continue
        ktx2.ecrire(str(fichier), EAC_RG11, T, T, niveaux)
        print(f'{vid}-{arg.skin} : {fichier.relative_to(RACINE)} : {fichier.stat().st_size / 1048576:.2f} Mo, EAC RG11 {T}×{T}, '
              f'{len(niveaux)} niveaux, PSNR niveau 0 {p0:.2f} dB, plus faible {pmin:.2f} dB')
    return code

if __name__ == '__main__':
    sys.exit(main())
