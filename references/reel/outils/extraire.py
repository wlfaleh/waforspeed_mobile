#!/usr/bin/env python3
# WaForSpeed — WFS-148 : extraction reproductible des jeux de données libres du référentiel du réel.
# Produit, dans references/reel/ : materiaux.json, photometrie.json, colorchecker.json, anthropometrie.json,
# soleil_controle.json. Les autres fichiers (voitures.json, marquage.json, lieux.json) sont des relevés écrits à la main,
# chacun avec sa source (voir SOURCES.md).
#
# Sources téléchargées (à passer en arguments, jamais exécutées) :
#   --pb <dossier>     dépôt github.com/AntonPalmqvist/physically-based-api (CC0 1.0), fichiers deploy/v2/*.json
#   --ansur <dossier>  copie des fichiers publics ANSUR II (« ANSUR II MALE Public.csv », « ANSUR II FEMALE Public.csv »,
#                      domaine public, armée des États-Unis), par ex. github.com/senihberkay/US-Army-ANSUR-II
# Bibliothèques (PyPI) : colour-science 0.4.7 (BSD-3 : données ColorChecker de X-Rite 2016), pvlib 0.16.1 (BSD-3 :
#   algorithme NREL SPA, Reda et Andreas 2004), numpy, pandas.
# Usage : python3 extraire.py --pb <dossier> --ansur <dossier> --sortie references/reel
import argparse, csv, hashlib, json, os, statistics

def sha256(chemin):
    h = hashlib.sha256()
    with open(chemin, 'rb') as f:
        for bloc in iter(lambda: f.read(1 << 20), b''): h.update(bloc)
    return h.hexdigest()

def ecrire(sortie, nom, doc):
    with open(os.path.join(sortie, nom), 'w', encoding='utf-8') as f:
        json.dump(doc, f, ensure_ascii=False, indent=1); f.write('\n')

def rel10():   # tolérance par défaut des albédos et intensités sans plage publiée : DA-realisme.md § 3 (± 10 %)
    return {"type": "relative", "valeur": 0.10, "regle": "DA-realisme.md § 3 (valeur de référence à ± 10 %)"}

# ---------------------------------------------------------------- matériaux (Physically Based, CC0)
MATERIAUX = [   # nom dans la base → identifiant du référentiel
    ("Asphalt (Fresh)", "asphalte_neuf"), ("Concrete", "beton"), ("Brick", "brique"), ("Grass", "herbe"), ("Sand", "sable"),
    ("Snow", "neige"), ("Car Paint", "peinture_voiture"), ("Glass (Soda-lime)", "verre"), ("Chromium", "chrome"),
    ("Tire", "pneu"), ("Skin I", "peau_i"), ("Skin III", "peau_iii"), ("Skin VI", "peau_vi"), ("Gray Card", "charte_grise_18"),
    ("Water", "eau"), ("Aluminum", "aluminium"), ("Stainless Steel", "acier_inox"), ("Terracotta", "terre_cuite"), ("Rust", "rouille"),
]

