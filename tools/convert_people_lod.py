# WaForSpeed — WFS-211 (RG-01) : niveaux de détail des humains réalistes (utilisé par convert_people_makehuman.py).
#
# Simplification par fusion d'arêtes vers un sommet EXISTANT (« half-edge collapse ») guidée par l'erreur quadrique de
# Garland et Heckbert (1997) : chaque niveau de détail n'est qu'une nouvelle liste de triangles sur les sommets du niveau 0.
# Les sommets gardés sont ceux d'origine : poids, palette d'os, _cloth et normales ne bougent pas (aucun saut de teinte ni de
# peau entre deux niveaux), et le .wfp ne grossit que des indices.
#   - les sommets sont soudés par position (coutures d'uv : la texture des humains est unie, les normales sont lissées par position) ;
#   - verrouillés (jamais retirés) : bords du maillage (yeux, bouche), sommets demandés par l'appelant (ligne médiane et
#     extrêmes : M-01 et M-02 se mesurent dessus) ;
#   - une fusion qui retourne un triangle ou en crée un dégénéré est refusée.
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


def simplifier(P, tri, cible, verrou=None):
    """triangles (indices dans P) réduits à `cible` triangles au plus ; P : positions (n, 3) ; verrou : booléens (n,)"""
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
