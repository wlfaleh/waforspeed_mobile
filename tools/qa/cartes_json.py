#!/usr/bin/env python3
"""Liste des paquets de cartes « cartes.json » (WFS-302, RG-03), écrite par la CI à partir des fichiers cuits (WFS-301).

Lit l'en-tête de chaque fichier <carte>_<astc|etc2|rgba8>.wfcarte du dossier donné (format : src/core/carte_cuite.h), vérifie
la magie, la version du format, le CRC-32 de l'en-tête, la taille et le CRC-32 de chaque bloc, puis écrit la liste.

Format écrit (lu par le jeu : android/.../ListePaquets.java de WFS-306 ; le jeu ignore les champs qu'il ne connaît pas) :
  {
    "version": 1,                      version du format de la liste (ListePaquets.VERSION_LISTE)
    "version_liste": 1,                même valeur, nom de RG-03
    "carte_par_defaut": "ville",       décision de l'utilisateur (RG-03)
    "version_min_apk": 927,            versionCode minimal (1er APK qui lit les paquets, WFS-301)
    "commit": "<sha>",                 commit cuit (information)
    "paquets": [ { "carte", "nom", "format", "version_carte", "version_format", "empreinte" (16 chiffres hexadécimaux),
                   "version_min_apk", "taille", "sha256", "adresse" (adresse directe du fichier, jamais l'API GitHub) } ]
  }
Le fichier est stable : mêmes paquets → même cartes.json, octet par octet (aucune date).

Usage : python3 tools/qa/cartes_json.py <dossier des paquets> <sortie cartes.json> --adresse-base <URL> --version-min-apk N
        [--commit SHA] [--nom ville=Ville ...] [--versionne --publies <fichier>]
Code de sortie : 0 si bon ; 2 si un paquet manque, est illisible ou si le dossier est vide.
"""
import argparse
import hashlib
import json
import re
import struct
import sys
import zlib
from pathlib import Path

VERSION_LISTE = 1
VERSION_FORMAT = 1          # carte::VERSION_FORMAT (src/core/carte_cuite.h)
FORMATS = ["astc", "etc2", "rgba8"]   # ordre de carte::FORMATS ; numéro dans l'en-tête : 0 ASTC, 1 ETC2, 2 RGBA8
NOM_FICHIER = re.compile(r"^([a-z0-9_]+)_(astc|etc2|rgba8)\.wfcarte$")


class PaquetFaux(Exception):
    pass


def lire_entete(octets):
    """En-tête d'un paquet .wfcarte (petit-boutiste) ; PaquetFaux si quelque chose ne va pas."""
    def lire(fmt, pos):
        n = struct.calcsize(fmt)
        if pos + n > len(octets):
            raise PaquetFaux("en-tête tronqué")
        return struct.unpack_from(fmt, octets, pos), pos + n

    if octets[:4] != b"WFSC":
        raise PaquetFaux("magie fausse")
    (version_format,), p = lire("<I", 4)
    if version_format != VERSION_FORMAT:
        raise PaquetFaux(f"version du format {version_format} (attendue {VERSION_FORMAT})")
    (taille_entete,), p = lire("<I", p)
    (n,), p = lire("<H", p)
    if p + n > len(octets):
        raise PaquetFaux("nom tronqué")
    carte = octets[p:p + n].decode("utf-8")
    p += n
    (version_carte, fmt, empreinte, nb_blocs), p = lire("<IBQI", p)
    if fmt >= len(FORMATS):
        raise PaquetFaux(f"format de texture inconnu {fmt}")
    blocs = []
    for _ in range(nb_blocs):
        (n,), p = lire("<B", p)
        if p + n > len(octets):
            raise PaquetFaux("nom de bloc tronqué")
        nom = octets[p:p + n].decode("utf-8")
        p += n
        (position, taille, crc), p = lire("<QQI", p)
        blocs.append((nom, position, taille, crc))
    (crc_entete,), fin = lire("<I", p)
    if zlib.crc32(octets[:p]) & 0xFFFFFFFF != crc_entete:
        raise PaquetFaux("CRC de l'en-tête faux")
    if fin != taille_entete:
        raise PaquetFaux(f"taille de l'en-tête {taille_entete} ≠ {fin}")
    for nom, position, taille, crc in blocs:
        if position + taille > len(octets):
            raise PaquetFaux(f"bloc {nom} hors du fichier")
        if zlib.crc32(octets[position:position + taille]) & 0xFFFFFFFF != crc:
            raise PaquetFaux(f"CRC du bloc {nom} faux")
    return {"carte": carte, "version_format": version_format, "version_carte": version_carte,
            "format": FORMATS[fmt], "empreinte": f"{empreinte:016x}", "blocs": [b[0] for b in blocs]}


