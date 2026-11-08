#!/usr/bin/env python3
# WaForSpeed — WFS-209 (RG-01 à RG-03) : humains réalistes MakeHuman (CC0) → .wfp v3, sur le squelette des personnages du jeu.
#
#   python3 -I tools/convert_people_makehuman.py <dossier MakeHuman> <homme.wfp> <femme.wfp> <dossier de sortie>
#     <dossier MakeHuman> : base.obj, default.mhskel, default_weights.mhw, targets/*.target (cibles « macrodetails » adultes) de
#     makehumancommunity/makehuman (makehuman/data/…, CC0 1.0 : LICENSE.ASSETS.md) ; SHA-256 au journal de WFS-209
#     homme.wfp / femme.wfp : personnages du jeu (Quaternius, CC0) : squelette (65 os, pose de repos en T), palette, clips
#
# Étapes, pour chacun des MORPHOTYPES (12) :
#   1. morphologie : base.obj + cibles macro de MakeHuman pondérées comme son modificateur « macro » (sexe, âge 25-90 ans,
#      musculature, poids, origines africaine / asiatique / européenne) ; corps seul (groupe « body ») ;
#   2. mètres (MakeHuman : décimètres), pieds à y = 0, échelle uniforme à la stature visée (M-01 : moyenne et écart-type
#      d'ANSUR-II, references/reel/anthropometrie.json, exacts sur les 6 hommes et sur les 6 femmes) ;
#   3. reciblage (RG-03) : articulations MakeHuman (default.mhskel) → 65 os du squelette du jeu (mêmes noms, même ordre, mêmes
#      rotations de repos) ; le maillage est mis dans la pose de repos du jeu (bras en T) en tournant chaque segment des membres
#      de sa direction MakeHuman à celle du jeu, par mélange linéaire des poids de MakeHuman (default_weights.mhw) regroupés
#      sur les os du jeu ; clips de homme.wfp / femme.wfp repris tels quels (même squelette), translation de la racine mise
#      à l'échelle du bassin ;
#   4. attribut _CLOTH (WFS-72) : bras (0-1 bras, 1-2 avant-bras, 2-3 main), hauteur (y de repos), jambe (0-1 cuisse, 1-2 jambe,
#      2-3 pied, 3-4 orteils), partie (0 : corps) ; repères du corps (cou, taille, profondeur du cou) comme les personnages du jeu ;
#   5. .wfp version 3 : sommets de 84 octets (tangente ajoutée après l'uv) ; texture de couleur unie (teinte de peau neutre,
#      multipliée par l'apparence), normales plates ; plages « corps-0 », « corps-1 », « corps-2 » (WFS-211 : niveaux de détail,
#      tools/convert_people_lod.py) ; tête et cou jamais habillés (ANO-111).
#   6. WFS-266 (ANO-134), adultes seulement (enfants inchangés) : globes oculaires de MakeHuman en partie 1 (centre du globe dans
#      _cloth.xyz : iris et pupille dessinés par scene_skin.frag) ; cuir chevelu peint en partie 2 (cuir_chevelu, VOLUME_CHEVEUX,
#      comme les enfants) ; 4 femmes sur 6 aux cheveux longs : « Hair_Long » de femme.wfp (Quaternius CC0, comme WFS-240) posé
#      sur la tête et ajouté aux plages corps-<lod> (aucun appel de dessin de plus) ; dents et langue non reprises.
import argparse, hashlib, io, json, math, pathlib, struct, sys
import numpy as np
from PIL import Image

ICI = pathlib.Path(__file__).resolve().parent
sys.dont_write_bytecode = True   # aucun __pycache__ dans le dépôt
sys.path.insert(0, str(ICI))
from convert_people_lod import simplifier   # WFS-211 : niveaux de détail
LOD_PART = (1.0, 0.38, 0.13)
LOD_ECART = (0, 0.006, 0.016)   # écart de forme borné (m) : sous un pixel aux seuils 8 m et 20 m (champ 60°, 1080 px)

# ---------------------------------------------------------------- morphotypes (RG-02)
# sexe 0 femme / 1 homme ; âge en années ; musculature et poids 0..1 (0,5 moyen) ; origines (africaine, asiatique, européenne)
# ; z : écart à la stature moyenne en écarts-types (± 0,463 / 0,926 / 1,389 : moyenne 0 et écart-type 1 exacts sur 6)
Z6 = [-1.389, -0.926, -0.463, 0.463, 0.926, 1.389]
MORPHOTYPES = [
    ('h01', 1, 25, 0.55, 0.45, (0.0, 0.0, 1.0), Z6[3]),
    ('h02', 1, 31, 0.75, 0.55, (1.0, 0.0, 0.0), Z6[5]),
    ('h03', 1, 38, 0.50, 0.50, (0.0, 1.0, 0.0), Z6[0]),
    ('h04', 1, 48, 0.45, 0.80, (0.0, 0.0, 1.0), Z6[4]),
    ('h05', 1, 58, 0.50, 0.65, (0.6, 0.0, 0.4), Z6[1]),
    ('h06', 1, 72, 0.35, 0.40, (0.0, 0.5, 0.5), Z6[2]),
    ('f01', 0, 24, 0.50, 0.40, (0.0, 0.0, 1.0), Z6[2]),
    ('f02', 0, 29, 0.55, 0.50, (1.0, 0.0, 0.0), Z6[4]),
    ('f03', 0, 36, 0.45, 0.55, (0.0, 1.0, 0.0), Z6[0]),
    ('f04', 0, 45, 0.45, 0.80, (0.0, 0.0, 1.0), Z6[5]),
    ('f05', 0, 56, 0.40, 0.60, (0.3, 0.3, 0.4), Z6[3]),
    ('f06', 0, 70, 0.35, 0.50, (0.5, 0.0, 0.5), Z6[1]),
]
STATURE = {1: (1.7562, 0.0686), 0: (1.6285, 0.0642)}   # ANSUR-II : humain.hommes / femmes.stature (m)

