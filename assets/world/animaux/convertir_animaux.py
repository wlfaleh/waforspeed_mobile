#!/usr/bin/env python3
# WaForSpeed — WFS-181 (RG-06 de WFS-139) : conversion des animaux de la Corniche en poses figées (.wfa).
#   mouette  : seagull.fbx de svartmc/seagull (licence MIT, assets/licences/animaux-corniche.txt), FBX binaire 7.5 à
#              squelette (12 os). Poses calculées ici par habillage linéaire (même calcul qu'un vertex shader de skinning) :
#              8 phases du battement d'ailes, 1 vol plané, 1 pose posée (ailes repliées le long du dos).
#   dauphin  : Dolphin.glb du « Animated Fish Pack » de Quaternius (CC0), pose de repos (corps droit).
# Usage : python3 -I convertir_animaux.py <seagull.fbx> <seagull_texture.jpg> <Dolphin.glb> <dossier de sortie>
#
# Format .wfa (petit-boutiste) :
#   'WFA1' u32 nPoses u32 nSommets (sommets non soudés, 3 par triangle)
#   couleurs : u8 × nSommets (code de teinte, mis en couleur par plage.cpp avec les albédos de la charte)
#   poses    : nPoses × nSommets × (pos3 normale3) f32 ; mètres, Y vers le haut, tête vers +Z, sol / ligne de flottaison en y = 0
#
# Mesures (RG-C4, DA-realisme.md : taille réelle de l'espèce) :
#   goéland leucophée (Larus michahellis, mouette de la Méditerranée) : longueur 52-68 cm, envergure 120-155 cm
#   (L. Svensson et al., « Le guide ornitho », Delachaux 2015) → envergure 1,40 m, longueur 0,62 m ;
#   dauphin commun (Delphinus delphis) : adulte 1,8-2,6 m (NOAA Fisheries, « Short-beaked common dolphin ») → 2,30 m.
import json, math, struct, sys, zlib

# ------------------------------------------------------------------ FBX binaire (lecteur minimal)
def lire_fbx(path):
    d = open(path, 'rb').read()
    if d[:20] != b'Kaydara FBX Binary  ': raise ValueError('pas un FBX binaire')
    big = struct.unpack_from('<I', d, 23)[0] >= 7500
    def prop(o):
        t = chr(d[o]); o += 1
        if t in 'YCILFD':
            f = {'Y': '<h', 'C': '<?', 'I': '<i', 'L': '<q', 'F': '<f', 'D': '<d'}[t]
            return struct.unpack_from(f, d, o)[0], o + struct.calcsize(f)
        if t in 'fdlib':
            n, enc, clen = struct.unpack_from('<III', d, o); o += 12
            raw = d[o:o + clen]; o += clen
            if enc == 1: raw = zlib.decompress(raw)
            f = {'f': 'f', 'd': 'd', 'l': 'q', 'i': 'i', 'b': '?'}[t]
            return list(struct.unpack('<%d%s' % (n, f), raw[:n * struct.calcsize(f)])), o
        if t in 'SR':
            n = struct.unpack_from('<I', d, o)[0]; o += 4
            return (d[o:o + n].decode('utf8', 'replace') if t == 'S' else d[o:o + n]), o + n
        raise ValueError('propriété FBX inconnue ' + t)
    def noeud(o):
        if big: end, npr, _ = struct.unpack_from('<QQQ', d, o); o += 24
        else: end, npr, _ = struct.unpack_from('<III', d, o); o += 12
        nl = d[o]; o += 1
        if end == 0: return None, o
        nom = d[o:o + nl].decode(); o += nl
        props = []
        for _ in range(npr): v, o = prop(o); props.append(v)
        kids = []
        while o < end:
            k, o2 = noeud(o)
            if k is None: break
            kids.append(k); o = o2
        return {'n': nom, 'p': props, 'k': kids}, end
    o, top = 27, []
    while o < len(d) - 13:
        k, o = noeud(o)
        if k is None: break
        top.append(k)
    return top
def fils(n, nom): return [k for k in n['k'] if k['n'] == nom]

