#!/usr/bin/env python3
# WaForSpeed — WFS-176 (RG-05 de WFS-137) : chiens de la Ville en poses figées (.wfa, même format que convertir_animaux.py).
#   ShibaInu.gltf et Husky.gltf de l'« Ultimate Animated Animal Pack » de Quaternius (CC0, quaternius.com), copiés du dépôt
#   public benjaminpjones/catch-the-animal (models/glTF). Poses calculées ici par habillage linéaire (même calcul qu'un
#   vertex shader de skinning) : 8 phases de l'animation « Walk », plus 1 pose « Idle » (assis / arrêté).
# Usage : python3 -I convertir_chiens.py <ShibaInu.gltf> <Husky.gltf> <dossier de sortie>
#
# Mesures (RG-C4, DA-realisme.md : taille réelle de la race) — hauteur au garrot :
#   shiba inu : mâle 39,5 cm, femelle 36,5 cm (± 1,5 cm) — standard FCI n° 257 → 0,39 m ;
#   husky de Sibérie : mâle 53,5-60 cm, femelle 50,5-56 cm — standard FCI n° 270 → 0,56 m.
# Le garrot est pris comme le haut du dos au milieu du corps (plus haut sommet du tiers central de la longueur, tête exclue).
# Codes de teinte : indice du matériau d'origine (0 à 5), mis en couleur par ville.cpp avec ses albédos.
import base64, json, math, struct, sys

def lire(path):
    d = json.load(open(path))
    bufs = [base64.b64decode(b['uri'].split(',', 1)[1]) for b in d['buffers']]
    def acc(i):
        a = d['accessors'][i]; v = d['bufferViews'][a['bufferView']]; b = bufs[v['buffer']]
        n = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4, 'MAT4': 16}[a['type']]
        f = {5126: 'f', 5123: 'H', 5121: 'B', 5125: 'I'}[a['componentType']]
        st = v.get('byteStride') or n * struct.calcsize(f)
        o = v.get('byteOffset', 0) + a.get('byteOffset', 0)
        return [list(struct.unpack_from('<%d%s' % (n, f), b, o + k * st)) for k in range(a['count'])]
    return d, acc

def qmat(q):
    x, y, z, w = q
    return [[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w), 0],
            [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w), 0],
            [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y), 0], [0, 0, 0, 1]]
def mul(a, b): return [[sum(a[i][k] * b[k][j] for k in range(4)) for j in range(4)] for i in range(4)]
def trs(t, r, s):
    m = qmat(r)
    for i in range(3):
        for j in range(3): m[i][j] *= s[j]
        m[i][3] = t[i]
    return m
def slerp(a, b, u):
    d = sum(x * y for x, y in zip(a, b))
    if d < 0: b = [-x for x in b]; d = -d
    if d > 0.9995: r = [x + (y - x) * u for x, y in zip(a, b)]
    else:
        th = math.acos(d); s = math.sin(th)
        r = [(math.sin((1 - u) * th) * x + math.sin(u * th) * y) / s for x, y in zip(a, b)]
    n = math.sqrt(sum(x * x for x in r)); return [x / n for x in r]

