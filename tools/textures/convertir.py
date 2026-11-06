#!/usr/bin/env python3
# WaForSpeed — WFS-149 (RG-02, RG-03) : conversion d'une texture PNG / JPG / EXR en KTX2 pour le moteur (voir README.md).
#
#   python3 tools/textures/convertir.py <type> <sortie> <entrée> [<entrée G> <entrée B>] [--astc 4x4|6x6] [--rapport f.txt]
#     type   : couleur (sRGB), normale (linéaire, renormalisée à chaque niveau), orm (linéaire : occlusion, rugosité, métal
#              dans R, G, B : une image déjà regroupée, ou trois images en niveaux de gris dans cet ordre)
#              normales : vert vers le haut (« OpenGL », défaut) inversé vers la convention du moteur ; --vert-bas sinon
#     sortie : chemin sans extension → <sortie>.astc.ktx2, <sortie>.etc2.ktx2, <sortie>.rgba8.ktx2
#
# Étapes : lecture → chaîne complète de mipmaps filtrée EN LINÉAIRE (moyenne 2×2 de la lumière, pas des codes sRGB ;
# normales renormalisées à chaque niveau) → compression de chaque niveau par les encodeurs libres (licence Apache-2.0,
# téléchargés par encodeurs.sh) : astcenc 5.2.0 (Arm) pour ASTC, etc2comp (Google) pour ETC2 → contrôle de précision
# (PSNR de chaque niveau décompressé contre le niveau d'origine) → écriture KTX2.
# Refus (code de sortie 1, aucun fichier écrit pour la texture) sous 38 dB (couleur, ORM) ou 35 dB (normales) ; en ASTC,
# un bloc trop grossier (6x6) est d'abord remplacé par un bloc plus fin (4x4) avant de refuser.
# La variante RGBA8 (repli du PC sans ASTC ni ETC2) est l'ASTC DÉCOMPRESSÉ : le PC montre la même image que le téléphone.
import argparse, math, os, pathlib, shutil, subprocess, sys, tempfile
import numpy as np
from PIL import Image

ICI = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(ICI))
import etc2, ktx2   # noqa: E402

SEUILS = {'couleur': 38.0, 'normale': 35.0, 'orm': 38.0}   # dB (RG-03)

# ---------------------------------------------------------------- lecture
def lire_image(chemin):
    """→ tableau h×w×4 float en [0, 1] (codes de l'image : sRGB pour une couleur PNG / JPG ; EXR : valeurs linéaires)"""
    p = str(chemin)
    if p.lower().endswith('.exr'):
        try:
            import OpenEXR, Imath   # module « OpenEXR » (pip install OpenEXR, licence BSD-3)
        except ImportError:
            sys.exit('EXR : installer le module Python OpenEXR (pip install OpenEXR)')
        f = OpenEXR.InputFile(p); dw = f.header()['dataWindow']
        w, h = dw.max.x - dw.min.x + 1, dw.max.y - dw.min.y + 1
        pt = Imath.PixelType(Imath.PixelType.FLOAT)
        canaux = f.header()['channels']
        def c(n, d): return np.frombuffer(f.channel(n, pt), np.float32).reshape(h, w) if n in canaux else np.full((h, w), d, np.float32)
        y = c('Y', 0.0)
        r, g, b = (c('R', 0.0), c('G', 0.0), c('B', 0.0)) if 'R' in canaux else (y, y, y)
        return np.stack([r, g, b, c('A', 1.0)], -1).astype(np.float64), True
    im = Image.open(p).convert('RGBA')
    return np.asarray(im, np.float64) / 255.0, False

def srgb_vers_lin(c): return np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)   # IEC 61966-2-1
def lin_vers_srgb(c):
    c = np.clip(c, 0.0, 1.0)
    return np.where(c <= 0.0031308, c * 12.92, 1.055 * np.power(c, 1 / 2.4) - 0.055)

