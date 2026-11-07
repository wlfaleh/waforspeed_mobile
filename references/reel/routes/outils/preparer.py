#!/usr/bin/env python3
# WaForSpeed — WFS-205 (RG-01, RG-02) : bibliothèque des revêtements de route : scans CC0 (Poly Haven) → KTX2 calibrés.
#
#   python3 -I references/reel/routes/outils/preparer.py <dossier des sources> [--seulement id,id]
#     <dossier des sources>/<id>/ : les fichiers *_diff_2k.*, *_nor_gl_2k.*, et *_arm_2k.* (occlusion, rugosité, métal) ou
#     *_rough_2k.* (rugosité seule : occlusion 1, métal 0), tels que téléchargés (SHA-256 au journal de WFS-205)
#
# Étapes, pour chaque revêtement de CIBLES (plages : revetements.json, expert-realisme-3d 2026-11-07) :
#   1. albédo (RG-02) : diff en linéaire, moyenne de luminance Rec. 709 ; gain k = centre de la plage / moyenne, appliqué aux trois
#      canaux en linéaire (teinte et contraste relatif gardés) ; 1er centile ≥ 0,02 et 99e ≤ 0,9 (extrêmes tassés seulement) ;
#   2. rugosité (M-02) : r' = a·r + b, moyenne au centre de la plage, écart-type gardé (a réduit s'il fait sortir de la plage) ;
#   3. réduction (décision du PO, WFS-205 Suivi 01:40:18) : couleur et normale 2K → 1K (moyenne 2×2 en linéaire ; normales
#      renormalisées), ORM → 512 ; KTX2 par tools/textures/convertir.py (WFS-149) : ASTC 6x6, 4x4 si le PSNR est trop bas
#      (cas des scans de chaussée : bruit fin) ;
#   4. budget : ETC2 non gardé (refusé sous le seuil ; repli RGBA8) ; RGBA8 (banc PC, GPU sans ASTC, exclu de l'APK) gardé SANS
#      son premier niveau (512 pour couleur et normale, 256 pour l'ORM : ≈ 4 mm par texel, un pixel du banc 1280 × 720 à 3 m) ;
#   5. rapport : references/reel/routes/revetements.json (k, a, b, moyennes avant / après, fichiers et octets).
# Écrit dans assets/textures/route/<id>/route_<id>-couleur|-normale|-orm.{astc,rgba8}.ktx2 (préfixe « route_ » : motif
# d'exclusion des replis RGBA8 de l'APK, android/app/build.gradle).
import argparse, json, pathlib, subprocess, sys, tempfile
import numpy as np
from PIL import Image

RACINE = pathlib.Path(__file__).resolve().parents[4]
sys.path.insert(0, str(RACINE / 'tools' / 'textures'))
import ktx2   # noqa: E402  (outil du dépôt, WFS-149)

# id : (scan Poly Haven, albédo min, max, rugosité min, max, côté du scan en m) — plages : expert-realisme-3d (journal de WFS-205)
CIBLES = {
    'enrobe-neuf':     ('asphalt_02',          0.04, 0.07, 0.75, 0.90, 2.0),
    'enrobe-use':      ('worn_asphalt',        0.10, 0.18, 0.75, 0.90, 2.0),
    'enrobe-campagne': ('asphalt_04',          0.08, 0.14, 0.82, 0.92, 2.0),
    'beton-circuit':   ('aerial_asphalt_01',   0.05, 0.09, 0.72, 0.85, 2.0),
    'beton-trottoir':  ('concrete_pavement',   0.25, 0.35, 0.80, 0.95, 2.0),
    'paves-granit':    ('cobblestone_floor_08', 0.20, 0.35, 0.60, 0.80, 2.0),
    'gravier':         ('gravel_road',         0.20, 0.35, 0.90, 1.00, 2.0),
}
SAUT_RGBA8 = 1   # niveau retiré de la variante RGBA8 (1024 → 512, 512 → 256)

def srgb_vers_lin(c): return np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)
def lin_vers_srgb(c):
    c = np.clip(c, 0.0, 1.0)
    return np.where(c <= 0.0031308, c * 12.92, 1.055 * np.power(c, 1 / 2.4) - 0.055)