def materiaux(pb, sortie):
    chemin = os.path.join(pb, 'deploy', 'v2', 'materials.json')
    doc = json.load(open(chemin, encoding='utf-8'))
    par_nom = {m['name']: m for m in doc['data']}
    sources = {"PB-MAT": {"titre": "Physically Based — base de valeurs physiques (materials.json, schéma v" + str(doc['header']['schemaVersion']) + ")",
                          "editeur": "A. Palmqvist et contributeurs", "annee": str(doc['header']['updated'])[:4],
                          "lien": "https://github.com/AntonPalmqvist/physically-based-api/blob/main/deploy/v2/materials.json",
                          "licence": "CC0 1.0", "acces": "ouvert", "empreinte_sha256": sha256(chemin),
                          "note": "chaque valeur garde ses références d'origine (champ « references » de la base), recopiées ci-dessous"}}
    mesures = []
    for nom, ident in MATERIAUX:
        m = par_nom[nom]
        lin = next(c['color'] for c in m['color'] if c['colorSpace'] == 'srgb-linear')
        refs = [{k: r[k] for k in ('title', 'author', 'url', 'doi', 'year') if k in r} for r in m.get('references', [])]
        base = {"source": "PB-MAT", "statut": "verifie", "references_origine": refs, "nom_base": nom}
        mesures.append(dict(base, id=f"mat.{ident}.albedo", domaine="materiaux", grandeur="albédo (couleur de base, sRGB linéaire)",
                            unite="sRGB linéaire", valeur=lin, tolerance=rel10()))
        mesures.append(dict(base, id=f"mat.{ident}.metal", domaine="materiaux", grandeur="métal (0 ou 1)", unite="-",
                            valeur=m.get('metalness', 0), tolerance={"type": "absolue", "valeur": 0}))
        if 'roughness' in m:
            mesures.append(dict(base, id=f"mat.{ident}.rugosite", domaine="materiaux", grandeur="rugosité", unite="-",
                                valeur=m['roughness'], tolerance={"type": "absolue", "valeur": 0.1, "regle": "DA-realisme.md § 4 (plage sourcée) : ± 0,1 faute de plage publiée"}))
        if 'ior' in m and not m.get('metalness'):
            n = m['ior']; f0 = ((n - 1) / (n + 1)) ** 2
            mesures.append(dict(base, id=f"mat.{ident}.f0", domaine="materiaux", grandeur="F0 (réflexion de face, diélectrique)", unite="-",
                                valeur=round(f0, 5), statut="derive", formule=f"F0 = ((n − 1) / (n + 1))² avec n = {n} (indice de la base)",
                                tolerance={"type": "absolue", "valeur": 0.005}))
    ecrire(sortie, 'materiaux.json', {"fichier": "materiaux.json", "description": "Matériaux physiques (albédo, rugosité, métal, F0) — WFS-148 RG-01 (d)",
                                       "sources": sources, "mesures": mesures})

