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

DESSINS = {'rallye': rallye}

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
        canaux = [remplir_marges(reduire_dessin(A)), remplir_marges(reduire_dessin(B))]
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
            for c, col in ((canaux[0], (0.91, 0.91, 0.89)), (canaux[1], (0.10, 0.11, 0.125))):
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
