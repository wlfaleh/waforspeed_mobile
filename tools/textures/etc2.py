# WaForSpeed — WFS-149 (RG-03) : décodeur ETC2 RGBA8 (EAC alpha + ETC2 couleur : modes individuel, différentiel, T, H,
# plan), d'après la Khronos Data Format Specification 1.3, § 21 « ETC2 compressed texture image formats ». Sert à mesurer
# le PSNR de ce que l'encodeur (etc2comp) a produit : un décodeur faux donnerait un PSNR effondré, jamais un bon.
import numpy as np

TABLE_ETC1 = [(2, 8), (5, 17), (9, 29), (13, 42), (18, 60), (24, 80), (33, 106), (47, 183)]
DISTANCES_TH = [3, 6, 11, 16, 23, 32, 41, 64]
TABLE_EAC = [
    (-3, -6, -9, -15, 2, 5, 8, 14), (-3, -7, -10, -13, 2, 6, 9, 12), (-2, -5, -8, -13, 1, 4, 7, 12), (-2, -4, -6, -13, 1, 3, 5, 12),
    (-3, -6, -8, -12, 2, 5, 7, 11), (-3, -7, -9, -11, 2, 6, 8, 10), (-4, -7, -8, -11, 3, 6, 7, 10), (-3, -5, -8, -11, 2, 4, 7, 10),
    (-2, -6, -8, -10, 1, 5, 7, 9), (-2, -5, -8, -10, 1, 4, 7, 9), (-2, -4, -8, -10, 1, 3, 7, 9), (-2, -5, -7, -10, 1, 4, 6, 9),
    (-3, -4, -7, -10, 2, 3, 6, 9), (-1, -2, -3, -10, 0, 1, 2, 9), (-4, -6, -8, -9, 3, 5, 7, 8), (-3, -5, -7, -9, 2, 4, 6, 8)]

def _c(x): return 0 if x < 0 else (255 if x > 255 else x)
def _e4(x): return (x << 4) | x
def _e5(x): return (x << 3) | (x >> 2)
def _e6(x): return (x << 2) | (x >> 4)
def _e7(x): return (x << 1) | (x >> 6)
def _b(v, haut, bas):   # bits haut..bas (inclus) du mot de 64 bits
    return (v >> bas) & ((1 << (haut - bas + 1)) - 1)

