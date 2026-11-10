#!/usr/bin/env python3
# WaForSpeed — crédits et vérification croisée des licences (WFS-22 / WFS-25 RG-04, WFS-406 RG-02, RG-03, RG-07).
# Une seule source pour les crédits : ce script fabrique assets/licences/CREDITS.txt (lu par l'écran « Crédits » du jeu, dans
# l'APK sous licences/CREDITS.txt) et store/credits.md (fiche des stores), à partir de :
#   - src/game/cars.h : auteur et licence de chaque voiture (champ credit de CARS[], titre d'origine NON repris : il nomme la
#     vraie voiture ; CC BY 4.0 et CC BY-SA 4.0 ne l'exigent pas, il reste dans assets/licences/<voiture>.txt) ; les voitures de
#     VOITURES_RETIREES (WFS-404, hors de l'APK) sont sautées ;
#   - configgame.json : noms inventés affichés dans le jeu (section voitures) ;
#   - la table AUTRES ci-dessous (deux-roues, personnes, animaux, routes, sons, police, code tiers), lue dans assets/licences/*.txt.
#   python3 tools/qa/verifier_licences.py            → vérifie (code de sortie 1 en cas de manque)
#   python3 tools/qa/verifier_licences.py --generer  → réécrit store/credits.md et assets/licences/CREDITS.txt
# Vérifications (RG-07) : chaque actif livré dans assets/ est couvert par une ligne de CREDITS.txt (ou créé par le projet) ;
# aucune marque ni aucun modèle réel de cars.h (ni des deux-roues) dans CREDITS.txt / store/credits.md, hors titres des sons
# CC BY-SA 3.0 (titre exigé par la licence, sans marque) ; les deux fichiers sont à jour.
import json, pathlib, re, sys

RACINE = pathlib.Path(__file__).resolve().parents[2]
LIC = RACINE / "assets" / "licences"

# liens des licences (RG-02 : auteur, licence et lien de la licence)
LIEN = {
    "CC BY 4.0": "https://creativecommons.org/licenses/by/4.0/",
    "CC BY-SA 4.0": "https://creativecommons.org/licenses/by-sa/4.0/",
    "CC BY-SA 3.0": "https://creativecommons.org/licenses/by-sa/3.0/",
    "CC0 1.0": "https://creativecommons.org/publicdomain/zero/1.0/",
    "MIT": "https://opensource.org/license/mit",
    "SIL OFL 1.1": "https://openfontlicense.org",
    "Apache 2.0": "https://www.apache.org/licenses/LICENSE-2.0",
}

# autres contenus livrés : (section, libellé, texte du crédit, licence) ; textes repris des fichiers assets/licences/*.txt
AUTRES = [
    ("Deux-roues du trafic (modèles 3D modifiés pour WaForSpeed : allégés, logos effacés)", [
        ("Scooter", "modèle 3D par neshallads (Sketchfab)", "CC BY 4.0"),
        ("Moto routière", "modèle 3D par DevanirGrau (Sketchfab)", "CC BY 4.0"),
        ("Moto sportive", "modèle 3D par valvetin (Sketchfab)", "CC BY 4.0"),
    ]),
    ("Personnes", [
        ("Piétons et animations", "« Universal Base Characters » et « Universal Animation Library » par Quaternius (quaternius.com)", "CC0 1.0"),
        ("Humains réalistes et enfants", "ressources de MakeHuman (communauté MakeHuman), modifiées pour WaForSpeed", "CC0 1.0"),
    ]),
    ("Animaux", [
        ("Vache, cheval, mouton, chiens, dauphin", "« Ultimate Animated Animal Pack », « Farm Animals Pack » et « Animated Fish Pack » par Quaternius, modifiés pour WaForSpeed", "CC0 1.0"),
        ("Goéland et pigeon", "modèle « seagull » par svartmc (Copyright (c) 2020 svartmc), modifié pour WaForSpeed (texte de la licence en fin de page)", "MIT"),
    ]),
    ("Revêtements de route", [
        ("Enrobés, béton, pavés, gravier", "scans de Poly Haven (polyhaven.com), modifiés pour WaForSpeed", "CC0 1.0"),
    ]),
    ("Sons des moteurs (modifiés pour WaForSpeed : mis en boucle, niveau égalisé)", [
        # WFS-406 : version de la licence de nhaudio non indiquée par la source lue (CDDA-Soundpacks, « CC-BY ») : jamais inventée
        ("v8.wav et thermique.wav", "« engine working internal » par nhaudio (freesound.org), version de la licence non indiquée par la source", "CC BY"),
        ("sport.wav", "« Car engine loop (96 kHz, 4 s) » par Ivan « qubodup » Gabovitch, version modifiée partagée sous la même licence", "CC BY-SA 3.0"),
        ("citadine.wav", "« engine_idle.wav » par mhad (freesound.org)", "CC0 1.0"),
        ("gros.wav", "« engine_large » par Marianne Gagnon (SuperTuxKart), mixage de « Transfer Truck Idle and Drive » (Mike Koenig), "
                     "« Compressor Motor » (Stephan) et « Car Accelerating » (SpeedJunkie), version modifiée partagée sous la même licence", "CC BY-SA 3.0"),
    ]),
    ("Police", [
        ("Rajdhani", "Copyright (c) 2014, Indian Type Foundry", "SIL OFL 1.1"),
    ]),
    ("Code", [
        ("stb_image, stb_image_write, stb_truetype", "Sean Barrett (domaine public ou licence MIT, au choix)", "MIT"),
        ("android_native_app_glue", "Copyright (C) 2010 The Android Open Source Project (texte de la licence en fin de page)", "Apache 2.0"),
    ]),
]
LIEN["CC BY"] = "https://creativecommons.org/licenses/"   # version inconnue : page des licences, sans en supposer une