# ---------------------------------------------------------------- photométrie (Physically Based, CC0)
def photometrie(pb, sortie):
    chemin = os.path.join(pb, 'deploy', 'v2', 'lightsources.json')
    doc = json.load(open(chemin, encoding='utf-8'))
    par_nom = {m['name']: m for m in doc['data']}
    sources = {"PB-LUM": {"titre": "Physically Based — base de valeurs physiques (lightsources.json, schéma v" + str(doc['header']['schemaVersion']) + ")",
                          "editeur": "A. Palmqvist et contributeurs", "annee": str(doc['header']['updated'])[:4],
                          "lien": "https://github.com/AntonPalmqvist/physically-based-api/blob/main/deploy/v2/lightsources.json",
                          "licence": "CC0 1.0", "acces": "ouvert", "empreinte_sha256": sha256(chemin),
                          "note": "unités de la base : lx (lumières directionnelles et ciel), lm (projecteurs) ; références d'origine recopiées"},
               "EV100": {"titre": "Définition de l'indice de lumination EV100 : E = 2,5 × 2^EV100 (constante de cellule C = 250 lx·s pour ISO 100)",
                         "editeur": "ISO 2720:1974 (constante d'étalonnage des posemètres à cellule incidente)", "annee": "1974",
                         "lien": "https://www.iso.org/standard/7690.html", "acces": "non_ouvert",
                         "note": "formule d'usage courant ; norme payante non ouverte : statut des valeurs converties « à confirmer »"}}
    mesures = []
    def ajoute(ident, nom, fmt, grandeur):
        m = par_nom[nom]; v = next(x for x in m['variants'] if x['format'] == fmt)
        refs = [{k: r[k] for k in ('title', 'author', 'url', 'year') if k in r} for r in m.get('references', [])]
        unite = m['unit'][0]
        inten = v.get('intensity')
        val = {"valeur": inten[0]} if inten and len(inten) == 1 else {"min": min(inten[:2]), "max": max(inten[:2]), "typique": inten[2] if len(inten) > 2 else None} if inten else {"valeur": None}
        mesures.append(dict(val, id=f"lum.{ident}.intensite", domaine="photometrie", grandeur=grandeur, unite=unite, source="PB-LUM",
                            statut="verifie", nom_base=f"{nom} / {fmt}", tolerance={"type": "relative", "valeur": 0.05, "regle": "DA-realisme.md § 4 (photométrie ± 5 %)"},
                            references_origine=refs))
        if 'temperature' in v:
            t = v['temperature']
            tv = {"valeur": t[0]} if len(t) == 1 else {"min": min(t[:2]), "max": max(t[:2])}
            mesures.append(dict(tv, id=f"lum.{ident}.temperature", domaine="photometrie", grandeur="température de couleur proximale", unite="K",
                                source="PB-LUM", statut="verifie", nom_base=f"{nom} / {fmt}", tolerance={"type": "absolue", "valeur": 100, "regle": "DA-realisme.md § 4 (± 100 K)"}))
    ajoute("soleil_midi", "Sun", "Midday", "éclairement du soleil seul, à midi, sol horizontal")
    ajoute("soleil_couchant", "Sun", "Sunset", "éclairement du soleil au coucher")
    ajoute("ciel_clair", "Sky", "Clear", "éclairement à l'ombre, ciel bleu clair à midi")
    ajoute("ciel_couvert", "Sky", "Overcast Day", "éclairement, ciel couvert, de jour")
    ajoute("nuit_sans_lune", "Sky", "Moonless Clear Night", "éclairement, nuit claire sans lune")
    ajoute("pleine_lune", "Moon", "Full", "éclairement de la pleine lune")
    ajoute("lampadaire_sodium", "Street Light", "Sodium-vapor Lamp", "flux lumineux d'une lampe de lampadaire au sodium")
    ajoute("phare_halogene", "Car Headlight", "Halogen", "flux lumineux d'un phare halogène (feu de croisement)")
    ajoute("phare_led", "Car Headlight", "LED", "flux lumineux d'un phare à LED (feu de croisement)")
    # EV100 du soleil de midi : dérivé de l'éclairement (E = 2,5 × 2^EV)
    import math
    e = next(x for x in par_nom['Sun']['variants'] if x['format'] == 'Midday')['intensity'][2]
    mesures.append({"id": "lum.soleil_midi.ev100", "domaine": "photometrie", "grandeur": "EV100 d'une scène au soleil de midi (incident)", "unite": "EV",
                    "valeur": round(math.log2(e / 2.5), 2), "source": "EV100", "statut": "a_confirmer",
                    "formule": f"EV100 = log2(E / 2,5) avec E = {e} lx (PB-LUM, Sun / Midday)", "tolerance": {"type": "absolue", "valeur": 0.1}})
    ecrire(sortie, 'photometrie.json', {"fichier": "photometrie.json", "description": "Photométrie (lux, lm, K, EV100) — WFS-148 RG-01 (c)",
                                         "sources": sources, "mesures": mesures})

# ---------------------------------------------------------------- charte ColorChecker (X-Rite 2016, via colour-science)
NOMS_CC = ["peau foncée", "peau claire", "ciel bleu", "feuillage", "fleur bleue", "vert bleuté", "orange", "bleu violacé",
           "rouge moyen", "violet", "vert jaune", "jaune orangé", "bleu", "vert", "rouge", "jaune", "magenta", "cyan",
           "blanc 9.5", "neutre 8", "neutre 6.5", "neutre 5", "neutre 3.5", "noir 2"]