def nom_publie(nom_local, sha256):
    """Nom du fichier dans la Release : <carte>_<format>-<12 premiers chiffres du SHA-256>.wfcarte. Un contenu nouveau a donc
    une adresse nouvelle : la liste publiée ne montre jamais un fichier remplacé entre-temps (le téléphone range toujours le
    paquet sous <carte>_<format>.wfcarte, WFS-306)."""
    return nom_local[:-len(".wfcarte")] + "-" + sha256[:12] + ".wfcarte"


def construire(dossier, adresse_base, version_min_apk, commit="", noms=None, versionne=False):
    noms = noms or {}
    fichiers = sorted(f for f in Path(dossier).iterdir() if f.is_file() and NOM_FICHIER.match(f.name))
    if not fichiers:
        raise PaquetFaux(f"aucun paquet .wfcarte dans {dossier}")
    paquets = []
    for f in fichiers:
        carte_nom, fmt_nom = NOM_FICHIER.match(f.name).groups()
        octets = f.read_bytes()
        try:
            e = lire_entete(octets)
        except PaquetFaux as err:
            raise PaquetFaux(f"{f.name} : {err}")
        if e["carte"] != carte_nom or e["format"] != fmt_nom:
            raise PaquetFaux(f"{f.name} : l'en-tête dit {e['carte']} / {e['format']}")
        sha = hashlib.sha256(octets).hexdigest()
        publie = nom_publie(f.name, sha) if versionne else f.name
        paquets.append({
            "carte": e["carte"], "nom": noms.get(e["carte"], e["carte"].capitalize()), "format": e["format"],
            "version_carte": e["version_carte"], "version_format": e["version_format"], "empreinte": e["empreinte"],
            "version_min_apk": version_min_apk, "taille": len(octets), "sha256": sha,
            "adresse": adresse_base.rstrip("/") + "/" + publie, "_local": f.name, "_publie": publie,
        })
    # ordre stable : carte, puis format dans l'ordre préféré du jeu (ASTC, ETC2, RGBA8)
    paquets.sort(key=lambda x: (x["carte"], FORMATS.index(x["format"])))
    publies = [(p.pop("_local"), p.pop("_publie")) for p in paquets]
    return publies, {"version": VERSION_LISTE, "version_liste": VERSION_LISTE, "carte_par_defaut": "ville",
            "version_min_apk": version_min_apk, "commit": commit, "paquets": paquets}


def main(argv=None):
    a = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    a.add_argument("dossier")
    a.add_argument("sortie")
    a.add_argument("--adresse-base", required=True)
    a.add_argument("--version-min-apk", type=int, required=True)
    a.add_argument("--commit", default="")
    a.add_argument("--nom", action="append", default=[], help="carte=Nom affiché")
    a.add_argument("--versionne", action="store_true", help="adresses <carte>_<format>-<sha12>.wfcarte (publication)")
    a.add_argument("--publies", help="écrit « <fichier local> <nom publié> », une ligne par paquet")
    o = a.parse_args(argv)
    if not o.adresse_base.startswith("https://"):
        print("cartes.json : l'adresse de base doit commencer par https://", file=sys.stderr)
        return 2
    noms = dict(x.split("=", 1) for x in o.nom)
    try:
        publies, liste = construire(o.dossier, o.adresse_base, o.version_min_apk, o.commit, noms, o.versionne)
    except PaquetFaux as err:
        print(f"cartes.json : {err}", file=sys.stderr)
        return 2
    Path(o.sortie).write_text(json.dumps(liste, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    if o.publies:
        Path(o.publies).write_text("".join(f"{a} {b}\n" for a, b in publies), encoding="utf-8")
    for p in liste["paquets"]:
        print(f"{p['carte']}_{p['format']}.wfcarte : {p['taille'] / 1048576:.2f} Mo, sha256 {p['sha256']}, empreinte {p['empreinte']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
