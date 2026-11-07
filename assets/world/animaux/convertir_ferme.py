#!/usr/bin/env python3
# WaForSpeed — WFS-186 (RG-02, RG-C4 de WFS-138) : animaux de ferme de la Campagne en poses figées (.wfa, même format que
# convertir_chiens.py, dont on reprend le calcul : habillage linéaire, comme un vertex shader de skinning).
#   Cow.gltf, Horse.gltf : « Ultimate Animated Animal Pack » de Quaternius (CC0 1.0), dépôt public benjaminpjones/catch-the-animal
#     (models/glTF) ; licence lue dans LICENSES.md du dépôt public trebeljahr/quaternius-showcase (animals_pack : CC0-1.0).
#   Sheep.gltf + Sheep.bin : « Farm Animals Pack » de Quaternius (CC0 1.0), dépôt public gfxfundamentals/threejsfundamentals
#     (threejs/resources/models/animals) ; licence lue dans License.txt du pack (dépôt public Dewflash/isekai-leveling).
# Poses : 4 phases de l'animation « Eating » (tête qui broute) + 1 pose « Idle » ; le mouton n'a pas d'« Eating » : 4 phases
# de « Idle » + 1 pose de repos.
# Usage : python3 -I convertir_ferme.py <Cow.gltf> <Horse.gltf> <Sheep.gltf> <dossier de sortie>
#
# Mesures (RG-02 de WFS-186, DA-realisme.md : taille réelle de l'espèce) — hauteur au garrot :
#   vache : 1,40 m (vache limousine adulte 1,40-1,45 m, charolaise ≈ 1,40 m : standards de race du herd-book) ;
#   mouton : 0,70 m (brebis de race Île-de-France ou Texel, 0,65-0,75 m) ;
#   cheval : 1,60 m (selle français, 1,55-1,70 m : standard de race SHF).
# Le garrot est pris comme le haut du dos au milieu du corps (plus haut sommet du tiers central de la longueur, tête exclue),
# même mesure que convertir_chiens.py. Codes de teinte : indice du matériau d'origine (0 à 7), mis en couleur par campagne.cpp.
import base64, json, math, os, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from convertir_chiens import mul, slerp, trs, ecrire   # noqa: E402  (calcul commun avec les chiens de WFS-176)


def lire(path):
    """glTF à tampons intégrés (data:) ou externes (fichier .bin à côté), même lecture que convertir_chiens.lire."""
    d = json.load(open(path))
    bufs = []
    for b in d['buffers']:
        u = b['uri']
        bufs.append(base64.b64decode(u.split(',', 1)[1]) if u.startswith('data:') else open(os.path.join(os.path.dirname(path), u), 'rb').read())
    def acc(i):
        a = d['accessors'][i]; v = d['bufferViews'][a['bufferView']]; b = bufs[v['buffer']]
        n = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4, 'MAT4': 16}[a['type']]
        f = {5126: 'f', 5123: 'H', 5121: 'B', 5125: 'I'}[a['componentType']]
        st = v.get('byteStride') or n * struct.calcsize(f)
        o = v.get('byteOffset', 0) + a.get('byteOffset', 0)
        return [list(struct.unpack_from('<%d%s' % (n, f), b, o + k * st)) for k in range(a['count'])]
    return d, acc


def animal(path, garrot, anim_broute):
    d, acc = lire(path)
    nodes = d['nodes']
    parent = {}
    for i, n in enumerate(nodes):
        for c in n.get('children', []): parent[c] = i
    skin = d['skins'][0]; joints = skin['joints']
    ibm = [[[m[c * 4 + r] for c in range(4)] for r in range(4)] for m in acc(skin['inverseBindMatrices'])]
    anims = {a['name']: a for a in d['animations']}
    def echant(anim, t):
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
    geo = []
    for pr in d['meshes'][0]['primitives']:
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
            for k in I: tris.append(vs[k]); codes.append(min(code, 7))
        return codes, tris
    broute = anims[anim_broute]
    dur = max(x[0] for ch in broute['channels'] for x in acc(broute['samplers'][ch['sampler']]['input']))
    poses = []; codes = None
    for f in range(4):
        codes, t = pose(broute, dur * f / 4); poses.append(t)
    codes, t = pose(anims.get('Idle'), 0.0); poses.append(t)
    rest = poses[4]
    # axe long horizontal : x ou z du modèle ; tête vers +Z (côté le plus haut de la silhouette au repos)
    xs = [v[0] for v in rest]; zs = [v[2] for v in rest]
    if max(xs) - min(xs) > max(zs) - min(zs):
        poses = [[[-v[2], v[1], v[0], -v[5], v[4], v[3]] for v in p] for p in poses]   # rotation de 90° autour de y (x → z)
        rest = poses[4]
    zs = [v[2] for v in rest]; z0, z1 = min(zs), max(zs)
    haut_av = max(v[1] for v in rest if v[2] > z0 + 0.8 * (z1 - z0)); haut_ar = max(v[1] for v in rest if v[2] < z0 + 0.2 * (z1 - z0))
    if haut_ar > haut_av:   # tête vers −Z : demi-tour
        poses = [[[-v[0], v[1], -v[2], -v[3], v[4], -v[5]] for v in p] for p in poses]
        rest = poses[4]
    ys = [v[1] for v in rest]; zs = [v[2] for v in rest]; y0 = min(ys); z0, z1 = min(zs), max(zs)
    mil = [v[1] for v in rest if z0 + 0.35 * (z1 - z0) <= v[2] <= z0 + 0.65 * (z1 - z0)]
    k = garrot / (max(mil) - y0); cz = (z0 + z1) / 2; cx = (min(v[0] for v in rest) + max(v[0] for v in rest)) / 2
    poses = [[[(v[0] - cx) * k, (v[1] - y0) * k, (v[2] - cz) * k, v[3], v[4], v[5]] for v in p] for p in poses]
    return codes, poses, (z1 - z0) * k


if __name__ == '__main__':
    vache, cheval, mouton, sortie = sys.argv[1:5]
    for nom, src, g, an in (('vache', vache, 1.40, 'Eating'), ('cheval', cheval, 1.60, 'Eating'), ('mouton', mouton, 0.70, 'Idle')):
        c, p, L = animal(src, g, an); ecrire(sortie + '/' + nom + '.wfa', c, p)
        rest = p[4]; zs = [v[2] for v in rest]
        mil = [v[1] for v in rest if min(zs) + 0.35 * (max(zs) - min(zs)) <= v[2] <= min(zs) + 0.65 * (max(zs) - min(zs))]
        print('%s : %d triangles, %d poses, longueur %.2f m, garrot %.3f m, hauteur totale %.2f m, largeur %.2f m'
              % (nom, len(c) // 3, len(p), L, max(mil), max(v[1] for v in rest), max(v[0] for v in rest) - min(v[0] for v in rest)))