def bloc_couleur(blk):
    """8 octets ETC2 RGB → tableau 4×4×3 (indice [y][x])"""
    v = int.from_bytes(blk, 'big')
    out = np.zeros((4, 4, 3), np.int32)
    idx = [[0] * 4 for _ in range(4)]
    for x in range(4):
        for y in range(4):
            i = x * 4 + y
            idx[y][x] = (_b(v, 16 + i, 16 + i) << 1) | _b(v, i, i)
    if _b(v, 33, 33) == 0:   # mode individuel
        c1 = [_e4(_b(v, 63, 60)), _e4(_b(v, 55, 52)), _e4(_b(v, 47, 44))]
        c2 = [_e4(_b(v, 59, 56)), _e4(_b(v, 51, 48)), _e4(_b(v, 43, 40))]
    else:
        r, g, b = _b(v, 63, 59), _b(v, 55, 51), _b(v, 47, 43)
        def sx(d): return d - 8 if d >= 4 else d
        r2, g2, b2 = r + sx(_b(v, 58, 56)), g + sx(_b(v, 50, 48)), b + sx(_b(v, 42, 40))
        if not 0 <= r2 <= 31:   # mode T
            c1 = [_e4((_b(v, 60, 59) << 2) | _b(v, 57, 56)), _e4(_b(v, 55, 52)), _e4(_b(v, 51, 48))]
            c2 = [_e4(_b(v, 47, 44)), _e4(_b(v, 43, 40)), _e4(_b(v, 39, 36))]
            d = DISTANCES_TH[(_b(v, 35, 34) << 1) | _b(v, 32, 32)]
            pal = [c1, [_c(k + d) for k in c2], c2, [_c(k - d) for k in c2]]
            for y in range(4):
                for x in range(4): out[y][x] = pal[idx[y][x]]
            return out
        if not 0 <= g2 <= 31:   # mode H
            c1 = [_e4(_b(v, 62, 59)), _e4((_b(v, 58, 56) << 1) | _b(v, 52, 52)), _e4((_b(v, 51, 51) << 3) | _b(v, 49, 47))]
            c2 = [_e4(_b(v, 46, 43)), _e4(_b(v, 42, 39)), _e4(_b(v, 38, 35))]
            di = (_b(v, 34, 34) << 2) | (_b(v, 32, 32) << 1)
            if (c1[0] << 16 | c1[1] << 8 | c1[2]) >= (c2[0] << 16 | c2[1] << 8 | c2[2]): di |= 1
            d = DISTANCES_TH[di]
            pal = [[_c(k + d) for k in c1], [_c(k - d) for k in c1], [_c(k + d) for k in c2], [_c(k - d) for k in c2]]
            for y in range(4):
                for x in range(4): out[y][x] = pal[idx[y][x]]
            return out
        if not 0 <= b2 <= 31:   # mode plan
            o = [_e6(_b(v, 62, 57)), _e7((_b(v, 56, 56) << 6) | _b(v, 54, 49)),
                 _e6((_b(v, 48, 48) << 5) | (_b(v, 44, 43) << 3) | _b(v, 41, 39))]
            h = [_e6((_b(v, 38, 34) << 1) | _b(v, 32, 32)), _e7(_b(v, 31, 25)), _e6(_b(v, 24, 19))]
            vv = [_e6(_b(v, 18, 13)), _e7(_b(v, 12, 6)), _e6(_b(v, 5, 0))]
            for y in range(4):
                for x in range(4):
                    out[y][x] = [_c((x * (h[k] - o[k]) + y * (vv[k] - o[k]) + 4 * o[k] + 2) >> 2) for k in range(3)]
            return out
        c1 = [_e5(r), _e5(g), _e5(b)]   # mode différentiel
        c2 = [_e5(r2), _e5(g2), _e5(b2)]
    cw = (_b(v, 39, 37), _b(v, 36, 34))
    flip = _b(v, 32, 32)
    for y in range(4):
        for x in range(4):
            s = (y >= 2) if flip else (x >= 2)
            a, bb = TABLE_ETC1[cw[s]]
            m = (a, bb, -a, -bb)[idx[y][x]]
            base = c2 if s else c1
            out[y][x] = [_c(k + m) for k in base]
    return out

def bloc_alpha(blk):
    """8 octets EAC (alpha de ETC2 RGBA8) → tableau 4×4"""
    base, mult, t = blk[0], blk[1] >> 4, TABLE_EAC[blk[1] & 0xF]
    bits = int.from_bytes(blk[2:8], 'big')
    out = np.zeros((4, 4), np.int32)
    for x in range(4):
        for y in range(4):
            i = x * 4 + y
            out[y][x] = _c(base + t[(bits >> (45 - 3 * i)) & 7] * mult)
    return out

def decoder(donnees, w, h):
    """ETC2 RGBA8 (16 octets par bloc, lignes de blocs) → image h×w×4 (uint8)"""
    bx, by = (w + 3) // 4, (h + 3) // 4
    img = np.zeros((by * 4, bx * 4, 4), np.uint8)
    for j in range(by):
        for i in range(bx):
            o = (j * bx + i) * 16
            img[j * 4:j * 4 + 4, i * 4:i * 4 + 4, 3] = bloc_alpha(donnees[o:o + 8])
            img[j * 4:j * 4 + 4, i * 4:i * 4 + 4, :3] = bloc_couleur(donnees[o + 8:o + 16])
    return img[:h, :w]
