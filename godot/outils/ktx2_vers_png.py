# WaForSpeed — prototype Godot (WFS-610) : textures de route du jeu actuel (.rgba8.ktx2, CC0 Poly Haven) converties en PNG
# (niveau 0) pour Godot, sans toucher aux fichiers d'origine.
#   python3 godot/outils/ktx2_vers_png.py entree.rgba8.ktx2 sortie.png
import struct, sys
from PIL import Image
d = open(sys.argv[1], 'rb').read()
assert d[:12] == b'\xabKTX 20\xbb\r\n\x1a\n', 'pas un KTX2'
fmt, taille_type, w, h = struct.unpack_from('<IIII', d, 12)
niveaux = struct.unpack_from('<I', d, 40)[0]
debut, longueur, _ = struct.unpack_from('<QQQ', d, 80)   # niveau 0 (le plus grand)
assert longueur == w * h * 4, f'format inattendu {fmt} ({longueur} octets pour {w}x{h})'
Image.frombytes('RGBA', (w, h), d[debut:debut + longueur]).convert('RGB').save(sys.argv[2])
print(f'{sys.argv[1]} → {sys.argv[2]} ({w}x{h}, vkFormat {fmt}, {niveaux} niveaux)')
