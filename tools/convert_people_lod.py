# WaForSpeed — WFS-211 (RG-01) : niveaux de détail des humains réalistes (utilisé par convert_people_makehuman.py).
#
# Simplification par fusion d'arêtes vers un sommet EXISTANT (« half-edge collapse ») guidée par l'erreur quadrique de
# Garland et Heckbert (1997) : chaque niveau de détail n'est qu'une nouvelle liste de triangles sur les sommets du niveau 0.
# Les sommets gardés sont ceux d'origine : poids, palette d'os, _cloth et normales ne bougent pas (aucun saut de teinte ni de
# peau entre deux niveaux), et le .wfp ne grossit que des indices.
#   - les sommets sont soudés par position (coutures d'uv : la texture des humains est unie, les normales sont lissées par position) ;
#   - verrouillés (jamais retirés) : bords du maillage (yeux, bouche), sommets demandés par l'appelant (ligne médiane et
#     extrêmes : M-01 et M-02 se mesurent dessus) ;
#   - une fusion qui retourne un triangle ou en crée un dégénéré est refusée, ainsi que celle qui éloigne un sommet d'origine de
#     plus de ecart_max de la surface simplifiée (écart de forme borné : aucun saut visible au seuil de distance).
import heapq
import numpy as np


def _quadriques(P, tri):
    a, b, c = P[tri[:, 0]], P[tri[:, 1]], P[tri[:, 2]]
    n = np.cross(b - a, c - a)
    aire = np.linalg.norm(n, axis=1)
    n = n / np.maximum(aire, 1e-20)[:, None]
    d = -(n * a).sum(1)
    pl = np.concatenate([n, d[:, None]], 1)                 # plan (a, b, c, d)
    K = pl[:, :, None] * pl[:, None, :] * aire[:, None, None]   # pondéré par l'aire (×2)
    Q = np.zeros((len(P), 4, 4))
    for k in range(3): np.add.at(Q, tri[:, k], K)
    return Q


def _dist_tris(Q, A, B, C):
    """distance minimale de chaque point de Q (m, 3) aux triangles (A, B, C) (k, 3) : (m,) ; Ericson § 5.1.5, vectorisé"""
    p = Q[:, None, :]; a, b, c = A[None], B[None], C[None]
    ab, ac, ap = b - a, c - a, p - a
    d1, d2 = (ab * ap).sum(-1), (ac * ap).sum(-1)
    bp = p - b; d3, d4 = (ab * bp).sum(-1), (ac * bp).sum(-1)
    cp = p - c; d5, d6 = (ab * cp).sum(-1), (ac * cp).sum(-1)
    va, vb, vc = d3 * d6 - d5 * d4, d5 * d2 - d1 * d6, d1 * d4 - d3 * d2
    den = np.where(np.abs(va + vb + vc) < 1e-30, 1e-30, va + vb + vc)
    q = a + ab * (vb / den)[..., None] + ac * (vc / den)[..., None]   # intérieur
    def choisir(m, val):
        nonlocal q; q = np.where(m[..., None], val, q)
    sb = np.where(np.abs((d4 - d3) + (d5 - d6)) < 1e-30, 1e-30, (d4 - d3) + (d5 - d6))
    choisir((va <= 0) & (d4 - d3 >= 0) & (d5 - d6 >= 0), b + (c - b) * ((d4 - d3) / sb)[..., None])
    s2 = np.where(np.abs(d2 - d6) < 1e-30, 1e-30, d2 - d6)
    choisir((vb <= 0) & (d2 >= 0) & (d6 <= 0), a + ac * (d2 / s2)[..., None])
    s1 = np.where(np.abs(d1 - d3) < 1e-30, 1e-30, d1 - d3)
    choisir((vc <= 0) & (d1 >= 0) & (d3 <= 0), a + ab * (d1 / s1)[..., None])
    choisir((d6 >= 0) & (d5 <= d6), np.broadcast_to(c, q.shape))
    choisir((d3 >= 0) & (d4 <= d3), np.broadcast_to(b, q.shape))
    choisir((d1 <= 0) & (d2 <= 0), np.broadcast_to(a, q.shape))
    return np.linalg.norm(p - q, axis=-1).min(1)