# ------------------------------------------------------------------ petites matrices 4 × 4 (listes de lignes)
def mat_id(): return [[1.0 if i == j else 0.0 for j in range(4)] for i in range(4)]
def mat_mul(a, b): return [[sum(a[i][k] * b[k][j] for k in range(4)) for j in range(4)] for i in range(4)]
def mat_trans(x, y, z): m = mat_id(); m[0][3], m[1][3], m[2][3] = x, y, z; return m
def mat_rot(axe, deg):
    a = math.radians(deg); c, s = math.cos(a), math.sin(a); m = mat_id()
    i, j = {'x': (1, 2), 'y': (2, 0), 'z': (0, 1)}[axe]
    m[i][i], m[i][j], m[j][i], m[j][j] = c, -s, s, c
    return m
def mat_scale(k): m = mat_id(); m[0][0] = k; return m
# rotation locale d'un os : liste de (axe, degrés) appliqués dans l'ordre (le premier d'abord), ('s', k) : longueur × k
def mat_locale(rots):
    m = mat_id()
    for axe, v in rots: m = mat_mul(mat_scale(v) if axe == 's' else mat_rot(axe, v), m)
    return m
def appl(m, v, w=1.0): return [m[i][0] * v[0] + m[i][1] * v[1] + m[i][2] * v[2] + m[i][3] * w for i in range(3)]
def norm(v): l = math.sqrt(sum(c * c for c in v)) or 1.0; return [c / l for c in v]

# codes de teinte (plage.cpp : CODE_*)
BLANC, GRIS, NOIR, JAUNE, DOS, VENTRE = 0, 1, 2, 3, 4, 5