def chien(path, garrot):
    d, acc = lire(path)
    nodes = d['nodes']
    parent = {}
    for i, n in enumerate(nodes):
        for c in n.get('children', []): parent[c] = i
    skin = d['skins'][0]; joints = skin['joints']; ibm = [[[m[c * 4 + r] for c in range(4)] for r in range(4)] for m in acc(skin['inverseBindMatrices'])]
    anims = {a['name']: a for a in d['animations']}
    def echant(anim, t):   # TRS locaux de chaque nœud à l'instant t (interpolation linéaire / sphérique)
        loc = [[list(n.get('translation', [0, 0, 0])), list(n.get('rotation', [0, 0, 0, 1])), list(n.get('scale', [1, 1, 1]))] for n in nodes]
        if anim is None: return loc
        for ch in anim['channels']:
            sm = anim['samplers'][ch['sampler']]; ts = [x[0] for x in acc(sm['input'])]; vs = acc(sm['output'])
            tt = t % ts[-1] if ts[-1] > 0 else 0
            k = 0
            while k + 1 < len(ts) and ts[k + 1] < tt: k += 1
            k2 = min(k + 1, len(ts) - 1); u = 0 if k2 == k else (tt - ts[k]) / (ts[k2] - ts[k])
            p = ch['target']['path']; i = ch['target']['node']
            if p == 'rotation': loc[i][1] = slerp(vs[k], vs[k2], u)
            elif p in ('translation', 'scale'): loc[i][0 if p == 'translation' else 2] = [a + (b - a) * u for a, b in zip(vs[k], vs[k2])]
        return loc
    def glob(loc):
        g = {}
        def G(i):
            if i in g: return g[i]
            m = trs(*loc[i]); g[i] = mul(G(parent[i]), m) if i in parent else m; return g[i]
        for i in range(len(nodes)): G(i)
        return g
    prims = d['meshes'][0]['primitives']
    geo = []   # (positions, normales, joints, poids, indices, code)
    for pr in prims:
        at = pr['attributes']
        geo.append((acc(at['POSITION']), acc(at['NORMAL']), acc(at['JOINTS_0']), acc(at['WEIGHTS_0']), [x[0] for x in acc(pr['indices'])], pr.get('material', 0)))
    def pose(anim, t):
        g = glob(echant(anim, t)); J = [mul(g[j], ibm[k]) for k, j in enumerate(joints)]
        tris, codes = [], []
        for P, N, JJ, W, I, code in geo:
            vs = []
            for k in range(len(P)):
                M = [[sum(W[k][q] * J[JJ[k][q]][r][c] for q in range(4)) for c in range(4)] for r in range(3)]
                p = [sum(M[r][c] * (P[k] + [1])[c] for c in range(4)) for r in range(3)]
                n = [sum(M[r][c] * N[k][c] for c in range(3)) for r in range(3)]
                l = math.sqrt(sum(x * x for x in n)) or 1; vs.append(p + [x / l for x in n])
            for k in I: tris.append(vs[k]); codes.append(min(code, 5))
        return codes, tris
    walk = anims['Walk']; dur = max(x[0] for ch in walk['channels'] for x in acc(walk['samplers'][ch['sampler']]['input']))
    poses = []; codes = None
    for f in range(8):
        codes, t = pose(walk, dur * f / 8); poses.append(t)
    codes, t = pose(anims.get('Idle'), 0.0); poses.append(t)
    # repère : sol en y = 0, centre en x = z = 0, tête vers +Z (axe long : z du modèle) ; mise à l'échelle au garrot
    rest = poses[8]
    ys = [v[1] for v in rest]; zs = [v[2] for v in rest]; y0 = min(ys); z0, z1 = min(zs), max(zs)
    mil = [v[1] for v in rest if z0 + 0.35 * (z1 - z0) <= v[2] <= z0 + 0.65 * (z1 - z0)]
    k = garrot / (max(mil) - y0); cz = (z0 + z1) / 2
    poses = [[[v[0] * k, (v[1] - y0) * k, (v[2] - cz) * k, v[3], v[4], v[5]] for v in p] for p in poses]
    long = (z1 - z0) * k
    return codes, poses, long

def ecrire(path, codes, poses):
    with open(path, 'wb') as f:
        f.write(b'WFA1'); f.write(struct.pack('<II', len(poses), len(codes))); f.write(bytes(codes))
        for p in poses:
            for s in p: f.write(struct.pack('<6f', *s))

if __name__ == '__main__':
    shiba, husky, sortie = sys.argv[1:4]
    for nom, src, g in (('shiba', shiba, 0.39), ('husky', husky, 0.56)):
        c, p, L = chien(src, g); ecrire(sortie + '/' + nom + '.wfa', c, p)
        print('%s : %d triangles, %d poses, longueur %.2f m, hauteur totale %.2f m' % (nom, len(c) // 3, len(p), L, max(v[1] for v in p[8])))