def simplifier(P, tri, cible, verrou=None, ecart_max=None):
    """triangles (indices dans P) réduits à `cible` triangles au plus ; P : positions (n, 3) ; verrou : booléens (n,) ;
    ecart_max (m) : une fusion est refusée si un sommet d'origine absorbé par les deux sommets de l'arête s'écarte de plus
    de ecart_max des triangles qui entourent le sommet gardé (borne de l'écart de forme, WFS-211 RG-01)"""
    P = np.asarray(P, dtype=np.float64); tri = np.asarray(tri, dtype=np.int64)
    # soudure par position (au dixième de millimètre)
    cle = np.round(P / 1e-4).astype(np.int64)
    _, rep, inv = np.unique(cle, axis=0, return_index=True, return_inverse=True)
    inv = inv.ravel()
    Pu = P[rep]
    T = inv[tri]
    T = T[(T[:, 0] != T[:, 1]) & (T[:, 1] != T[:, 2]) & (T[:, 0] != T[:, 2])]
    nU = len(Pu)
    bloque = np.zeros(nU, bool)
    if verrou is not None: bloque[inv[np.nonzero(verrou)[0]]] = True
    # bords : arêtes portées par un seul triangle
    ar = np.sort(np.concatenate([T[:, [0, 1]], T[:, [1, 2]], T[:, [2, 0]]]), 1)
    u, cpt = np.unique(ar, axis=0, return_counts=True)
    bloque[u[cpt == 1].ravel()] = True
    Q = _quadriques(Pu, T)
    faces = [set() for _ in range(nU)]
    for f, t in enumerate(T):
        for v in t: faces[v].add(f)
    vivant = np.ones(len(T), bool); nVivant = len(T)
    version = np.zeros(nU, np.int64)
    Ph = np.concatenate([Pu, np.ones((nU, 1))], 1)
    absorbes = [[i] for i in range(nU)]   # sommets d'origine représentés par chaque sommet gardé

    def cout(a, b):   # a fusionné sur b (b reste)
        q = Q[a] + Q[b]; x = Ph[b]
        return float(x @ q @ x)

    tas = []
    def pousser(a, b):
        if bloque[a]: return
        heapq.heappush(tas, (cout(a, b), a, b, version[a], version[b]))
    for a, b in u:
        pousser(a, b); pousser(b, a)

    def accepte(a, b):
        for f in faces[a]:
            t = T[f]
            if b in t: continue   # disparaît
            p = [Pu[v] for v in t]
            n0 = np.cross(p[1] - p[0], p[2] - p[0])
            q = [Pu[b] if v == a else Pu[v] for v in t]
            n1 = np.cross(q[1] - q[0], q[2] - q[0])
            l1 = np.linalg.norm(n1)
            if l1 < 1e-12 or n0 @ n1 <= 0.2 * np.linalg.norm(n0) * l1: return False
        # liaison : les voisins communs de a et b ne doivent être que les sommets opposés aux triangles de l'arête (variété)
        if ecart_max is not None:   # forme : les sommets absorbés par a et b restent près de l'éventail de b après fusion
            fan = [[b if v == a else v for v in T[f]] for f in (faces[a] | faces[b]) if b not in T[f] or a not in T[f]]
            if fan:
                F = np.array(fan)
                pts = Pu[absorbes[a] + absorbes[b]]
                if _dist_tris(pts, Pu[F[:, 0]], Pu[F[:, 1]], Pu[F[:, 2]]).max() > ecart_max: return False
        va = {v for f in faces[a] for v in T[f]} - {a}
        vb = {v for f in faces[b] for v in T[f]} - {b}
        opp = {v for f in faces[a] & faces[b] for v in T[f]} - {a, b}
        return (va & vb) <= opp

    while nVivant > cible and tas:
        c, a, b, ga, gb = heapq.heappop(tas)
        if bloque[a] or ga != version[a] or gb != version[b]: continue
        communs = faces[a] & faces[b]
        if not communs or not accepte(a, b): continue
        for f in communs:
            vivant[f] = False; nVivant -= 1
            for v in T[f]: faces[v].discard(f)
        for f in list(faces[a]):
            T[f][T[f] == a] = b
            faces[b].add(f)
        faces[a] = set()
        absorbes[b] += absorbes[a]; absorbes[a] = []
        Q[b] = Q[b] + Q[a]
        version[a] += 1; version[b] += 1
        voisins = {v for f in faces[b] for v in T[f]} - {b}
        for v in voisins:
            version[v] += 1
        for v in voisins:
            pousser(b, v); pousser(v, b)
            for f in faces[v]:
                for w in T[f]:
                    if w != v: pousser(v, w)
    T = T[vivant]
    return rep[T].astype(np.uint32)   # retour aux sommets d'origine (représentant de chaque position)