def mouette(fbx, tex):
    from PIL import Image   # lecture de la texture d'origine (3 cases : blanc, jaune, noir)
    img = Image.open(tex).convert('RGB')
    t = lire_fbx(fbx)
    obj = [k for k in t if k['n'] == 'Objects'][0]
    geo = fils(obj, 'Geometry')[0]
    V = fils(geo, 'Vertices')[0]['p'][0]; P = [V[i:i + 3] for i in range(0, len(V), 3)]
    PVI = fils(geo, 'PolygonVertexIndex')[0]['p'][0]
    luv = fils(geo, 'LayerElementUV')[0]
    UV = fils(luv, 'UV')[0]['p'][0]; UVI = fils(luv, 'UVIndex')[0]['p'][0]
    modeles, amas = {}, {}
    for k in obj['k']:
        if k['n'] == 'Model':
            pr = {p['p'][0]: p['p'][4:] for p in fils(fils(k, 'Properties70')[0], 'P')}
            modeles[k['p'][0]] = {'nom': k['p'][1].split('\x00')[0], 't': pr.get('Lcl Translation', [0, 0, 0])}
        if k['n'] == 'Deformer' and k['p'][2] == 'Cluster':
            ix = fils(k, 'Indexes'); w = fils(k, 'Weights'); tl = fils(k, 'TransformLink')[0]['p'][0]
            amas[k['p'][0]] = {'i': ix[0]['p'][0] if ix else [], 'w': w[0]['p'][0] if w else [], 'lien': tl}
    parent, os_amas = {}, {}
    for c in [k for k in t if k['n'] == 'Connections'][0]['k']:
        a, b = c['p'][1], c['p'][2]
        if a in modeles and b in modeles: parent[a] = b
        if a in modeles and b in amas: os_amas[a] = b
    noms = {m['nom']: i for i, m in modeles.items()}
    # poids par sommet de contrôle
    poids = [[] for _ in P]
    for osid, aid in os_amas.items():
        for i, w in zip(amas[aid]['i'], amas[aid]['w']): poids[i].append((osid, w))
    # repère de chaque os au repos : translation globale de TransformLink (rotations nulles dans ce fichier)
    def glob_repos(osid):
        tl = amas[os_amas[osid]]['lien']; return mat_trans(tl[12], tl[13], tl[14])
    def glob_pose(osid, rot):
        if osid not in os_amas: return mat_id()
        p = parent.get(osid)
        base = glob_pose(p, rot) if p in os_amas else mat_id()
        rep = glob_repos(osid)
        locale = mat_mul(mat_inv_trans(glob_repos(p)) if p in os_amas else mat_id(), rep)
        return mat_mul(mat_mul(base, locale), mat_locale(rot.get(modeles[osid]['nom'], [])))
    def mat_inv_trans(m): return mat_trans(-m[0][3], -m[1][3], -m[2][3])
    # triangles (polygones en éventail), coins : sommet de contrôle, uv
    coins, poly = [], []
    for k, v in enumerate(PVI):
        fin = v < 0; poly.append((v if v >= 0 else -v - 1, k))
        if fin:
            for j in range(1, len(poly) - 1): coins += [poly[0], poly[j], poly[j + 1]]
            poly = []
    # teinte : case de la texture d'origine (jaune : bec, noir : bout des ailes), dessus des ailes gris (manteau)
    codes = []
    for tri in range(0, len(coins), 3):
        u = sum(UV[UVI[c[1]] * 2] for c in coins[tri:tri + 3]) / 3; v = sum(UV[UVI[c[1]] * 2 + 1] for c in coins[tri:tri + 3]) / 3
        r, g, b = img.getpixel((min(img.width - 1, int(u * img.width)), min(img.height - 1, int((1 - v) * img.height))))
        code = JAUNE if r > 180 and b < 120 else NOIR if r < 90 else BLANC
        a, b2, c2 = (P[c[0]] for c in coins[tri:tri + 3])
        n = norm([(b2[1] - a[1]) * (c2[2] - a[2]) - (b2[2] - a[2]) * (c2[1] - a[1]), (b2[2] - a[2]) * (c2[0] - a[0]) - (b2[0] - a[0]) * (c2[2] - a[2]),
                  (b2[0] - a[0]) * (c2[1] - a[1]) - (b2[1] - a[1]) * (c2[0] - a[0])])
        cx = abs((a[0] + b2[0] + c2[0]) / 3); cz = (a[2] + b2[2] + c2[2]) / 3
        if code == NOIR and cx < 6: code = BLANC   # tête et queue blanches (goéland adulte) : noir au bout des ailes seulement
        if code == BLANC and n[1] > 0.3 and (cx > 1.3 or -3.5 < cz < 1.0): code = GRIS   # manteau gris : dessus des ailes et dos
        codes += [code] * 3
    # poses : battement (rotation des épaules et des mains autour de l'axe du corps), plané, posé
    L, Rr = 'WINGLEFT', 'WINGRIGHT'
    poses = []
    for k in range(8):   # 8 phases d'un battement : épaule ±32°, main en retard de 50° de phase
        ph = 2 * math.pi * k / 8
        ep, ma = 6 + 32 * math.cos(ph), 14 * math.cos(ph - 0.9)
        poses.append({L: [('z', ep)], Rr: [('z', -ep)], L + 'END': [('z', ma)], Rr + 'END': [('z', -ma)]})
    poses.append({L: [('z', 7)], Rr: [('z', -7)], L + 'END': [('z', -4)], Rr + 'END': [('z', 4)]})   # plané : léger dièdre
    # posé : aile tournée sur sa longueur (dessus gris vers l'extérieur, bord d'attaque en haut), ramenée vers l'arrière le
    # long du flanc ; rémiges de la main superposées (longueur × 0,6) : pointes croisées 5 cm au-delà de la queue
    poses.append({L: [('x', 75), ('y', 97), ('z', -4)], Rr: [('x', -75), ('y', -97), ('z', 4)],
                  L + 'END': [('s', 0.6), ('y', -4)], Rr + 'END': [('s', 0.6), ('y', 4)], 'HEAD': [('x', -10)]})
    # échelle : envergure 1,40 m (x), longueur 0,62 m (z) ; posé : ventre sur y = 0
    xs = [p[0] for p in P]; zs = [p[2] for p in P]
    kx = 1.40 / (max(xs) - min(xs)); kz = 0.62 / (max(zs) - min(zs)); cz = (max(zs) + min(zs)) / 2
    sorties = []
    for rot in poses:
        mats = {osid: mat_mul(glob_pose(osid, rot), mat_inv_trans(glob_repos(osid))) for osid in os_amas}
        pts = []
        for i, p in enumerate(P):
            q = [0.0, 0.0, 0.0]; tot = sum(w for _, w in poids[i]) or 1.0
            for osid, w in poids[i]:
                r = appl(mats[osid], p); q = [q[j] + r[j] * w / tot for j in range(3)]
            if not poids[i]: q = list(p)
            pts.append([q[0] * kx, q[1] * kx, (q[2] - cz) * kz])
        sorties.append(pts)
    ymin = min(p[1] for p in sorties[-1])
    poses_out = []
    for pts in sorties:
        som = []
        for tri in range(0, len(coins), 3):
            a, b, c = (pts[cc[0]] for cc in coins[tri:tri + 3])
            n = norm([(b[1] - a[1]) * (c[2] - a[2]) - (b[2] - a[2]) * (c[1] - a[1]), (b[2] - a[2]) * (c[0] - a[0]) - (b[0] - a[0]) * (c[2] - a[2]),
                      (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])])
            for q in (a, b, c): som.append((q[0], q[1] - ymin, q[2], n[0], n[1], n[2]))
        poses_out.append(som)
    return codes, poses_out