# textes complets à donner avec le jeu (MIT : notice dans toutes les copies ; Apache 2.0 § 4 a : copie de la licence)
def texte_mit():
    t = (LIC / "animaux-corniche.txt").read_text(encoding="utf-8")
    debut = t.index("  MIT License"); fin = t.index("SOFTWARE.", debut) + len("SOFTWARE.")
    return "\n".join(l[2:] if l.startswith("  ") else l for l in t[debut:fin].split("\n"))

def texte_apache():
    return (LIC / "apache-2.0.txt").read_text(encoding="utf-8").strip("\n")

# actifs livrés (dossier assets/, copié dans l'APK) → texte qui doit figurer dans CREDITS.txt (None : créé par le projet)
COUVERTURE = [
    (r"cars/deux_roues/scooter.*", "neshallads"), (r"cars/deux_roues/routiere.*", "DevanirGrau"), (r"cars/deux_roues/sportive.*", "valvetin"),
    (r"people/(homme|femme)\.wfp", "Quaternius"), (r"people/humains/.*\.wfp", "MakeHuman"),
    (r"world/animaux/(vache|cheval|mouton|shiba|husky|dauphin)\.wfa", "Quaternius"), (r"world/animaux/mouette\.wfa", "svartmc"),
    (r"world/animaux/.*\.py", None),        # scripts de conversion du projet
    (r"world/feuillage\.png", None),        # atlas dessiné par le code du jeu (tools/atlas_feuillage.cjs)
    (r"textures/route/.*", "Poly Haven"), (r"textures/essai/.*", None),
    (r"fonts/Rajdhani-.*\.ttf", "Rajdhani"), (r"textes/.*", None),
]
# marques des deux-roues d'origine (titres des modèles, assets/licences/deux-roues-*.txt) : jamais dans les crédits
MARQUES_DEUX_ROUES = ["Honda", "CB500X", "OLA", "Kawasaki", "Ninja", "ZX-6R"]
# mots génériques des modèles de cars.h, sans risque de marque (non cherchés)
GENERIQUES = {"berline", "spyder", "competition", "sport", "model", "range"}

def voitures():
    src = (RACINE / "src" / "game" / "cars.h").read_text(encoding="utf-8")
    motif = re.compile(r'\{"(\w+)",\s*"([^"]*)",\s*"([^"]*)",\s*"(cars/[\w-]+\.wfm)".*?"(«[^"]*)"\}', re.S)
    return [dict(id=m[1], marque=m[2], modele=m[3], fichier=m[4], credit=m[5]) for m in motif.finditer(src)]