def luminance(lin): return lin[..., 0] * 0.2126 + lin[..., 1] * 0.7152 + lin[..., 2] * 0.0722
def charger(p): return np.asarray(Image.open(p).convert('RGB'), np.float64) / 255.0
def sauver(p, a): Image.fromarray(np.clip(np.rint(a * 255.0), 0, 255).astype(np.uint8)).save(p)

def trouver(dossier, motif):
    l = sorted(dossier.glob(f'*_{motif}_2k.*'))
    return l[0] if l else None

def calibrer_albedo(diff, amin, amax):
    lin = srgb_vers_lin(diff)
    y = luminance(lin); avant = float(y.mean())
    centre = 0.5 * (amin + amax)
    k = centre / avant
    out = lin * k
    yo = luminance(out)
    p1, p99 = np.percentile(yo, 1), np.percentile(yo, 99)
    # extrêmes tassés seulement (le cœur de la distribution garde le gain k)
    if p99 > 0.9: out = np.where(yo[..., None] > 0.9 * 0.97, out * (0.9 / max(p99, 1e-6)), out)
    if p1 < 0.02: out = np.where(yo[..., None] < 0.02, out + (0.02 - yo[..., None]) * 0.5, out)
    apres = float(luminance(out).mean())
    return lin_vers_srgb(out), k, avant, apres, float(np.percentile(luminance(out), 1)), float(np.percentile(luminance(out), 99))

def calibrer_rugosite(r, rmin, rmax):
    m, s = float(r.mean()), float(r.std())
    centre = 0.5 * (rmin + rmax)
    a = 1.0
    # écart-type gardé, sauf si ±2 σ sort de la plage : a réduit
    if 2 * s > 0.5 * (rmax - rmin): a = 0.5 * (rmax - rmin) / (2 * s)
    b = centre - a * m
    out = np.clip(a * r + b, 0.0, 1.0)
    return out, a, b, m, float(out.mean())

def reduire2(a):   # moyenne 2×2 (valeurs linéaires)
    return 0.25 * (a[0::2, 0::2] + a[1::2, 0::2] + a[0::2, 1::2] + a[1::2, 1::2])
def reduire_normale(n):   # normales codées [0, 1] → moyenne 2×2 des vecteurs, renormalisée
    v = reduire2(n * 2.0 - 1.0); v /= np.maximum(np.linalg.norm(v, axis=-1, keepdims=True), 1e-6)
    return v * 0.5 + 0.5

def convertir(typ, sortie, entree, rapport, refus_ok=False):
    cmd = [sys.executable, str(RACINE / 'tools/textures/convertir.py'), typ, str(sortie), str(entree), '--rapport', str(rapport)]
    r = subprocess.run(cmd, capture_output=True, text=True)
    sys.stdout.write(r.stdout); sys.stderr.write(r.stderr)
    if r.returncode != 0 and not refus_ok: sys.exit(f'{sortie} : conversion refusée')
    return r.returncode == 0

def flou_periodique(a, sigma):
    """flou gaussien séparable, bords repliés (la texture se répète : pas de couture)"""
    r = int(np.ceil(3 * sigma)); x = np.arange(-r, r + 1); k = np.exp(-x * x / (2 * sigma * sigma)); k /= k.sum()
    out = sum(w * np.roll(a, int(d), axis=0) for d, w in zip(x, k))
    return sum(w * np.roll(out, int(d), axis=1) for d, w in zip(x, k))

