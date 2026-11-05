#!/usr/bin/env python3
# WaForSpeed — vérification croisée des licences (WFS-22 / WFS-25, RG-04) : chaque modèle, son, personnage et police
# livré dans assets/ a sa licence dans assets/licences/, et les crédits des 20 voitures (champ credit de CARS[],
# src/game/cars.h, repris de CAR_MODELS du jeu web) sont complets dans store/credits.md et assets/licences/CREDITS.txt.
#   python3 tools/qa/verifier_licences.py            → vérifie (code de sortie 1 en cas de manque)
#   python3 tools/qa/verifier_licences.py --generer  → réécrit store/credits.md et assets/licences/CREDITS.txt depuis cars.h
import pathlib, re, sys

RACINE = pathlib.Path(__file__).resolve().parents[2]
LIC = RACINE / "assets" / "licences"
# identifiant de la voiture → fichier de licence (dossier du modèle dans le jeu web), quand le nom diffère
ALIAS = {"golf": "golf-8", "m4": "bmw-m4", "p911": "porsche-911", "gallardo": "lambo-gallardo"}

def voitures():
    src = (RACINE / "src" / "game" / "cars.h").read_text(encoding="utf-8")
    motif = re.compile(r'\{"(\w+)",\s*"([^"]*)",\s*"([^"]*)",\s*"(cars/[\w-]+\.wfm)".*?"(«[^"]*)"\}', re.S)
    return [dict(id=m[1], marque=m[2], modele=m[3], fichier=m[4], credit=m[5]) for m in motif.finditer(src)]

def credits_md(vs):
    l = ["# Crédits de WaForSpeed", "",
         "Texte à reprendre dans la fiche des stores (Google Play, App Store) et affiché dans le jeu (garage : crédit de chaque voiture).",
         "Fichier généré par `python3 tools/qa/verifier_licences.py --generer` depuis `src/game/cars.h` (champ `credit`, repris de `CAR_MODELS` du jeu web).", "",
         "## Voitures (modèles 3D, Sketchfab)", "", "| Voiture | Crédit et licence | Fichier de licence |", "|---|---|---|"]
    for v in vs:
        l.append(f"| {v['marque']} {v['modele']} | {v['credit']} | `assets/licences/{ALIAS.get(v['id'], v['id'])}.txt` |")
    l += ["", "## Autres contenus", "",
          "- Personnages (piétons) : « Universal Base Characters » par Quaternius, CC0 1.0 — `assets/licences/personnages-quaternius.txt`",
          "- Sons des moteurs : nhaudio (CC BY), Ivan « qubodup » Gabovitch (CC BY-SA 3.0), mhad (CC0), Marianne Gagnon / SuperTuxKart (CC BY-SA 3.0) — `assets/licences/sons-moteur.txt`",
          "- Police Rajdhani : Indian Type Foundry, SIL Open Font License 1.1 — `assets/licences/police-rajdhani-OFL.txt`",
          "- Feuillage, route, ville, ciel, sons d'effets : générés par le code du jeu (aucune licence tierce)", "",
          "## Conséquences des licences", "",
          "- CC BY-NC et CC BY-NC-SA (T-Cross, X5, A 45, Elantra N, Mustang) : usage non commercial → **le jeu reste gratuit**, sans achat intégré ni publicité.",
          "- CC BY-SA et CC BY-NC-SA (Porsche 911, Mustang, T-Cross, X5 ; sons « sport » et « gros ») : les versions modifiées (modèles allégés `.wfm`, sons bouclés) sont partagées sous la même licence.",
          "- Les noms et logos des constructeurs restent la propriété de leurs titulaires ; le jeu n'est affilié à aucun d'eux.", ""]
    return "\n".join(l)

def credits_txt(vs):
    l = ["WaForSpeed — crédits (inclus dans l'application)", "", "Voitures (modèles 3D, Sketchfab) :"]
    l += [f"  {v['marque']} {v['modele']} : {v['credit']}" for v in vs]
    l += ["", "Personnages : « Universal Base Characters » par Quaternius, CC0 1.0",
          "Sons des moteurs : nhaudio (CC BY), Ivan « qubodup » Gabovitch (CC BY-SA 3.0), mhad (CC0), Marianne Gagnon / SuperTuxKart (CC BY-SA 3.0)",
          "Police Rajdhani : Indian Type Foundry, SIL Open Font License 1.1",
          "Licences complètes : fichiers de ce dossier (licences/). Jeu gratuit (licences non commerciales).", ""]
    return "\n".join(l)

def main():
    vs = voitures()
    if "--generer" in sys.argv:
        (RACINE / "store" / "credits.md").write_text(credits_md(vs), encoding="utf-8")
        (LIC / "CREDITS.txt").write_text(credits_txt(vs), encoding="utf-8")
        print(f"crédits écrits pour {len(vs)} voitures")
        return 0
    manques = []
    if len(vs) != 20: manques.append(f"{len(vs)} voitures dans cars.h (20 attendues)")
    ids = {v["fichier"]: v for v in vs}
    for f in sorted((RACINE / "assets" / "cars").glob("*.wfm")):
        v = ids.get(f"cars/{f.name.replace('-lod.wfm', '.wfm')}")   # WFS-134 : version allégée = crédit de sa voiture
        if not v: manques.append(f"{f.name} : absent de CARS[] (pas de crédit)"); continue
        lic = LIC / f"{ALIAS.get(v['id'], v['id'])}.txt"
        if not lic.is_file() or lic.stat().st_size < 50: manques.append(f"{f.name} : licence {lic.name} absente")
        if "CC" not in v["credit"]: manques.append(f"{f.name} : crédit sans licence ({v['credit']})")
    sons = (LIC / "sons-moteur.txt").read_text(encoding="utf-8") if (LIC / "sons-moteur.txt").is_file() else ""
    for f in sorted((RACINE / "assets" / "audio").glob("*.wav")):
        if f.name not in sons: manques.append(f"{f.name} : absent de sons-moteur.txt")
    for f in sorted((RACINE / "assets" / "people").glob("*.wfp")):
        if not (LIC / "personnages-quaternius.txt").is_file(): manques.append(f"{f.name} : licence des personnages absente")
    for f in sorted((RACINE / "assets" / "fonts").glob("*.ttf")):
        if not (LIC / f"police-{f.stem.split('-')[0].lower()}-OFL.txt").is_file(): manques.append(f"{f.name} : licence de la police absente")
    for nom, attendu in (("store/credits.md", credits_md(vs)), ("assets/licences/CREDITS.txt", credits_txt(vs))):
        p = RACINE / nom
        texte = p.read_text(encoding="utf-8") if p.is_file() else ""
        for v in vs:
            if v["credit"] not in texte: manques.append(f"{nom} : crédit de {v['id']} absent")
        if texte != attendu: manques.append(f"{nom} : pas à jour (relancer avec --generer)")
    nb = len(list((RACINE / "assets").rglob("*.*"))) - len(list(LIC.glob("*")))
    print(f"licences : {len(vs)} voitures, {nb} fichiers d'assets vérifiés, {len(manques)} manque(s)")
    for m in manques: print("  MANQUE", m)
    return 1 if manques else 0

if __name__ == "__main__":
    sys.exit(main())
