#!/usr/bin/env python3
"""Test de tools/qa/cartes_json.py (WFS-302, RG-03) sur des paquets .wfcarte d'exemple écrits dans un dossier temporaire.

Cas : 3 formats bons → liste juste (adresses simples ou versionnées par le SHA-256) (ordre ASTC, ETC2, RGBA8 ; taille, SHA-256, adresse directe, empreinte en hexadécimal) et
stable octet par octet ; bloc abîmé (CRC faux), en-tête abîmé, format futur, nom de fichier qui ne dit pas l'en-tête, dossier
vide, adresse non https → code 2, aucune liste écrite.
Usage : python3 tools/qa/test_cartes_json.py   (code de sortie 0 si tout est vert)
"""
import hashlib
import json
import struct
import sys
import tempfile
import zlib
from pathlib import Path

ICI = Path(__file__).resolve().parent
sys.path.insert(0, str(ICI))
import cartes_json  # noqa: E402

BASE = "https://github.com/wlfaleh/waforspeed_mobile/releases/download/paquets"
echecs = 0


def verifier(ok, quoi):
    global echecs
    print(f"{'OK   ' if ok else 'ÉCHEC'} {quoi}")
    if not ok:
        echecs += 1


def paquet(carte="ville", fmt=0, version_format=1, empreinte=0x1F2E3D4C5B6A7980, blocs=None):
    """Même écriture que carte::ecrire (src/core/carte_cuite.cpp)."""
    blocs = blocs if blocs is not None else [("decor_regroupe", b""), ("maillages", b"WFSA\0\0\0\0"), ("textures", bytes(range(50)))]
    nom = carte.encode()
    taille_table = sum(1 + len(n.encode()) + 8 + 8 + 4 for n, _ in blocs)
    taille_entete = 12 + 2 + len(nom) + 4 + 1 + 8 + 4 + taille_table + 4
    e = bytearray(b"WFSC" + struct.pack("<II", version_format, taille_entete) + struct.pack("<H", len(nom)) + nom)
    e += struct.pack("<IBQI", 2, fmt, empreinte, len(blocs))
    pos = taille_entete
    for n, d in blocs:
        e += struct.pack("<B", len(n.encode())) + n.encode() + struct.pack("<QQI", pos, len(d), zlib.crc32(d) & 0xFFFFFFFF)
        pos += len(d)
    e += struct.pack("<I", zlib.crc32(bytes(e)) & 0xFFFFFFFF)
    return bytes(e) + b"".join(d for _, d in blocs)


def lancer(dossier, sortie, *extra):
    return cartes_json.main([str(dossier), str(sortie), "--adresse-base", BASE, "--version-min-apk", "927", "--nom", "ville=Ville", *extra])


with tempfile.TemporaryDirectory() as t:
    t = Path(t)
    bon = t / "bon"
    bon.mkdir()
    for i, f in enumerate(cartes_json.FORMATS):
        (bon / f"ville_{f}.wfcarte").write_bytes(paquet(fmt=i))
    (bon / "ville_astc.wfcarte.part").write_bytes(b"pas fini")   # jamais listé
    verifier(lancer(bon, t / "a.json", "--commit", "abc") == 0, "3 formats bons → code 0")
    liste = json.loads((t / "a.json").read_text(encoding="utf-8"))
    verifier(liste["version"] == 1 and liste["version_liste"] == 1 and liste["carte_par_defaut"] == "ville" and liste["version_min_apk"] == 927,
             "en-tête de la liste (version, version_liste, carte_par_defaut, version_min_apk)")
    verifier([p["format"] for p in liste["paquets"]] == ["astc", "etc2", "rgba8"], "ordre ASTC, ETC2, RGBA8 ; .part ignoré")
    p = liste["paquets"][0]
    octets = (bon / "ville_astc.wfcarte").read_bytes()
    verifier(p["taille"] == len(octets) and p["sha256"] == hashlib.sha256(octets).hexdigest(), "taille et SHA-256 du fichier")
    verifier(p["adresse"] == BASE + "/ville_astc.wfcarte", "adresse directe du fichier (pas l'API)")
    verifier(p["empreinte"] == "1f2e3d4c5b6a7980" and p["nom"] == "Ville" and p["version_carte"] == 2 and p["version_format"] == 1,
             "empreinte en hexadécimal (16 chiffres), nom, versions")
    verifier(lancer(bon, t / "b.json", "--commit", "abc") == 0 and (t / "a.json").read_bytes() == (t / "b.json").read_bytes(),
             "liste stable octet par octet")
    verifier(lancer(bon, t / "v.json", "--versionne", "--publies", str(t / "publies.txt")) == 0, "liste versionnée → code 0")
    v = json.loads((t / "v.json").read_text(encoding="utf-8"))["paquets"][0]
    publies = (t / "publies.txt").read_text(encoding="utf-8").split("\n")[0].split()
    attendu = "ville_astc-" + hashlib.sha256(octets).hexdigest()[:12] + ".wfcarte"
    verifier(v["adresse"] == BASE + "/" + attendu and publies == ["ville_astc.wfcarte", attendu],
             "nom publié versionné par le SHA-256 (adresse nouvelle pour un contenu nouveau) et liste des noms publiés")

    def refuse(nom, octets, quoi):
        d = t / nom
        d.mkdir()
        (d / f"ville_astc.wfcarte").write_bytes(octets)
        verifier(lancer(d, d / "cartes.json") == 2 and not (d / "cartes.json").exists(), quoi)

    abime = bytearray(paquet())
    abime[-3] ^= 0x5A
    refuse("bloc", bytes(abime), "bloc abîmé (CRC faux) → code 2")
    abime = bytearray(paquet())
    abime[20] ^= 0x01
    refuse("entete", bytes(abime), "en-tête abîmé → code 2")
    refuse("futur", paquet(version_format=2), "format futur → code 2")
    refuse("autre", paquet(fmt=2), "fichier « astc » dont l'en-tête dit RGBA8 → code 2")
    refuse("tronque", paquet()[:40], "fichier tronqué → code 2")
    vide = t / "vide"
    vide.mkdir()
    verifier(lancer(vide, vide / "cartes.json") == 2, "dossier vide → code 2")
    verifier(cartes_json.main([str(bon), str(t / "c.json"), "--adresse-base", "http://exemple", "--version-min-apk", "1"]) == 2,
             "adresse non https → code 2")

print(f"{'tout est vert' if echecs == 0 else f'{echecs} échec(s)'}")
sys.exit(1 if echecs else 0)