def retirees():
    # WFS-404 (RG-02) : liste unique VOITURES_RETIREES de cars.h (licence non commerciale : hors de l'APK et des crédits)
    src = (RACINE / "src" / "game" / "cars.h").read_text(encoding="utf-8")
    m = re.search(r'VOITURES_RETIREES\[\]\s*=\s*\{([^}]*)\}', src)
    return set(re.findall(r'"([\w-]+)"', m[1])) if m else set()

def noms_affiches():
    v = json.loads((RACINE / "configgame.json").read_text(encoding="utf-8")).get("voitures", {})
    return {i: f"{n.get('marque', '')} {n.get('modele', '')}".strip() for i, n in v.items()}

def auteur_licence(credit):
    # « titre » par AUTEUR (Sketchfab), LICENCE → (« modèle 3D par AUTEUR (Sketchfab) », LICENCE) ; titre d'origine écarté
    m = re.match(r'«.*» par (.+), (CC[^,]*)$', credit)
    return (f"modèle 3D par {m[1]}", m[2].strip()) if m else (credit, "")

def lignes_voitures(vs):
    noms = noms_affiches()
    return [(noms.get(v["id"], v["id"]),) + auteur_licence(v["credit"]) for v in vs]

def credits_txt(vs):
    l = ["WaForSpeed — crédits", "",
         "Ce jeu utilise des œuvres partagées sous licence libre : merci à leurs auteurs.",
         "Les noms des voitures et des deux-roues du jeu sont inventés ; le jeu n'est affilié à aucun constructeur.", "",
         "Voitures (modèles 3D modifiés pour WaForSpeed : allégés, logos effacés) :"]
    l += [f"  {nom} : {cr} — licence {lic} : {LIEN.get(lic, '')}" for nom, cr, lic in lignes_voitures(vs)]
    for titre, entrees in AUTRES:
        l += ["", titre + " :"] + [f"  {nom} : {cr} — licence {lic} : {LIEN[lic]}" for nom, cr, lic in entrees]
    l += ["", "Créés par le jeu :", "  Feuillage, ville, ciel, sons d'effets : dessinés ou calculés par le code de WaForSpeed.",
          "", "Licences complètes : dossier licences/ de l'application.",
          "", "Texte de la licence MIT (goéland et pigeon) :", texte_mit(),
          "", "Texte de la licence Apache 2.0 (android_native_app_glue) :", texte_apache(), ""]
    return "\n".join(l)

def credits_md(vs):
    l = ["# Crédits de WaForSpeed", "",
         "Texte à reprendre dans la fiche des stores (Google Play, App Store) ; le jeu montre le même contenu dans son écran « Crédits » "
         "(`assets/licences/CREDITS.txt`).",
         "Fichier généré par `python3 tools/qa/verifier_licences.py --generer` (WFS-406) : noms inventés de `configgame.json`, auteurs et "
         "licences de `src/game/cars.h` et des fichiers `assets/licences/*.txt`. Aucun nom de marque ni de modèle réel (titres d'origine "
         "gardés seulement dans `assets/licences/`).", "",
         "## Voitures (modèles 3D modifiés pour WaForSpeed : allégés, logos effacés)", "", "| Voiture | Crédit | Licence |", "|---|---|---|"]
    l += [f"| {nom} | {cr} | [{lic}]({LIEN.get(lic, '')}) |" for nom, cr, lic in lignes_voitures(vs)]
    for titre, entrees in AUTRES:
        l += ["", f"## {titre}", ""] + [f"- {nom} : {cr} — licence [{lic}]({LIEN[lic]})" for nom, cr, lic in entrees]
    l += ["", "## Créés par le jeu", "", "- Feuillage, ville, ciel, sons d'effets : dessinés ou calculés par le code de WaForSpeed (aucune licence tierce)", "",
          "## Conséquences des licences", "",
          "- Toutes les licences de l'application (CC0, CC BY, CC BY-SA, MIT, SIL OFL, Apache 2.0) permettent la vente, la publicité et les achats.",
          "- CC BY-SA (un modèle de voiture ; sons « sport » et « gros ») : les versions modifiées sont partagées sous la même licence.",
          "- MIT (goéland) et Apache 2.0 (android_native_app_glue) : texte de la licence donné avec le jeu (`assets/licences/CREDITS.txt`).",
          "- Les noms des voitures sont inventés ; le jeu n'est affilié à aucun constructeur.", ""]
    return "\n".join(l)