def alleger(sortie):
    """ETC2 retiré ; RGBA8 sans ses SAUT_RGBA8 premiers niveaux. → octets ASTC, octets RGBA8"""
    e = pathlib.Path(f'{sortie}.etc2.ktx2')
    if e.exists(): e.unlink()
    p = pathlib.Path(f'{sortie}.rgba8.ktx2')
    fmt, w, h, niveaux = ktx2.lire(p)
    niveaux = niveaux[SAUT_RGBA8:]
    ktx2.ecrire(p, fmt, max(1, w >> SAUT_RGBA8), max(1, h >> SAUT_RGBA8), niveaux)
    return pathlib.Path(f'{sortie}.astc.ktx2').stat().st_size, p.stat().st_size

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('sources'); ap.add_argument('--seulement', default='')
    a = ap.parse_args()
    src = pathlib.Path(a.sources)
    choix = [s for s in a.seulement.split(',') if s] or list(CIBLES)
    rapport_json = RACINE / 'references/reel/routes/revetements.json'
    doc = json.loads(rapport_json.read_text(encoding='utf-8')) if rapport_json.exists() else {}
    doc.setdefault('revetements', {})
    psnr = RACINE / 'references/reel/routes/textures-psnr.txt'
    for rid in choix:
        scan, amin, amax, rmin, rmax, cote = CIBLES[rid]
        d = src / rid
        diff, nor = trouver(d, 'diff'), trouver(d, 'nor_gl')
        arm, rough = trouver(d, 'arm'), trouver(d, 'rough')
        if not diff or not nor or not (arm or rough): sys.exit(f'{rid} : sources incomplètes dans {d}')
        dest = RACINE / 'assets/textures/route' / rid; dest.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory() as tmp:
            tmp = pathlib.Path(tmp)
            coul, k, avant, apres, p1, p99 = calibrer_albedo(charger(diff), amin, amax)
            sauver(tmp / 'couleur.png', lin_vers_srgb(reduire2(srgb_vers_lin(coul))))   # 1K, moyenne en linéaire
            if arm:
                orm = charger(arm)   # R occlusion, G rugosité, B métal (linéaires)
            else:
                g = charger(rough)[..., 0]; orm = np.stack([np.ones_like(g), g, np.zeros_like(g)], -1)
            orm[..., 2] = 0.0   # chaussée : jamais métallique (WFS-142)
            g, ga, gb, gavant, gapres = calibrer_rugosite(orm[..., 1], rmin, rmax)
            orm[..., 1] = g
            sauver(tmp / 'orm.png', reduire2(reduire2(orm)))   # 512
            normale = reduire_normale(np.asarray(Image.open(nor).convert('RGB'), np.float64) / 255.0)   # 1K
            sauver(tmp / 'normale.png', normale)
            octets = {}; flou = 0.0
            for typ, nom, fic in (('couleur', 'couleur', 'couleur.png'), ('normale', 'normale', 'normale.png'), ('orm', 'orm', 'orm.png')):
                sortie = dest / f'route_{rid}-{nom}'
                if typ == 'normale':
                    # bruit très fin (scan usé) : ASTC 4x4 sous 35 dB → lissage léger et périodique, puis nouvel essai (σ noté au rapport)
                    for flou in (0.0, 0.6, 0.9, 1.2):
                        if flou > 0:
                            v = flou_periodique(normale * 2.0 - 1.0, flou); v /= np.maximum(np.linalg.norm(v, axis=-1, keepdims=True), 1e-6)
                            sauver(tmp / fic, v * 0.5 + 0.5)
                        if convertir(typ, sortie, tmp / fic, psnr, refus_ok=flou < 1.2): break
                else:
                    convertir(typ, sortie, tmp / fic, psnr)
                octets[nom] = alleger(sortie)
        doc['revetements'][rid] = {
            'scan': f'Poly Haven « {scan} » (CC0 1.0)', 'cote_m': cote, 'statut_cote': 'E',
            'albedo': {'plage': [amin, amax], 'avant': round(avant, 4), 'gain_k': round(k, 4), 'apres': round(apres, 4),
                       'centile_1': round(p1, 4), 'centile_99': round(p99, 4)},
            'rugosite': {'plage': [rmin, rmax], 'avant': round(gavant, 4), 'a': round(ga, 4), 'b': round(gb, 4), 'apres': round(gapres, 4)},
            'octets': {n: {'astc': o[0], 'rgba8': o[1]} for n, o in octets.items()},
            'normale_flou_sigma_texels_1k': flou,
        }
        print(f'{rid} : albédo {avant:.3f} → {apres:.3f} (k = {k:.3f}) ; rugosité {gavant:.3f} → {gapres:.3f} (a = {ga:.3f}, b = {gb:.3f})')
    rapport_json.write_text(json.dumps(doc, ensure_ascii=False, indent=1) + '\n', encoding='utf-8')

if __name__ == '__main__':
    main()