# WFS-233 (RG-01) : 8 enfants, garçon (g) et fille (f) à 4, 7, 10 et 14 ans ; stature = médiane (P50) des courbes CDC 2000
# stature-pour-âge (statage.csv, colonne M, à l'âge révolu + 0,5 mois ; references/reel/anthropometrie.json, enfant.*) ;
# proportions : celles des cibles MakeHuman d'enfant (pas de ratio adulte imposé), mesurées et notées « à confirmer »
ENFANTS = [
    ('e04g', 1, 4, 0.50, 0.50, (0.0, 0.0, 1.0), 1.025105),
    ('e04f', 0, 4, 0.50, 0.50, (1.0, 0.0, 0.0), 1.010339),
    ('e07g', 1, 7, 0.50, 0.50, (0.0, 1.0, 0.0), 1.220305),
    ('e07f', 0, 7, 0.50, 0.50, (0.0, 0.0, 1.0), 1.217617),
    ('e10g', 1, 10, 0.50, 0.45, (0.6, 0.0, 0.4), 1.388234),
    ('e10f', 0, 10, 0.50, 0.50, (0.0, 0.5, 0.5), 1.382112),
    ('e14g', 1, 14, 0.50, 0.45, (1.0, 0.0, 0.0), 1.641418),
    ('e14f', 0, 14, 0.45, 0.50, (0.0, 0.0, 1.0), 1.604777),
]
def est_enfant(nom): return nom.startswith('e')

# ---------------------------------------------------------------- correspondance des os MakeHuman → os du jeu
def os_du_jeu(nom_mh):
    """os du jeu (Quaternius) qui reçoit les poids de l'os MakeHuman"""
    n = nom_mh
    c = '_l' if n.endswith('.L') else '_r' if n.endswith('.R') else ''
    b = n[:-2] if c else n
    if b in ('root', 'spine05', 'pelvis'): return 'pelvis'
    if b == 'spine04': return 'spine_01'
    if b in ('spine03', 'spine02'): return 'spine_02'
    if b in ('spine01', 'breast'): return 'spine_03'
    if b.startswith('neck'): return 'neck_01'
    if b == 'clavicle': return 'clavicle' + c
    if b in ('shoulder01', 'upperarm01', 'upperarm02'): return 'upperarm' + c
    if b in ('lowerarm01', 'lowerarm02'): return 'lowerarm' + c
    if b in ('wrist',) or b.startswith('metacarpal'): return 'hand' + c
    if b.startswith('finger'):
        doigt, phal = b[6:].split('-')
        nom = {'1': 'thumb', '2': 'index', '3': 'middle', '4': 'ring', '5': 'pinky'}[doigt]
        return '%s_0%s%s' % (nom, phal, c)
    if b in ('upperleg01', 'upperleg02'): return 'thigh' + c
    if b in ('lowerleg01', 'lowerleg02'): return 'calf' + c
    if b == 'foot': return 'foot' + c
    if b.startswith('toe'): return 'ball' + c
    return 'Head'   # tête, visage, yeux, langue, mâchoire

# position de chaque os du jeu (tête de l'os) d'après les articulations MakeHuman (nom de joint, ou liste à moyenner)
def joint_du_jeu(nom):
    c = nom[-2:] if nom.endswith(('_l', '_r')) else ''
    L = {'_l': '.L', '_r': '.R', '': ''}[c]
    b = nom[:-2] if c else nom
    if b == 'pelvis': return ['root____head']
    if b == 'spine_01': return ['spine04____head']
    if b == 'spine_02': return ['spine03____head']
    if b == 'spine_03': return ['spine01____head']
    if b == 'neck_01': return ['neck01____head']
    if b == 'Head': return ['head____head']
    if b == 'clavicle': return ['clavicle%s____head' % L]
    if b == 'upperarm': return ['upperarm01%s____head' % L]
    if b == 'lowerarm': return ['lowerarm01%s____head' % L]
    if b == 'hand': return ['wrist%s____head' % L]
    for k, d in (('thumb', '1'), ('index', '2'), ('middle', '3'), ('ring', '4'), ('pinky', '5')):
        if b.startswith(k + '_0'):
            ph = b[len(k) + 2:]
            if ph.startswith('4'): return ['finger%s-3%s____tail' % (d, L)]
            return ['finger%s-%s%s____head' % (d, ph, L)]
    if b == 'thigh': return ['upperleg01%s____head' % L]
    if b == 'calf': return ['lowerleg01%s____head' % L]
    if b == 'foot': return ['foot%s____head' % L]
    if b == 'ball': return ['toe%d-1%s____head' % (k, L) for k in range(1, 6)]
    if b == 'ball_leaf': return ['toe%d-%d%s____tail' % (k, 2 if k == 1 else 3, L) for k in range(1, 6)]
    return None

# ---------------------------------------------------------------- lecture
# WFS-266 (RG-02) : globes oculaires de MakeHuman (groupes « helper-l-eye » / « helper-r-eye » de base.obj, mêmes cibles et mêmes
# poids que le corps : os eye.L / eye.R → Head) ; dents, langue, cils et aides de vêtements ne sont pas repris (RG-03 : masqués)
GROUPES_YEUX = ('helper-l-eye', 'helper-r-eye')
def lire_obj(chemin):
    V, VT, F, FY = [], [], [], []
    groupe = None
    for l in open(chemin, encoding='utf-8', errors='replace'):
        if l.startswith('v '): V.append([float(x) for x in l.split()[1:4]])
        elif l.startswith('vt '): VT.append([float(x) for x in l.split()[1:3]])
        elif l.startswith('g '): groupe = l.split()[1]
        elif l.startswith('f ') and (groupe == 'body' or groupe in GROUPES_YEUX):
            (F if groupe == 'body' else FY).append([(int(t.split('/')[0]) - 1, int(t.split('/')[1]) - 1) for t in l.split()[1:]])
    return np.array(V), np.array(VT), F, FY