def colorchecker(sortie):
    import numpy as np, colour
    from colour.characterisation.datasets.colour_checkers import chromaticity_coordinates as cc
    lab = list(cc.DATA_COLORCHECKER24_AFTER_NOV2014_CIE_LAB.values())
    d50 = colour.CCS_ILLUMINANTS["CIE 1931 2 Degree Standard Observer"]["ICC D50"]
    d65 = colour.CCS_ILLUMINANTS["CIE 1931 2 Degree Standard Observer"]["D65"]
    sources = {"XRITE-2016": {"titre": "New color specifications for ColorChecker SG and Classic Charts (valeurs CIE L*a*b* D50, 2°, après novembre 2014)",
                              "editeur": "X-Rite", "annee": "2016",
                              "lien": "https://www.xrite.com/service-support/new_color_specifications_for_colorchecker_sg_and_classic_charts",
                              "acces": "non_ouvert",
                              "note": "valeurs lues dans le jeu de données de colour-science " + colour.__version__ + " (BSD-3), qui les reprend de X-Rite (2016) : DATA_COLORCHECKER24_AFTER_NOV2014_CIE_LAB"},
               "COLOUR-SCI": {"titre": "colour-science " + colour.__version__ + " (conversions Lab → XYZ → sRGB, adaptation de Bradford, ΔE CIE 2000)",
                              "editeur": "Colour Developers", "annee": "2026", "lien": "https://pypi.org/project/colour-science/", "licence": "BSD-3-Clause", "acces": "ouvert"}}
    mesures = []
    for i, (n, v) in enumerate(zip(NOMS_CC, lab)):
        xyz50 = colour.Lab_to_XYZ(np.array(v), d50)
        xyz65 = colour.chromatic_adaptation(xyz50, colour.xy_to_XYZ(d50), colour.xy_to_XYZ(d65), method='Von Kries', transform='Bradford')
        rgb = colour.XYZ_to_RGB(xyz65, 'sRGB', apply_cctf_encoding=False)
        srgb8 = np.clip(colour.cctf_encoding(np.clip(rgb, 0, 1), function='sRGB') * 255, 0, 255)
        mesures.append({"id": f"cc.{i + 1:02d}", "domaine": "couleurs", "grandeur": f"case {i + 1} « {n} » : CIE L*a*b* (D50, 2°)", "unite": "L*a*b*",
                        "valeur": [float(x) for x in v], "source": "XRITE-2016", "statut": "a_confirmer",
                        "note": "lu dans colour-science (copie publiée des valeurs de X-Rite) ; page de X-Rite non ouverte (réseau)",
                        "tolerance": {"type": "deltaE2000", "valeur": 2, "regle": "DA-realisme.md § 4 (ΔE2000 ≤ 2)"}})
        mesures.append({"id": f"cc.{i + 1:02d}.albedo", "domaine": "couleurs", "grandeur": f"case {i + 1} : albédo sRGB linéaire (D65, Bradford)", "unite": "sRGB linéaire",
                        "valeur": [round(float(x), 5) for x in rgb], "source": "COLOUR-SCI", "statut": "derive",
                        "formule": "Lab D50 → XYZ D50 → Bradford → XYZ D65 → sRGB linéaire (colour-science)",
                        "srgb8": [int(round(float(x))) for x in srgb8], "tolerance": {"type": "deltaE2000", "valeur": 2}})
    # valeurs de contrôle de ΔE2000 (pour vérifier le calcul C++ du banc) : paires de cases et quelques écarts
    paires = []
    for a, b in [(0, 1), (2, 12), (5, 17), (18, 23), (14, 16), (6, 11), (3, 13), (19, 20)]:
        paires.append({"a": [float(x) for x in lab[a]], "b": [float(x) for x in lab[b]],
                       "deltaE2000": round(float(colour.delta_E(lab[a], lab[b], method='CIE 2000')), 4)})
    ecrire(sortie, 'colorchecker.json', {"fichier": "colorchecker.json", "description": "Charte ColorChecker Classic 24 cases — WFS-148 RG-01 (e), RG-04",
                                          "sources": sources, "mesures": mesures, "controle_deltaE2000": paires})