def interdits(tout):
    # RG-07 : marques et modèles réels (cars.h, deux-roues), en mots entiers
    mots = set(MARQUES_DEUX_ROUES)
    for v in tout:
        mots.add(v["marque"]); mots.add(v["modele"])
        for t in re.split(r"[\s()]+", v["modele"]):
            if (t.isalpha() and len(t) >= 4 and t.lower() not in GENERIQUES) or (re.search(r"\d", t) and re.search(r"[A-Za-z]", t) and len(t) >= 3):
                mots.add(t)
    return sorted(m for m in mots if m)

def main():
    tout = voitures()
    ret = retirees()
    vs = [v for v in tout if v["id"] not in ret]   # WFS-404 : voitures livrées dans l'APK, seules créditées
    if "--generer" in sys.argv:
        (RACINE / "store" / "credits.md").write_text(credits_md(vs), encoding="utf-8")
        (LIC / "CREDITS.txt").write_text(credits_txt(vs), encoding="utf-8")
        print(f"crédits écrits pour {len(vs)} voitures")
        return 0
    manques = []
    if len(tout) != 20: manques.append(f"{len(tout)} voitures dans cars.h (20 attendues)")
    noms = noms_affiches()
    for v in vs:
        if "NC" in v["credit"] or "ND" in v["credit"]: manques.append(f"{v['id']} : licence non commerciale ou sans modification ({v['credit']})")
        if not auteur_licence(v["credit"])[1]: manques.append(f"{v['id']} : crédit sans auteur ni licence lisible ({v['credit']})")
        if v["id"] not in noms: manques.append(f"{v['id']} : pas de nom inventé dans configgame.json")
    ids = {v["fichier"]: v for v in vs}
    credits = (LIC / "CREDITS.txt").read_text(encoding="utf-8") if (LIC / "CREDITS.txt").is_file() else ""
    nb = 0
    for f in sorted((RACINE / "assets").rglob("*")):
        rel = f.relative_to(RACINE / "assets").as_posix()
        if not f.is_file() or rel.startswith("licences/") or f.name.startswith("."): continue   # fichiers cachés : jamais dans l'APK (aapt)
        nb += 1
        if rel.startswith("cars/") and not rel.startswith("cars/deux_roues/"):
            if f.stem in ret: continue   # WFS-404 : modèle gardé dans le dépôt, hors de l'APK
            v = ids.get(f"cars/{f.stem.removesuffix('-lod')}.wfm")   # WFS-134 : .lod et version allégée = crédit de sa voiture
            if not v: manques.append(f"{rel} : absent de CARS[] (pas de crédit)"); continue
            if auteur_licence(v["credit"])[0] not in credits: manques.append(f"{rel} : auteur absent de CREDITS.txt")
            continue
        if rel.startswith("audio/"):
            if f.name not in credits: manques.append(f"{rel} : absent de CREDITS.txt")
            continue
        regle = next((t for motif, t in COUVERTURE if re.fullmatch(motif, rel)), "?")
        if regle == "?": manques.append(f"{rel} : actif livré sans crédit connu (ajouter sa ligne dans AUTRES et COUVERTURE)")
        elif regle and regle not in credits: manques.append(f"{rel} : « {regle} » absent de CREDITS.txt")
    for nom, attendu in (("store/credits.md", credits_md(vs)), ("assets/licences/CREDITS.txt", credits_txt(vs))):
        p = RACINE / nom
        texte = p.read_text(encoding="utf-8") if p.is_file() else ""
        if texte != attendu: manques.append(f"{nom} : pas à jour (relancer avec --generer)")
        for n, ligne in enumerate(texte.split("\n"), 1):
            if "CC BY-SA 3.0" in ligne: continue   # titres des sons exigés par CC BY-SA 3.0 § 4 c (sans marque)
            for m in interdits(tout):
                if re.search(r"(?<![\w-])" + re.escape(m) + r"(?![\w-])", ligne, re.I):
                    manques.append(f"{nom}:{n} : marque ou modèle réel « {m} »")
    print(f"licences : {len(vs)} voitures livrées ({len(ret)} retirées{' : ' + ', '.join(sorted(ret)) if ret else ''}), "
          f"{nb} fichiers d'assets vérifiés, {len(manques)} manque(s)")
    for m in manques: print("  MANQUE", m)
    return 1 if manques else 0

if __name__ == "__main__":
    sys.exit(main())