def lire_cible(chemin, n):
    d = np.zeros((n, 3))
    for l in open(chemin, encoding='utf-8'):
        if not l.strip() or l.startswith('#'): continue
        p = l.split(); d[int(p[0])] = [float(p[1]), float(p[2]), float(p[3])]
    return d

def interp3(v):
    """valeur 0..1 → poids (min, moyen, max) du modificateur macro de MakeHuman"""
    return (max(0.0, (0.5 - v) * 2), 1 - abs(v - 0.5) * 2, max(0.0, (v - 0.5) * 2))

def poids_age(age):
    """poids des cibles d'âge du modificateur macro de MakeHuman (human.py, _setAgeVals) ; curseur : 1 an → 0, 11 ans → 0,1875,
    25 ans → 0,5, 90 ans → 1 (WFS-233 : enfants ; les adultes gardent exactement les poids de WFS-209)"""
    if age >= 25:
        av = min(1.0, (age - 25) / (90 - 25))   # 25 ans → jeune, 90 ans → âgé
        return {'young': 1 - av, 'old': av}
    c = (age - 1) / 10 * 0.1875 if age <= 11 else 0.1875 + (age - 11) / 14 * 0.3125
    young = max(0.0, (c - 0.1875) * 3.2)
    baby = max(0.0, 1 - c * 5.333)
    child = max(0.0, min(1.0, 5.333 * c) - young)
    return {k: v for k, v in (('baby', baby), ('child', child), ('young', young)) if v > 1e-6}

def morpher(V0, dossier, sexe, age, muscle, poids, origines, cache):
    n = len(V0)
    def cible(nom):
        if nom not in cache: cache[nom] = lire_cible(dossier / 'targets' / nom, n)
        return cache[nom]
    ages = poids_age(age)
    g = float(sexe) if age >= 14 else (0.55 if sexe else 0.45)   # WFS-233 : avant la puberté, corps peu différenciés (avis expert)
    sexes = {'female': 1 - g, 'male': g}
    mus = dict(zip(('minmuscle', 'averagemuscle', 'maxmuscle'), interp3(muscle)))
    poi = dict(zip(('minweight', 'averageweight', 'maxweight'), interp3(poids)))
    V = V0.copy()
    for s, ws in sexes.items():
        for a, wa in ages.items():
            for m, wm in mus.items():
                for p, wp in poi.items():
                    w = ws * wa * wm * wp
                    if w > 1e-6: V += w * cible('universal-%s-%s-%s-%s.target' % (s, a, m, p))
            for o, wo in zip(('african', 'asian', 'caucasian'), origines):
                w = ws * wa * wo
                if w > 1e-6: V += w * cible('%s-%s-%s.target' % (o, s, a))
    return V