# ---------------------------------------------------------------- anthropométrie (ANSUR II, domaine public)
def anthropometrie(ansur, sortie):
    fichiers = {"hommes": os.path.join(ansur, 'ANSUR II MALE Public.csv'), "femmes": os.path.join(ansur, 'ANSUR II FEMALE Public.csv')}
    sources = {"ANSUR-II": {"titre": "2012 Anthropometric Survey of U.S. Army Personnel (ANSUR II) : fichiers publics (NATICK/TR-15/007)",
                            "editeur": "U.S. Army Natick Soldier RD&E Center", "annee": "2014",
                            "lien": "https://www.openlab.psu.edu/ansur2/", "licence": "domaine public (gouvernement des États-Unis)", "acces": "ouvert",
                            "empreintes_sha256": {k: sha256(v) for k, v in fichiers.items()},
                            "note": "fichiers lus dans la copie github.com/senihberkay/US-Army-ANSUR-II (empreintes ci-contre) ; population militaire des "
                                    "États-Unis (pas la population française) : sert aux proportions et aux plages de taille"}}
    mesures = []
    for sexe, chemin in fichiers.items():
        rows = list(csv.DictReader(open(chemin, encoding='latin-1')))
        for col, nom in (("stature", "taille (stature)"), ("crotchheight", "hauteur de l'entrejambe"), ("acromialheight", "hauteur des épaules (acromion)"),
                         ("span", "envergure"), ("biacromialbreadth", "largeur des épaules (biacromiale)")):
            v = sorted(float(r[col]) for r in rows); q = statistics.quantiles(v, n=100)
            mesures.append({"id": f"humain.{sexe}.{col}", "domaine": "anthropometrie", "grandeur": f"{nom}, {sexe}", "unite": "mm",
                            "valeur": round(statistics.mean(v), 1), "ecart_type": round(statistics.stdev(v), 1),
                            "p5": round(q[4], 1), "p95": round(q[94], 1), "n": len(v), "source": "ANSUR-II", "statut": "verifie",
                            "tolerance": {"type": "relative", "valeur": 0.01, "regle": "DA-realisme.md § 4 (dimensions ± 1 %) sur la moyenne d'une foule"}})
        for col, nom in (("crotchheight", "entrejambe / taille"), ("acromialheight", "épaules / taille"), ("span", "envergure / taille")):
            r = [float(x[col]) / float(x['stature']) for x in rows]
            mesures.append({"id": f"humain.{sexe}.ratio_{col}", "domaine": "anthropometrie", "grandeur": f"proportion {nom}, {sexe}", "unite": "-",
                            "valeur": round(statistics.mean(r), 4), "ecart_type": round(statistics.stdev(r), 4), "source": "ANSUR-II", "statut": "derive",
                            "formule": f"moyenne de {col} / stature par personne", "tolerance": {"type": "relative", "valeur": 0.01}})
    mesures.append({"id": "humain.tete_sur_taille", "domaine": "anthropometrie", "grandeur": "hauteur de la tête (vertex – menton) / taille", "unite": "-",
                    "valeur": None, "source": "ANSUR-II", "statut": "a_sourcer",
                    "note": "les fichiers publics d'ANSUR II n'ont pas la hauteur vertex – menton ; la règle « 1/7,5 » de la charte n'a pas de source ouverte"})
    ecrire(sortie, 'anthropometrie.json', {"fichier": "anthropometrie.json", "description": "Anthropométrie adulte — WFS-148 RG-01 (f)",
                                            "sources": sources, "mesures": mesures})