def dauphin(glb):
    d = open(glb, 'rb').read()
    lj = struct.unpack_from('<I', d, 12)[0]; j = json.loads(d[20:20 + lj])
    binaire = d[20 + lj + 8:]
    def acc(i):
        a = j['accessors'][i]; bv = j['bufferViews'][a['bufferView']]
        n = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}[a['type']]
        f = {5126: 'f', 5125: 'I', 5123: 'H', 5121: 'B'}[a['componentType']]
        o = bv.get('byteOffset', 0) + a.get('byteOffset', 0); st = bv.get('byteStride', 0) or n * struct.calcsize(f)
        return [list(struct.unpack_from('<%d%s' % (n, f), binaire, o + k * st)) for k in range(a['count'])]
    noeud = [n for n in j['nodes'] if n.get('mesh') == 0][0]
    q = noeud.get('rotation', [0, 0, 0, 1]); s = noeud.get('scale', [1, 1, 1])
    def rotq(v):   # rotation par le quaternion (x, y, z, w) du nœud, puis échelle
        x, y, z, w = q; v = [v[0] * s[0], v[1] * s[1], v[2] * s[2]]
        t = [2 * (y * v[2] - z * v[1]), 2 * (z * v[0] - x * v[2]), 2 * (x * v[1] - y * v[0])]
        return [v[0] + w * t[0] + y * t[2] - z * t[1], v[1] + w * t[1] + z * t[0] - x * t[2], v[2] + w * t[2] + x * t[1] - y * t[0]]
    tris, codes = [], []
    for p in j['meshes'][0]['primitives']:
        pos = [rotq(v) for v in acc(p['attributes']['POSITION'])]
        nrm = [norm(rotq([c / max(s) for c in v])) for v in acc(p['attributes']['NORMAL'])]
        idx = [i[0] for i in acc(p['indices'])]
        code = VENTRE if j['materials'][p['material']]['name'] == 'Bottom' else DOS
        for i in idx: tris.append(pos[i] + nrm[i]); codes.append(code)
    # le nœud du modèle met déjà l'axe long selon Z, museau vers +Z (vérifié sur l'aperçu) ; ligne de flottaison au milieu
    ext = [max(t[k] for t in tris) - min(t[k] for t in tris) for k in range(3)]
    if max(range(3), key=lambda k: ext[k]) != 2: raise ValueError('dauphin : axe long inattendu')
    zs = [t[2] for t in tris]; zmin, zmax = min(zs), max(zs)
    k = 2.30 / (zmax - zmin)
    cy = (max(t[1] for t in tris) + min(t[1] for t in tris)) / 2; cz = (zmax + zmin) / 2
    tris = [[t[0] * k, (t[1] - cy) * k, (t[2] - cz) * k, t[3], t[4], t[5]] for t in tris]
    return codes, [tris]

def ecrire(path, codes, poses):
    with open(path, 'wb') as f:
        f.write(b'WFA1'); f.write(struct.pack('<II', len(poses), len(codes)))
        f.write(bytes(codes))
        for som in poses:
            for s in som: f.write(struct.pack('<6f', *s))

if __name__ == '__main__':
    fbx, tex, glb, sortie = sys.argv[1:5]
    c, p = mouette(fbx, tex); ecrire(sortie + '/mouette.wfa', c, p)
    print('mouette : %d triangles, %d poses' % (len(c) // 3, len(p)))
    c, p = dauphin(glb); ecrire(sortie + '/dauphin.wfa', c, p)
    print('dauphin : %d triangles' % (len(c) // 3))