# ---------------------------------------------------------------- squelette du jeu (homme.wfp / femme.wfp)
def quat_mat(q):
    x, y, z, w = q
    return np.array([[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
                     [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
                     [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]])

def lire_wfp(chemin):
    b = open(chemin, 'rb').read(); o = 4
    ver, = struct.unpack_from('<I', b, o); o += 4
    assert ver == 2, chemin
    nT, = struct.unpack_from('<I', b, o); o += 4
    for i in range(nT):
        n, = struct.unpack_from('<I', b, o); o += 4 + n
    o += 12 + 16
    nV, nI, nP, nO, nC = struct.unpack_from('<5I', b, o); o += 20
    o += nV * 68 + nI * 4 + nP * 40
    O = []
    for i in range(nO):
        nom = b[o:o + 32].split(b'\0')[0].decode(); par, = struct.unpack_from('<i', b, o + 32)
        f = struct.unpack_from('<10f', b, o + 36); o += 32 + 4 + 40 + 64
        O.append(dict(nom=nom, parent=par, t=np.array(f[0:3]), r=np.array(f[3:7]), s=np.array(f[7:10])))
    nPal, = struct.unpack_from('<I', b, o); o += 4 + 4 * nPal
    return O, b[o:], nC

# WFS-266 (RG-01) : coiffure « Hair_<nom>-<lod> » d'un personnage du jeu (femme.wfp, Quaternius CC0, version 2), comme WFS-240 :
# sommets de la plage (position, normale, uv, os du jeu et poids), triangles renumérotés
def lire_coiffure(chemin, plage):
    b = open(chemin, 'rb').read(); o = 4
    ver, nT = struct.unpack_from('<2I', b, o); o += 8
    assert ver == 2, chemin
    for i in range(nT):
        n, = struct.unpack_from('<I', b, o); o += 4 + n
    o += 28
    nV, nI, nP, nO, nC = struct.unpack_from('<5I', b, o); o += 20
    v = np.frombuffer(b, np.dtype([('p', '<f4', 3), ('n', '<f4', 3), ('uv', '<f4', 2), ('pal', 'u1', 4), ('w', '<f4', 4), ('cl', '<f4', 4)]), nV, o)
    o += nV * 68
    I = np.frombuffer(b, '<u4', nI, o); o += nI * 4
    pl = {}
    for i in range(nP):
        pl[b[o:o + 32].split(b'\0')[0].decode()] = struct.unpack_from('<2I', b, o + 32); o += 40
    o += nO * (32 + 4 + 40 + 64)
    nPal, = struct.unpack_from('<I', b, o)
    palette = np.array(struct.unpack_from('<%dI' % nPal, b, o + 4))
    a, c = pl[plage]
    ix = I[a:a + c]
    u, tri = np.unique(ix, return_inverse=True)
    vv = v[u]
    return dict(P=vv['p'].astype(float), N=vv['n'].astype(float), UV=vv['uv'].astype(float), os=palette[vv['pal']],
                w=vv['w'].astype(float), tri=tri.reshape(-1, 3).astype(np.uint32), nO=nO)

def repos(O):
    R, p = [None] * len(O), [None] * len(O)
    for i, o in enumerate(O):
        Rl = quat_mat(o['r'])
        if o['parent'] < 0: R[i], p[i] = Rl, o['t'].copy()
        else: R[i], p[i] = R[o['parent']] @ Rl, p[o['parent']] + R[o['parent']] @ o['t']
    return R, p

def rotation_entre(a, b):
    """rotation minimale qui amène la direction a sur la direction b"""
    a = a / np.linalg.norm(a); b = b / np.linalg.norm(b)
    v = np.cross(a, b); c = float(np.dot(a, b))
    if c < -0.999999: return -np.eye(3)
    k = np.array([[0, -v[2], v[1]], [v[2], 0, -v[0]], [-v[1], v[0], 0]])
    return np.eye(3) + k + k @ k / (1 + c)

def mise_a_echelle_clips(clips, nC, os_racine, k):
    """clips du .wfp : translations de l'os racine multipliées par k (autres canaux inchangés)"""
    out = bytearray(clips); o = 0
    for c in range(nC):
        nk, = struct.unpack_from('<I', out, o + 36); o += 40
        for i in range(nk):
            os_, ch, n = struct.unpack_from('<3I', out, o); o += 12; o += 4 * n
            m = n * (4 if ch == 1 else 3)
            if ch == 0 and os_ == os_racine:
                vals = np.frombuffer(bytes(out[o:o + 4 * m]), '<f4') * k
                out[o:o + 4 * m] = vals.astype('<f4').tobytes()
            o += 4 * m
    return bytes(out)


def normales_sommets(P, F):
    """normales des sommets de la topologie MakeHuman (faces du groupe « body », indices des sommets d'origine)"""
    N = np.zeros_like(P)
    for f in F:
        ids = [v for v, _ in f]
        for k in range(1, len(ids) - 1):
            a, b, c = P[ids[0]], P[ids[k]], P[ids[k + 1]]
            n = np.cross(b - a, c - a)
            for i in (ids[0], ids[k], ids[k + 1]): N[i] += n
    return N / np.maximum(np.linalg.norm(N, axis=1, keepdims=True), 1e-12)

# ---------------------------------------------------------------- mesures M-01 / M-02 (mêmes définitions que tests/fidelite_humains.cpp)
RATIOS = {1: (0.4815, 1.033), 0: (0.4802, 1.0195)}   # ANSUR-II : ratio_crotchheight, ratio_span (hommes, femmes)
def mesurer(P, cou_y, N):
    """stature, entrejambe (point le plus bas de la ligne médiane dont la normale regarde vers le bas, entre 30 et 60 % de la
    stature : les jambes jointes ne comptent pas), envergure (bras en T), menton (point le plus bas de la ligne médiane qui
    avance de 3 cm devant le cou, au-dessus du cou − 2 cm)"""
    y0 = float(P[:, 1].min()); P = P - [0, y0, 0]; cou_y -= y0   # hauteurs comptées depuis le dessous des pieds
    st = float(P[:, 1].max())
    m = np.abs(P[:, 0]) < 0.008
    mid = P[m]
    bas = m & (N[:, 1] < -0.5) & (P[:, 1] > 0.3 * st) & (P[:, 1] < 0.6 * st)
    entre = float(P[bas, 1].min())
    env = float(P[:, 0].max() - P[:, 0].min())
    zcou = float(mid[(mid[:, 1] >= cou_y - 0.02) & (mid[:, 1] <= cou_y)][:, 2].max())
    devant = mid[(mid[:, 1] > cou_y - 0.02) & (mid[:, 2] > zcou + 0.03)]
    menton = float(devant[:, 1].min()) if len(devant) else cou_y
    return st, entre, env, menton   # menton : hauteur au-dessus des pieds

def ajuster_proportions(V, pos, W, idx, sexe, noms, normales, corps):
    """M-02 : jambes (étirement vertical sous l'entrejambe) et bras (étirement horizontal au-delà de l'épaule, pondéré par les
    poids des os du bras) amenés aux ratios d'ANSUR-II, tête à 1/7,5 de la stature ; 6 itérations"""
    rc, rs = RATIOS[sexe]
    bras = [i for i, n in enumerate(noms) if n.split('_')[0] in ('upperarm', 'lowerarm', 'hand', 'thumb', 'index', 'middle', 'ring', 'pinky')]
    wb = W[:, bras].sum(1)
    wt = W[:, idx['Head']]   # tête, visage, yeux, mâchoire (regroupés sur Head)
    for it in range(6):
        st, entre, env, _ = mesurer(V[corps], pos[idx['neck_01']][1], normales(V)[corps])
        sL = rc * (st - entre) / (entre * (1 - rc))
        entre += float(V[corps, 1].min())   # hauteur de l'entrejambe dans le repère du maillage
        for X in (V, pos):
            y = X[:, 1].copy()
            X[:, 1] = np.where(y < entre, entre - (entre - y) * sL, y)   # entrejambe fixe, jambes étirées vers le bas
        st = float(V[corps, 1].max() - V[corps, 1].min())
        xs = abs(float(pos[idx['upperarm_l']][0]))
        xm = float(np.abs(V[corps, 0]).max())
        sA = (rs * st / 2 - xs) / (xm - xs)
        def etire(x, w):
            ax = np.abs(x); e = np.where(ax > xs, np.sign(x) * (xs + (ax - xs) * sA), x)
            return x + w * (e - x)
        V[:, 0] = etire(V[:, 0], wb)
        pos[:, 0] = np.where(np.abs(pos[:, 0]) > xs, np.sign(pos[:, 0]) * (xs + (np.abs(pos[:, 0]) - xs) * sA), pos[:, 0])
        # tête (M-02 : hauteur vertex-menton = stature / 7,5) : agrandie autour de la base du crâne (articulation Head), pondérée par le poids de l'os Head
        y0 = float(V[corps, 1].min()); pivot = pos[idx['Head']].copy()   # base du crâne : le cou ne s'allonge pas
        st, entre, env, menton = mesurer(V[corps], pos[idx['neck_01']][1], normales(V)[corps])
        h = st - menton; a = st - (pivot[1] - y0)
        sH = (st - a) / (7.5 * h - a)
        V[:] = V + wt[:, None] * ((V - pivot) * sH + pivot - V)
    return V, pos

# ---------------------------------------------------------------- un morphotype
def convertir(mh, jeu, morpho, sortie, cache):
    nom, sexe, age, muscle, poids, origines, z = morpho
    V0, VT, F = mh['V'], mh['VT'], mh['F']
    V = morpher(V0, mh['dossier'], sexe, age, muscle, poids, origines, cache) * 0.1   # décimètres → mètres
    sk, poidsMH = mh['sk'], mh['poids']
    J = {j: V[np.array(ix)].mean(0) for j, ix in sk['joints'].items()}
    corps = sorted({i for f in F for i, _ in f})
    sol = V[corps, 1].min(); haut = V[corps, 1].max()
    stature = z if est_enfant(nom) else STATURE[sexe][0] + z * STATURE[sexe][1]   # WFS-233 : stature CDC de l'enfant
    k = stature / (haut - sol)
    org = np.array([0.0, sol, 0.0])
    V = (V - org) * k
    J = {j: (p - org) * k for j, p in J.items()}
    O, clips, nC = jeu[sexe]
    noms = [o['nom'] for o in O]; idx = {n: i for i, n in enumerate(noms)}
    Rq, pq = repos(O)
    # positions MakeHuman de chaque os du jeu (tête de l'os) ; racine au sol sous le bassin
    pm = []
    for n in noms:
        js = joint_du_jeu(n)
        if n == 'root': pm.append(np.zeros(3)); continue
        pm.append(np.mean([J[j] for j in js], 0))
    pm = np.array(pm)
    # poids MakeHuman regroupés sur les os du jeu (4 au plus par sommet)
    nV = len(V)
    W = np.zeros((nV, len(O)))
    for bone, lst in poidsMH.items():
        q = idx[os_du_jeu(bone)]
        for v, w in lst: W[v, q] += w
    # enfant « directeur » de chaque os (pour la direction du segment) : bras, jambes, doigts, clavicule
    dir_enfant = {}
    for i, o in enumerate(O):
        p = o['parent']
        if p >= 0 and p not in dir_enfant: dir_enfant[p] = i
    for c in ('_l', '_r'):
        dir_enfant[idx['hand' + c]] = idx['middle_01' + c]
        dir_enfant[idx['foot' + c]] = idx['ball' + c]
    tournes = {i for i, n in enumerate(noms) if any(n.startswith(p) for p in
               ('clavicle', 'upperarm', 'lowerarm', 'hand', 'thumb', 'index', 'middle', 'ring', 'pinky', 'thigh', 'calf', 'foot', 'ball'))}
    # transformations de pose (repère du modèle) : x → p'(b) + R_b (x − p(b)), hiérarchiques
    Rp = [np.eye(3)] * len(O); pp = [None] * len(O)
    for i, o in enumerate(O):
        par = o['parent']
        if par < 0: pp[i] = pm[i].copy(); continue
        pp[i] = pp[par] + Rp[par] @ (pm[i] - pm[par])
        if i in tournes and i in dir_enfant:
            c = dir_enfant[i]
            dm = Rp[par] @ (pm[c] - pm[i])                  # direction MakeHuman déjà portée par le parent
            dq = pq[c] - pq[i]                              # direction du jeu (pose de repos)
            Rp[i] = rotation_entre(dm, dq) @ Rp[par]
        else:
            Rp[i] = Rp[par]
    # sommets mis en pose (mélange linéaire)
    Ws = W / np.maximum(W.sum(1, keepdims=True), 1e-9)
    Vp = np.zeros_like(V)
    for i in range(len(O)):
        m = Ws[:, i] > 0
        if m.any(): Vp[m] += Ws[m, i:i + 1] * ((V[m] - pm[i]) @ Rp[i].T + pp[i])
    sans = Ws.sum(1) < 0.5
    Vp[sans] = V[sans]
    pos_os = np.array(pp)
    # M-02 : proportions d'ANSUR-II, puis M-01 : stature visée (échelle uniforme, pieds au sol)
    if not est_enfant(nom):   # WFS-233 : ratios d'adultes (ANSUR-II) seulement chez l'adulte
        Vp, pos_os = ajuster_proportions(Vp, pos_os, Ws, idx, sexe, noms, lambda X: normales_sommets(X, mh['F']), corps)
    sol = float(Vp[corps, 1].min()); k2 = stature / (float(Vp[corps, 1].max()) - sol)
    Vp = (Vp - [0, sol, 0]) * k2; pos_os = (pos_os - [0, sol, 0]) * k2; pos_os[idx['root']] = 0
    return dict(nom=nom, sexe=sexe, V=Vp, W=Ws, O=O, Rq=Rq, pos=pos_os, clips=clips, nC=nC, stature=stature,
                k_racine=(pos_os[idx['pelvis']][1] / pq[idx['pelvis']][1]), idx=idx)

# ---------------------------------------------------------------- maillage, attributs, écriture
CHAINES = {'bras': ('upperarm', 'lowerarm', 'hand'), 'jambe': ('thigh', 'calf', 'foot', 'ball')}

def maillage(r, mh, yeux=False):
    """sommets uniques (position, uv) du groupe « body » (et des globes oculaires si yeux : WFS-266), triangles, normales,
    tangentes ; masque des sommets des yeux"""
    F, VT = mh['F'] + (mh['FY'] if yeux else []), mh['VT']
    nF = len(mh['F'])
    cle, sv, tri, oeil = {}, [], [], []
    for k_f, f in enumerate(F):
        ids = []
        for v, t in f:
            if (v, t) not in cle: cle[(v, t)] = len(sv); sv.append((v, t)); oeil.append(k_f >= nF)
            ids.append(cle[(v, t)])
        for k in range(1, len(ids) - 1): tri.append((ids[0], ids[k], ids[k + 1]))
    sv = np.array(sv); tri = np.array(tri, dtype=np.uint32)
    P = r['V'][sv[:, 0]]; UV = VT[sv[:, 1]]; UV = np.stack([UV[:, 0], 1 - UV[:, 1]], 1)   # v vers le bas (glTF)
    a, b, c = P[tri[:, 0]], P[tri[:, 1]], P[tri[:, 2]]
    fn = np.cross(b - a, c - a)
    N = np.zeros_like(P)
    # normales lissées par position (les coutures d'uv ne cassent pas l'éclairage)
    for k in range(3): np.add.at(N, tri[:, k], fn)
    pos_cle = {}
    for i, v in enumerate(sv[:, 0]): pos_cle.setdefault(v, []).append(i)
    for v, l in pos_cle.items():
        if len(l) > 1: N[l] = N[l].sum(0)
    N /= np.maximum(np.linalg.norm(N, axis=1, keepdims=True), 1e-12)
    # tangentes (dérivées des uv par triangle, orthogonalisées), signe de la bitangente en w
    du1, du2 = UV[tri[:, 1]] - UV[tri[:, 0]], UV[tri[:, 2]] - UV[tri[:, 0]]
    e1, e2 = b - a, c - a
    det = du1[:, 0] * du2[:, 1] - du2[:, 0] * du1[:, 1]
    det = np.where(np.abs(det) < 1e-12, 1e-12, det)
    T = (e1 * du2[:, 1:2] - e2 * du1[:, 1:2]) / det[:, None]
    B = (e2 * du1[:, 0:1] - e1 * du2[:, 0:1]) / det[:, None]
    Tv = np.zeros_like(P); Bv = np.zeros_like(P)
    for k in range(3): np.add.at(Tv, tri[:, k], T); np.add.at(Bv, tri[:, k], B)
    Tv -= N * (Tv * N).sum(1, keepdims=True)
    Tv /= np.maximum(np.linalg.norm(Tv, axis=1, keepdims=True), 1e-12)
    w = np.where((np.cross(N, Tv) * Bv).sum(1) < 0, -1.0, 1.0)
    return sv, tri, P, N, UV, np.concatenate([Tv, w[:, None]], 1), np.array(oeil, dtype=bool)

def attribut_cloth(r, P, Wv, cheveux=None):
    """_cloth (bras, hauteur, jambe, partie) de chaque sommet (WFS-72)"""
    noms = [o['nom'] for o in r['O']]; idx = r['idx']; pos = r['pos']
    cl = np.full((len(P), 4), -1.0); cl[:, 1] = P[:, 1]; cl[:, 3] = 0.0
    dom = Wv.argmax(1)
    for ch, segs in CHAINES.items():
        col = 0 if ch == 'bras' else 2
        for c in ('_l', '_r'):
            ids = [idx[s + c] for s in segs]
            fin = idx[('middle_04_leaf' if ch == 'bras' else 'ball_leaf') + c]
            bouts = [pos[i] for i in ids] + [pos[fin]]
            for k, i in enumerate(ids):
                famille = {i}
                if segs[k] == 'hand': famille |= {j for j, n in enumerate(noms) if n.endswith(c) and n.split('_')[0] in ('thumb', 'index', 'middle', 'ring', 'pinky')}
                m = np.isin(dom, list(famille))
                if not m.any(): continue
                a, b = bouts[k], bouts[k + 1]
                t = ((P[m] - a) @ (b - a)) / max(float((b - a) @ (b - a)), 1e-12)
                cl[m, col] = k + np.clip(t, 0, 0.999)
    # WFS-211 (ANO-111) : tête et cou jamais habillés. Le col du haut se mesure depuis l'articulation du cou (neck_01) ; chez
    # MakeHuman, le menton et la bouche descendent sous « cou + 5 cm » et prenaient la couleur du haut : les sommets portés
    # surtout par neck_01 et Head reçoivent une hauteur au-dessus du col (le shader et pietons.cpp les voient en peau)
    tete = Wv[:, idx['neck_01']] + Wv[:, idx['Head']] > 0.5
    cl[tete, 1] = np.maximum(cl[tete, 1], pos[idx['neck_01']][1] + 0.1)
    if cheveux is not None: cl[cheveux, 3] = 2.0   # WFS-233 : cheveux courts peints (partie 2 : cheveux)
    return cl

# WFS-233 : cuir chevelu des enfants (les humains MakeHuman du dépôt n'ont pas de maillage de cheveux) : sommets portés surtout
# par l'os Head au-dessus d'une ligne qui monte de la nuque au front. Hauteur relative f entre la base du crâne (articulation
# Head) et le sommet : 0,40 sur les côtés (au-dessus des oreilles), 0,75 au front, −0,10 à la nuque ; le visage regarde vers +z
LIGNE_COTE, LIGNE_FRONT, LIGNE_NUQUE = 0.40, 0.75, -0.32   # nuque : avis expert-realisme-3d
VOLUME_CHEVEUX = 0.004   # sommets du cuir chevelu poussés de 4 mm le long de la normale (épaisseur des cheveux courts)
def cuir_chevelu(P, Wv, pos, idx):
    tete = Wv[:, idx['Head']] > 0.5
    base = pos[idx['Head']]
    haut = float(P[tete, 1].max())
    zav, zar = float(P[tete, 2].max()), float(P[tete, 2].min())
    t = np.where(P[:, 2] >= base[2], (P[:, 2] - base[2]) / max(zav - base[2], 1e-6), -(base[2] - P[:, 2]) / max(base[2] - zar, 1e-6))
    f = np.where(t >= 0, LIGNE_COTE + (LIGNE_FRONT - LIGNE_COTE) * t, LIGNE_COTE + (LIGNE_COTE - LIGNE_NUQUE) * t)
    return tete & ((P[:, 1] - base[1]) > f * (haut - base[1]))

# WFS-266 (RG-01) : coiffure longue (« Hair_Long » de femme.wfp, Quaternius CC0, déjà portée par les filles dans WFS-240) pour 4
# femmes sur 6 ; les 2 autres (f04, f06) et les 6 hommes : cheveux courts peints sur le cuir chevelu (WFS-233). Niveaux de détail
# comme WFS-240 : Hair_Long-1 (1 104 triangles) de près et à moyenne distance, Hair_Long-2 (377) au loin
CHEVEUX_LONGS = ('f01', 'f02', 'f03', 'f05')
COIFFURE_LOD = ('Hair_Long-1', 'Hair_Long-1', 'Hair_Long-2')
ECART_CRANE = 0.003   # une mèche n'entre jamais dans la tête : 3 mm au moins au-dessus de la peau (le long de sa normale)

def boite_tete(P, wtete):
    """boîte de la tête : sommets portés surtout par l'os Head (visage et crâne)"""
    m = wtete > 0.5
    return P[m].min(0), P[m].max(0)

def poser_coiffure(c, femme_corps, P, N, Wv, idx):
    """coiffure de femme.wfp mise sur la tête de l'humain : boîte de la tête de la femme du jeu → boîte de la tête de l'humain
    (échelle par axe, pose de repos commune : bras en T, visage vers +z), puis mèches sorties de la peau (ECART_CRANE)"""
    h = idx['Head']
    lo_f, hi_f = boite_tete(femme_corps['P'], (femme_corps['w'] * (femme_corps['os'] == h)).sum(1))
    lo_h, hi_h = boite_tete(P, Wv[:, h])
    s = (hi_h - lo_h) / (hi_f - lo_f)
    Q = lo_h + (c['P'] - lo_f) * s
    Nq = c['N'] / s; Nq /= np.maximum(np.linalg.norm(Nq, axis=1, keepdims=True), 1e-12)
    tete = np.where((Wv[:, h] + Wv[:, idx['neck_01']] > 0.3) | (P[:, 1] > lo_h[1] - 0.25))[0]   # tête, cou, épaules, haut du dos
    for i in range(len(Q)):
        d = ((P[tete] - Q[i]) ** 2).sum(1); j = tete[d.argmin()]
        e = float((Q[i] - P[j]) @ N[j])
        if e < ECART_CRANE: Q[i] += N[j] * (ECART_CRANE - e)
    return Q, Nq

def ecrire_wfp(chemin, r, mh):
    adulte = not est_enfant(r['nom'])
    sv, tri, P, N, UV, TG, oeil = maillage(r, mh, yeux=adulte)   # WFS-266 : yeux des adultes (enfants : inchangés)
    Wfull = r['W'][sv[:, 0]]
    # WFS-233 : cuir chevelu peint et légèrement épaissi ; WFS-266 (RG-01) : aussi chez les 12 adultes (même ligne des cheveux)
    cheveux = cuir_chevelu(P, Wfull, r['pos'], r['idx']) & ~oeil
    P = P + N * (VOLUME_CHEVEUX * cheveux[:, None])
    # 4 poids au plus, renormalisés
    top = np.argsort(-Wfull, 1)[:, :4]
    wt = np.take_along_axis(Wfull, top, 1); wt /= np.maximum(wt.sum(1, keepdims=True), 1e-9)
    utilises = sorted({int(o) for o, w in zip(top.ravel(), wt.ravel()) if w > 0})
    assert len(utilises) <= 64, len(utilises)
    vers_pal = {o: i for i, o in enumerate(utilises)}
    pal = np.vectorize(lambda o: vers_pal.get(int(o), 0))(top).astype(np.uint8)
    cl = attribut_cloth(r, P, Wfull, cheveux)
    if adulte:
        cl[cheveux, 0] = 0.0   # WFS-266 : cheveux d'un humain réaliste adulte (scene_skin.frag : couleur de la palette telle quelle)
        # WFS-266 (RG-02) : partie 1 (yeux) ; _cloth.xyz = centre du globe (pose de repos) : iris et pupille dessinés au pixel
        # par scene_skin.frag autour de l'axe du regard (+z)
        for cote in (P[:, 0] > 0, P[:, 0] <= 0):
            m = oeil & cote
            cl[m, 0:3] = P[m].mean(0); cl[m, 3] = 1.0
    O, Rq, pos, idx = r['O'], r['Rq'], r['pos'], r['idx']
    # repères du corps (comme les personnages du jeu) : cou = y de neck_01, taille = 80 % du bassin à spine_01, profondeur du cou
    cou, sp1, bas = pos[idx['neck_01']], pos[idx['spine_01']], pos[idx['pelvis']]
    corps = (float(cou[1]), float(bas[1] + 0.8 * (sp1[1] - bas[1])), float(cou[2]), 0.0)
    # textures : couleur de peau neutre unie (saturée : pas prise pour un sous-vêtement par pedMaterial), normales plates
    def image(c, fmt):
        f = io.BytesIO(); Image.new('RGB', (8, 8), c).save(f, fmt, **({'quality': 95} if fmt == 'JPEG' else {})); return f.getvalue()
    tex = [image((222, 178, 150), 'JPEG'), image((128, 128, 255), 'PNG')]
    out = bytearray(b'WFP1') + struct.pack('<I', 3) + struct.pack('<I', len(tex))
    for t in tex: out += struct.pack('<I', len(t)) + t
    out += struct.pack('<3f', 0.5, 0.0, float(r['stature'])) + struct.pack('<4f', *corps)
    # WFS-211 (RG-01) : 3 niveaux de détail (proportions des personnages du jeu : 100 %, 38 %, 13 % des triangles), mêmes
    # sommets ; verrouillés : ligne médiane (|x| < 8 mm : entrejambe, menton, M-02) et extrêmes (stature, envergure)
    verrou = np.abs(P[:, 0]) < 0.008
    for a in (P[:, 0].argmin(), P[:, 0].argmax(), P[:, 1].argmin(), P[:, 1].argmax()): verrou[a] = True
    niveaux = [tri] + [simplifier(P, tri, int(len(tri) * f), verrou, e) for f, e in zip(LOD_PART[1:], LOD_ECART[1:])]
    if r['nom'] in CHEVEUX_LONGS:   # WFS-266 (RG-01) : mèches ajoutées à chaque niveau (aucun appel de dessin de plus)
        femme_corps = mh['femme_corps']
        poses = {}
        for k, nom in enumerate(COIFFURE_LOD):
            if nom not in poses:
                c = mh['coiffures'][nom]
                Q, Nq = poser_coiffure(c, femme_corps, P, N, Wfull, idx)
                W = np.zeros((len(Q), Wfull.shape[1]))
                for j in range(4): np.add.at(W, (np.arange(len(Q)), c['os'][:, j]), c['w'][:, j])
                t = np.cross(Nq, [0.0, 1.0, 0.0]); t = np.where(np.linalg.norm(t, axis=1, keepdims=True) < 1e-6, [[1.0, 0.0, 0.0]], t)
                t /= np.linalg.norm(t, axis=1, keepdims=True)
                cq = np.stack([np.ones(len(Q)), Q[:, 1], -np.ones(len(Q)), np.full(len(Q), 2.0)], 1)   # partie 2, mèche (x = 1)
                poses[nom] = len(P)
                P = np.concatenate([P, Q]); N = np.concatenate([N, Nq]); UV = np.concatenate([UV, c['UV']])
                TG = np.concatenate([TG, np.concatenate([t, np.ones((len(Q), 1))], 1)]); Wfull = np.concatenate([Wfull, W])
                cl = np.concatenate([cl, cq])
            niveaux[k] = np.concatenate([niveaux[k], mh['coiffures'][nom]['tri'] + poses[nom]]).astype(np.uint32)
        top = np.argsort(-Wfull, 1)[:, :4]
        wt = np.take_along_axis(Wfull, top, 1); wt /= np.maximum(wt.sum(1, keepdims=True), 1e-9)
        utilises = sorted({int(o) for o, w in zip(top.ravel(), wt.ravel()) if w > 0})
        assert len(utilises) <= 64, len(utilises)
        vers_pal = {o: i for i, o in enumerate(utilises)}
        pal = np.vectorize(lambda o: vers_pal.get(int(o), 0))(top).astype(np.uint8)
    plages, debut = [], 0
    for k, t in enumerate(niveaux):
        plages.append(('corps-%d' % k, debut, len(t) * 3)); debut += len(t) * 3
    tri = np.concatenate(niveaux)
    clips = mise_a_echelle_clips(r['clips'], r['nC'], idx['pelvis'], r['k_racine'])
    out += struct.pack('<5I', len(P), len(tri) * 3, len(plages), len(O), r['nC'])
    v = np.zeros(len(P), dtype=[('p', '<f4', 3), ('n', '<f4', 3), ('uv', '<f4', 2), ('t', '<f4', 4), ('pal', 'u1', 4), ('w', '<f4', 4), ('cl', '<f4', 4)])
    v['p'], v['n'], v['uv'], v['t'], v['pal'], v['w'], v['cl'] = P, N, UV, TG, pal, wt, cl
    assert v.dtype.itemsize == 84
    out += v.tobytes() + tri.astype('<u4').tobytes()
    for n, a, c in plages: out += n.encode().ljust(32, b'\0') + struct.pack('<2I', a, c)
    for i, o in enumerate(O):
        par = o['parent']
        t = pos[i] if par < 0 else Rq[par].T @ (pos[i] - pos[par])
        M = np.eye(4); M[:3, :3] = Rq[i]; M[:3, 3] = pos[i]
        inv = np.linalg.inv(M)
        out += o['nom'].encode().ljust(32, b'\0') + struct.pack('<i', par) + struct.pack('<3f', *t) + struct.pack('<4f', *o['r']) \
               + struct.pack('<3f', 1, 1, 1) + struct.pack('<16f', *inv.T.ravel())   # colonne par colonne (comme glTF)
    out += struct.pack('<I', len(utilises)) + struct.pack('<%dI' % len(utilises), *utilises) + clips
    pathlib.Path(chemin).write_bytes(bytes(out))
    return dict(sommets=len(P), triangles=[len(t) for t in niveaux], octets=len(out), stature=r['stature'])

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('makehuman'); ap.add_argument('homme'); ap.add_argument('femme'); ap.add_argument('sortie')
    ap.add_argument('--seulement', default='')
    a = ap.parse_args()
    d = pathlib.Path(a.makehuman)
    V, VT, F, FY = lire_obj(d / 'base.obj')
    mh = dict(dossier=d, V=V, VT=VT, F=F, FY=FY, sk=json.load(open(d / 'default.mhskel')), poids=json.load(open(d / 'default_weights.mhw'))['weights'])
    jeu = {1: lire_wfp(a.homme), 0: lire_wfp(a.femme)}
    mh['femme_corps'] = lire_coiffure(a.femme, 'corps-0')   # WFS-266 : tête de la femme du jeu (repère des coiffures)
    mh['coiffures'] = {n: lire_coiffure(a.femme, n) for n in set(COIFFURE_LOD)}
    pathlib.Path(a.sortie).mkdir(parents=True, exist_ok=True)
    cache = {}
    choix = [s for s in a.seulement.split(',') if s]
    for m in MORPHOTYPES + ENFANTS:   # WFS-233 : enfants
        if choix and m[0] not in choix: continue
        r = convertir(mh, jeu, m, a.sortie, cache)
        s = ecrire_wfp(pathlib.Path(a.sortie) / (m[0] + '.wfp'), r, mh)
        print('%s : stature %.4f m, %d sommets, triangles %s, %.1f Ko' % (m[0], s['stature'], s['sommets'], s['triangles'], s['octets'] / 1024))

if __name__ == '__main__':
    main()