# ---------------------------------------------------------------- soleil : valeurs de contrôle (NREL SPA, via pvlib)
CAS_SOLEIL = [   # latitude, longitude (° E), date et heure UTC — 10 cas variés (hémisphères, saisons, matin / soir)
    (43.5, 7.0, "2026-06-21T12:00:00"), (43.5, 7.0, "2026-12-21T10:00:00"), (45.0, 5.0, "2026-03-20T08:00:00"),
    (0.0, 0.0, "2026-03-20T12:00:00"), (-33.9, 18.4, "2026-01-15T14:00:00"), (60.2, 24.9, "2026-07-01T15:30:00"),
    (40.7, -74.0, "2026-09-22T17:00:00"), (35.7, 139.7, "2026-04-10T03:00:00"), (-23.5, -46.6, "2026-11-06T18:20:00"),
    (51.5, -0.1, "2026-10-31T13:45:00")]

def soleil(sortie):
    import pandas as pd, pvlib
    sources = {"NREL-SPA": {"titre": "Solar Position Algorithm for Solar Radiation Applications (NREL/TP-560-34302), incertitude ± 0,0003°",
                            "editeur": "I. Reda et A. Andreas, National Renewable Energy Laboratory", "annee": "2004",
                            "lien": "https://www.nrel.gov/docs/fy08osti/34302.pdf", "acces": "non_ouvert",
                            "note": "valeurs calculées par pvlib " + pvlib.__version__ + " (solarposition.spa_python, BSD-3), ΔT = 67 s, 1013,25 hPa, 12 °C ; "
                                    "le calculateur de la NOAA (gml.noaa.gov/grad/solcalc) ne s'ouvre pas depuis la session : la SPA, plus précise que "
                                    "l'algorithme de la NOAA (± 0,0167°), sert de référence"},
               "NOAA-SOLCALC": {"titre": "NOAA Solar Calculator — General Solar Position Calculations (équations du tableur)",
                                "editeur": "NOAA Global Monitoring Laboratory", "annee": "2024",
                                "lien": "https://gml.noaa.gov/grad/solcalc/solareqns.PDF", "acces": "non_ouvert",
                                "note": "équations de J. Meeus (Astronomical Algorithms, 1991) ; implantées dans soleil() (src/render/bench_realisme.h)"}}
    mesures = []
    for k, (lat, lon, t) in enumerate(CAS_SOLEIL):
        r = pvlib.solarposition.spa_python(pd.DatetimeIndex([t], tz='UTC'), lat, lon).iloc[0]
        base = {"source": "NREL-SPA", "statut": "verifie", "lat": lat, "lon": lon, "utc": t, "tolerance": {"type": "absolue", "valeur": 0.05, "regle": "WFS-148 RG-02 (± 0,05°)"}}
        mesures.append(dict(base, id=f"soleil.cas{k + 1:02d}.hauteur", domaine="soleil", grandeur="hauteur géométrique (sans réfraction)", unite="°", valeur=round(float(r['elevation']), 4)))
        mesures.append(dict(base, id=f"soleil.cas{k + 1:02d}.hauteur_apparente", domaine="soleil", grandeur="hauteur apparente (réfraction)", unite="°", valeur=round(float(r['apparent_elevation']), 4)))
        mesures.append(dict(base, id=f"soleil.cas{k + 1:02d}.azimut", domaine="soleil", grandeur="azimut (depuis le nord, vers l'est)", unite="°", valeur=round(float(r['azimuth']), 4)))
    ecrire(sortie, 'soleil_controle.json', {"fichier": "soleil_controle.json", "description": "Position du soleil : 10 cas de contrôle — WFS-148 RG-02",
                                             "sources": sources, "mesures": mesures})

if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--pb', required=True); p.add_argument('--ansur', required=True); p.add_argument('--sortie', required=True)
    a = p.parse_args()
    materiaux(a.pb, a.sortie); photometrie(a.pb, a.sortie); colorchecker(a.sortie); anthropometrie(a.ansur, a.sortie); soleil(a.sortie)
    print("référentiel extrait dans", a.sortie)
