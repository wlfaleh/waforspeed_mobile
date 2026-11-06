# WaForSpeed — WFS-149 (RG-01, RG-02) : écriture et lecture du conteneur KTX2 (spécification Khronos « KTX File Format
# Specification » version 2.0) pour les formats du moteur : ASTC 4x4 / 6x6, ETC2 RGBA, RGBA8 ; sRGB ou linéaire.
# Le moteur lit ces fichiers avec src/render/ktx2.cpp (mêmes règles : 2D, sans supercompression, chaîne complète).
import struct

IDENTIFIANT = bytes([0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A])

# vkFormat (vulkan_core.h) → (largeur du bloc, hauteur du bloc, octets par bloc, sRGB, modèle de couleur du DFD)
MODELE_RGBSDA, MODELE_ETC2, MODELE_ASTC = 1, 161, 162   # Khronos Data Format Specification 1.3, § 5.6
FORMATS = {
    37: (1, 1, 4, False, MODELE_RGBSDA),   # VK_FORMAT_R8G8B8A8_UNORM
    43: (1, 1, 4, True, MODELE_RGBSDA),    # VK_FORMAT_R8G8B8A8_SRGB
    151: (4, 4, 16, False, MODELE_ETC2),   # VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK
    152: (4, 4, 16, True, MODELE_ETC2),    # VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK
    157: (4, 4, 16, False, MODELE_ASTC),   # VK_FORMAT_ASTC_4x4_UNORM_BLOCK
    158: (4, 4, 16, True, MODELE_ASTC),    # VK_FORMAT_ASTC_4x4_SRGB_BLOCK
    165: (6, 6, 16, False, MODELE_ASTC),   # VK_FORMAT_ASTC_6x6_UNORM_BLOCK
    166: (6, 6, 16, True, MODELE_ASTC),    # VK_FORMAT_ASTC_6x6_SRGB_BLOCK
}

def vk_format(famille, srgb, bloc=None):
    """famille 'astc' (bloc '4x4' ou '6x6'), 'etc2' ou 'rgba8' → vkFormat"""
    if famille == 'rgba8': return 43 if srgb else 37
    if famille == 'etc2': return 152 if srgb else 151
    if famille == 'astc': return {('4x4', False): 157, ('4x4', True): 158, ('6x6', False): 165, ('6x6', True): 166}[(bloc, srgb)]
    raise ValueError(famille)

def taille_niveau(fmt, w, h):
    bw, bh, ob, _, _ = FORMATS[fmt]
    return ((w + bw - 1) // bw) * ((h + bh - 1) // bh) * ob

def chaine(w, h):
    """dimensions de tous les niveaux, jusqu'à 1×1"""
    out = [(w, h)]
    while w > 1 or h > 1:
        w, h = max(1, w // 2), max(1, h // 2); out.append((w, h))
    return out

def _dfd(fmt):
    """descripteur de format de base (Khronos Data Format Specification 1.3, § 5)"""
    bw, bh, ob, srgb, modele = FORMATS[fmt]
    LINEAIRE = 0x10   # qualificatif de l'échantillon (bit « linear ») : alpha toujours linéaire
    if modele == MODELE_RGBSDA:
        ech = [(0, 8, 0), (8, 8, 1), (16, 8, 2), (24, 8, 15 | (LINEAIRE if srgb else 0))]
        bas, haut = 0, 255
    elif modele == MODELE_ETC2:
        ech = [(0, 64, 15 | (LINEAIRE if srgb else 0)), (64, 64, 2)]   # alpha EAC puis couleur ETC2
        bas, haut = 0, 0xFFFFFFFF
    else:
        ech = [(0, 128, 0)]
        bas, haut = 0, 0xFFFFFFFF
    taille_bloc = 24 + 16 * len(ech)
    mots = [0, 2 | (taille_bloc << 16),   # vendorId 0, descriptorType 0 ; versionNumber 2, descriptorBlockSize
            modele | (1 << 8) | ((2 if srgb else 1) << 16) | (0 << 24),   # modèle, primaires BT.709, transfert sRGB / linéaire, alpha droit
            (bw - 1) | ((bh - 1) << 8),   # dimensions du bloc de texels (moins 1)
            ob, 0]                         # octets du plan 0
    for (decalage, longueur, canal) in ech:
        mots += [decalage | ((longueur - 1) << 16) | (canal << 24), 0, bas, haut]
    corps = struct.pack('<%dI' % len(mots), *mots)
    return struct.pack('<I', 4 + len(corps)) + corps

def ecrire(chemin, fmt, w, h, niveaux):
    """niveaux = liste d'octets, niveau 0 (pleine taille) d'abord ; écrits du plus petit au plus grand (spécification § 3.9)"""
    dims = chaine(w, h)
    assert len(niveaux) == len(dims), 'chaîne de mipmaps incomplète'
    for (lw, lh), d in zip(dims, niveaux):
        assert len(d) == taille_niveau(fmt, lw, lh), (lw, lh, len(d))
    bw, bh, ob, _, _ = FORMATS[fmt]
    align = 16 if ob == 16 else 4   # ppcm(taille du bloc de texels, 4)
    dfd = _dfd(fmt)
    cle, val = b'KTXwriter\0', b'WaForSpeed tools/textures/convertir.py\0'
    kv = struct.pack('<I', len(cle) + len(val)) + cle + val
    kv += b'\0' * ((-len(kv)) % 4)
    n = len(niveaux)
    ofs_dfd = 80 + 24 * n
    ofs_kvd = ofs_dfd + len(dfd)
    fin = ofs_kvd + len(kv)
    pos, index = fin, [None] * n
    corps = b''
    for i in reversed(range(n)):
        pad = (-pos) % align
        corps += b'\0' * pad; pos += pad
        index[i] = (pos, len(niveaux[i]))
        corps += niveaux[i]; pos += len(niveaux[i])
    tete = IDENTIFIANT + struct.pack('<9I', fmt, 1, w, h, 0, 0, 1, n, 0)
    tete += struct.pack('<4I2Q', ofs_dfd, len(dfd), ofs_kvd, len(kv), 0, 0)
    for (o, l) in index: tete += struct.pack('<3Q', o, l, l)
    with open(chemin, 'wb') as f:
        f.write(tete + dfd + kv + corps)

def lire(chemin):
    """→ (vkFormat, w, h, [octets des niveaux, niveau 0 d'abord]) ; lève ValueError si invalide"""
    b = open(chemin, 'rb').read()
    if b[:12] != IDENTIFIANT: raise ValueError('pas un KTX2')
    fmt, ts, w, h, d, couches, faces, n, sc = struct.unpack_from('<9I', b, 12)
    if fmt not in FORMATS or ts != 1 or d or couches or faces != 1 or sc: raise ValueError('KTX2 non pris en charge')
    dims = chaine(w, h)
    if n != len(dims): raise ValueError('chaîne incomplète')
    niv = []
    for i in range(n):
        o, l, _ = struct.unpack_from('<3Q', b, 80 + 24 * i)
        if l != taille_niveau(fmt, *dims[i]) or o + l > len(b): raise ValueError('niveau %d invalide' % i)
        niv.append(b[o:o + l])
    return fmt, w, h, niv