# ---------------------------------------------------------------- mipmaps (en linéaire)
def reduire(a):
    """moyenne 2×2 (dimension impaire : dernière colonne / ligne répétée, comme VkCtx::uploadMipsCpu)"""
    h, w = a.shape[:2]
    nh, nw = max(1, h // 2), max(1, w // 2)
    ys = [np.minimum(np.arange(nh) * 2 + d, h - 1) for d in (0, 1)]
    xs = [np.minimum(np.arange(nw) * 2 + d, w - 1) for d in (0, 1)]
    return sum(a[ys[j]][:, xs[i]] for j in (0, 1) for i in (0, 1)) / 4.0

def niveaux_lineaires(lin, typ):
    out = [lin]
    while out[-1].shape[0] > 1 or out[-1].shape[1] > 1:
        n = reduire(out[-1])
        if typ == 'normale':   # renormalisation de chaque niveau (vecteur unitaire, z ≥ 0)
            v = n[..., :3]; l = np.linalg.norm(v, axis=-1, keepdims=True)
            n = np.concatenate([v / np.maximum(l, 1e-8), n[..., 3:]], -1)
        out.append(n)
    return out

def vers_octets(lin, typ):
    """niveau linéaire → codes 8 bits (uint8 h×w×4) dans l'espace du fichier"""
    if typ == 'couleur': rgb = lin_vers_srgb(lin[..., :3])
    elif typ == 'normale': rgb = lin[..., :3] * 0.5 + 0.5
    else: rgb = lin[..., :3]
    a = np.concatenate([rgb, lin[..., 3:4]], -1)
    return np.clip(np.rint(a * 255.0), 0, 255).astype(np.uint8)

def preparer(typ, entrees, vert_bas=False):
    if typ == 'orm' and len(entrees) == 3:   # trois images en niveaux de gris : occlusion, rugosité, métal
        cs = [lire_image(e)[0][..., 0] for e in entrees]
        if len({c.shape for c in cs}) != 1: sys.exit('ORM : les trois images doivent avoir la même taille')
        img = np.stack(cs + [np.ones_like(cs[0])], -1); exr = False
    elif len(entrees) == 1:
        img, exr = lire_image(entrees[0])
    else:
        sys.exit('une entrée (ou trois pour orm)')
    rgb, a = img[..., :3], img[..., 3:4]
    if typ == 'couleur': lin = rgb if exr else srgb_vers_lin(rgb)
    elif typ == 'normale':
        v = rgb * 2.0 - 1.0; lin = v / np.maximum(np.linalg.norm(v, axis=-1, keepdims=True), 1e-8)
        # convention du moteur : vert = +v (vers les lignes suivantes de l'image, bitangente MikkTSpace de src/render/mesh.h,
        # « DirectX »). Sources « OpenGL » (vert vers le haut : ambientCG NormalGL, Poly Haven nor_gl) : vert inversé.
        if not vert_bas: lin = lin * np.array([1.0, -1.0, 1.0])
    else: lin = rgb
    return niveaux_lineaires(np.concatenate([lin, a], -1), typ)

# ---------------------------------------------------------------- encodeurs
def encodeurs():
    racine = pathlib.Path(os.environ.get('WFS_ENCODEURS', ICI / '.encodeurs'))
    astc = racine / 'astcenc' / 'bin' / ('astcenc-avx2' if 'avx2' in open('/proc/cpuinfo').read() else 'astcenc-sse4.1')
    etc = racine / 'etc2comp' / 'EtcTool'
    if not astc.is_file() or not etc.is_file(): sys.exit(f'encodeurs absents de {racine} : lancer tools/textures/encodeurs.sh')
    return str(astc), str(etc)

def lancer(cmd):
    r = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if r.returncode != 0: sys.exit('échec : ' + ' '.join(cmd) + '\n' + r.stdout[-2000:])

def png(chemin, octets, w4=False):
    """écrit un niveau en PNG RGBA ; w4 : complété au multiple de 4 (bord répété) pour etc2comp"""
    a = octets
    if w4:
        h, w = a.shape[:2]
        a = np.pad(a, ((0, (-h) % 4), (0, (-w) % 4), (0, 0)), mode='edge')
    Image.fromarray(a, 'RGBA').save(chemin)

def astc_niveau(astcenc, tmp, octets, srgb, bloc, k):
    src, dst, dec = f'{tmp}/a{k}.png', f'{tmp}/a{k}.astc', f'{tmp}/a{k}-dec.png'
    png(src, octets)
    lancer([astcenc, '-cs' if srgb else '-cl', src, dst, bloc, '-thorough', '-j', '1', '-silent'])
    lancer([astcenc, '-ds' if srgb else '-dl', dst, dec, '-j', '1', '-silent'])
    donnees = open(dst, 'rb').read()[16:]   # en-tête .astc de 16 octets (magie 0x5CA1AB13, bloc, taille), puis les blocs
    return donnees, np.asarray(Image.open(dec).convert('RGBA'), np.uint8)

def ktx1_donnees(chemin):
    """données du niveau unique d'un .ktx (KTX 1, sortie d'EtcTool)"""
    import struct
    b = open(chemin, 'rb').read()
    kv = struct.unpack_from('<I', b, 60)[0]
    n = struct.unpack_from('<I', b, 64 + kv)[0]
    return b[68 + kv:68 + kv + n]

def etc2_niveau(etctool, tmp, octets, srgb, typ, k):
    h, w = octets.shape[:2]
    src, dst = f'{tmp}/e{k}.png', f'{tmp}/e{k}.ktx'
    png(src, octets, w4=True)
    lancer([etctool, src, '-format', 'SRGBA8' if srgb else 'RGBA8', '-effort', '100', '-errormetric', 'numeric', '-j', '1', '-output', dst])
    donnees = ktx1_donnees(dst)
    attendu = ((w + 3) // 4) * ((h + 3) // 4) * 16
    if len(donnees) != attendu: sys.exit(f'etc2comp : {len(donnees)} octets pour {w}x{h} ({attendu} attendus)')
    return donnees, etc2.decoder(donnees, w, h)

# ---------------------------------------------------------------- précision (RG-03)
def psnr(ref, dec, alpha):
    c = 4 if alpha else 3
    mse = np.mean((ref[..., :c].astype(np.float64) - dec[..., :c].astype(np.float64)) ** 2)
    return float('inf') if mse == 0 else 10 * math.log10(255.0 ** 2 / mse), mse

def mesurer(refs, decs, alpha):
    """PSNR du niveau 0 et de toute la chaîne (erreur quadratique pondérée par le nombre de pixels)"""
    p0, _ = psnr(refs[0], decs[0], alpha)
    tot = sum(psnr(r, d, alpha)[1] * r.shape[0] * r.shape[1] for r, d in zip(refs, decs))
    npx = sum(r.shape[0] * r.shape[1] for r in refs)
    mse = tot / npx
    return p0, (float('inf') if mse == 0 else 10 * math.log10(255.0 ** 2 / mse))

def fmt_db(x): return 'sans perte' if x == float('inf') else f'{x:.2f} dB'

def main():
    ap = argparse.ArgumentParser(description='PNG / JPG / EXR → KTX2 (ASTC, ETC2, RGBA8) avec mipmaps et contrôle PSNR')
    ap.add_argument('type', choices=sorted(SEUILS)); ap.add_argument('sortie'); ap.add_argument('entrees', nargs='+')
    ap.add_argument('--astc', choices=['6x6', '4x4'], default='6x6', help='bloc ASTC de départ (6x6 ; 4x4 si le PSNR est trop bas)')
    ap.add_argument('--vert-bas', action='store_true', help='normales déjà « DirectX » (vert vers le bas) : pas d\'inversion du vert')
    ap.add_argument('--rapport', help='fichier où ajouter les lignes du rapport de compression')
    a = ap.parse_args()
    astcenc, etctool = encodeurs()
    lins = preparer(a.type, a.entrees, a.vert_bas)
    refs = [vers_octets(l, a.type) for l in lins]
    w, h = refs[0].shape[1], refs[0].shape[0]
    srgb, seuil = a.type == 'couleur', SEUILS[a.type]
    alpha = bool(np.any(refs[0][..., 3] != 255))
    nom = pathlib.Path(a.sortie).name
    lignes, refus = [], []
    with tempfile.TemporaryDirectory() as tmp:
        sorties = {}
        # ASTC : bloc de départ, puis 4x4 si le PSNR n'atteint pas le seuil
        for bloc in ([a.astc, '4x4'] if a.astc == '6x6' else ['4x4']):
            res = [astc_niveau(astcenc, tmp, r, srgb, bloc, k) for k, r in enumerate(refs)]
            p0, pc = mesurer(refs, [d for _, d in res], alpha)
            ok = min(p0, pc) >= seuil
            lignes.append((nom, a.type, f'astc {bloc}', p0, pc, ok, sum(len(d) for d, _ in res)))
            if ok:
                sorties['astc'] = (ktx2.vk_format('astc', srgb, bloc), [d for d, _ in res])
                sorties['rgba8'] = (ktx2.vk_format('rgba8', srgb), [dec.tobytes() for _, dec in res])   # ASTC décompressé
                p0r, pcr = mesurer(refs, [dec for _, dec in res], alpha)
                lignes.append((nom, a.type, 'rgba8 (ASTC décompressé)', p0r, pcr, True, sum(dec.nbytes for _, dec in res)))
                break
        else:
            refus.append('astc')
        res = [etc2_niveau(etctool, tmp, r, srgb, a.type, k) for k, r in enumerate(refs)]
        p0, pc = mesurer(refs, [d for _, d in res], alpha)
        ok = min(p0, pc) >= seuil
        lignes.append((nom, a.type, 'etc2 rgba', p0, pc, ok, sum(len(d) for d, _ in res)))
        if ok: sorties['etc2'] = (ktx2.vk_format('etc2', srgb), [d for d, _ in res])
        else: refus.append('etc2')
        if not refus:
            pathlib.Path(a.sortie).parent.mkdir(parents=True, exist_ok=True)
            for v, (f, niv) in sorties.items(): ktx2.ecrire(f'{a.sortie}.{v}.ktx2', f, w, h, niv)
    texte = ''.join(f'{n} | {t} | {v} | {w}x{h}, {len(refs)} niveaux | {o / 1024:.1f} Ko | niveau 0 {fmt_db(p0)} | chaîne {fmt_db(pc)} | '
                    f'seuil {seuil:.0f} dB | {"conforme" if ok else "REFUSÉ"}\n' for (n, t, v, p0, pc, ok, o) in lignes)
    if refus: texte += f'{nom} | REFUSÉE : {", ".join(refus)} sous {seuil:.0f} dB, aucun fichier écrit\n'
    sys.stdout.write(texte)
    if a.rapport:
        with open(a.rapport, 'a', encoding='utf-8') as f: f.write(texte)
    return 1 if refus else 0

if __name__ == '__main__':
    sys.exit(main())
