#!/usr/bin/env python3
"""Test des scripts des bancs du matin (WFS-384) avec un faux « gh » (aucun appel à GitHub) :
heure_matin.sh (RG-01, RG-03 : un banc par jour quelle que soit l'heure du réveil), etat_captures_matin.sh, retard_matin.sh et
recette_prete.sh (RG-06 : jamais « Recette QA prête » sans les captures du jour).
Le faux gh répond aux « gh api <chemin> [--jq …] » avec les données JSON du cas (filtre --jq appliqué par jq -r), note les
écritures (-X POST) dans un fichier, et ne fait aucun réseau. Usage : python3 tools/qa/test_matin.py (code 0 si tout est vert).
"""
import json
import os
import subprocess
import sys
import tempfile
from datetime import datetime, timezone
from pathlib import Path

QA = Path(__file__).resolve().parent
FAUX_GH = r'''#!/usr/bin/env python3
import json, os, subprocess, sys
a = sys.argv[1:]
if not a or a[0] != "api":
    sys.exit(f"faux gh : commande inattendue {a}")
pos, i = [], 1
while i < len(a):   # options à valeur sautées ; le premier argument restant est le chemin
    if a[i] in ("-X", "--jq", "-f", "-F", "--method"):
        i += 2
        continue
    if not a[i].startswith("-"):
        pos.append(a[i])
    i += 1
chemin = pos[0]
methode = a[a.index("-X") + 1] if "-X" in a else "GET"
with open(os.environ["FAUX_GH_JOURNAL"], "a", encoding="utf-8") as j:
    j.write(json.dumps({"methode": methode, "chemin": chemin, "args": a}) + "\n")
if methode != "GET":
    sys.exit(0)
donnees = json.load(open(os.environ["FAUX_GH_DONNEES"], encoding="utf-8"))
cle = next((k for k in sorted(donnees, key=len, reverse=True) if chemin.split("?")[0].endswith(k)), None)
if cle is None:
    sys.exit(f"faux gh : aucune donnée pour {chemin}")
texte = json.dumps(donnees[cle])
if "--jq" in a:
    r = subprocess.run(["jq", "-r", a[a.index("--jq") + 1]], input=texte, capture_output=True, text=True)
    sys.stdout.write(r.stdout); sys.exit(r.returncode)
sys.stdout.write(texte)
'''

ECHECS = []


def verifier(nom, condition, detail=""):
    print(("ok    " if condition else "ÉCHEC ") + nom + ("" if condition else f" — {detail}"))
    if not condition:
        ECHECS.append(nom)


def epoch(texte):   # « 2026-11-10T05:30:00Z » → secondes
    return int(datetime.strptime(texte, "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=timezone.utc).timestamp())


class Bac:
    """Dossier temporaire : faux gh dans le PATH, données, journal des appels."""

    def __init__(self):
        self.d = Path(tempfile.mkdtemp())
        (self.d / "bin").mkdir()
        gh = self.d / "bin" / "gh"
        gh.write_text(FAUX_GH, encoding="utf-8")
        gh.chmod(0o755)
        self.journal = self.d / "appels.jsonl"

    def lancer(self, script, donnees, env):
        (self.d / "donnees.json").write_text(json.dumps(donnees), encoding="utf-8")
        self.journal.write_text("", encoding="utf-8")
        sortie = self.d / "sortie"
        sortie.write_text("", encoding="utf-8")
        e = dict(os.environ, PATH=f"{self.d / 'bin'}:{os.environ['PATH']}", FAUX_GH_DONNEES=str(self.d / "donnees.json"),
                 FAUX_GH_JOURNAL=str(self.journal), GITHUB_OUTPUT=str(sortie), GH_REPO="exemple/depot", RUN_ID="999",
                 RUN_NUMBER="12", GITHUB_SHA="abcdef1234567", GITHUB_SERVER_URL="https://github.com")
        e.update(env)
        r = subprocess.run(["bash", str(QA / script)], env=e, capture_output=True, text=True)
        sorties = dict(l.split("=", 1) for l in sortie.read_text(encoding="utf-8").splitlines() if "=" in l)
        appels = [json.loads(l) for l in self.journal.read_text(encoding="utf-8").splitlines() if l]
        return r, sorties, appels


def run(id_, cree, etat="completed", duree=10):
    debut = epoch(cree)
    fin = datetime.fromtimestamp(debut + duree, timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    return {"id": id_, "created_at": cree, "status": etat, "run_started_at": cree, "updated_at": fin}


def jobs(compilation):
    return {"jobs": [{"name": "heure", "conclusion": "success", "status": "completed"}] +
            ([{"name": "compilation", "conclusion": compilation, "status": "completed"}] if compilation else [])}


def test_heure(bac):
    s = "heure_matin.sh"
    # lancement à la main : toujours, même à 02:00 heure de Paris et même si un banc a déjà tourné
    r, o, _ = bac.lancer(s, {}, {"EVENEMENT": "workflow_dispatch", "MAINTENANT": str(epoch("2026-11-10T01:00:00Z"))})
    verifier("à la main : lancé", o.get("lancer") == "oui" and o.get("retard") == "non", r.stdout + r.stderr)
    # réveil avant 04:00 à Paris (02:30 UTC = 03:30 en hiver) : rien
    r, o, _ = bac.lancer(s, {}, {"EVENEMENT": "schedule", "MAINTENANT": str(epoch("2026-11-10T02:30:00Z"))})
    verifier("réveil à 03:30 Paris : rien", o.get("lancer") == "non", r.stdout + r.stderr)
    # RG-01 : premier réveil du jour à 15:23 Paris (avant WFS-384 : sauté car après 10:59) → lancé, en retard
    donnees = {"/actions/workflows/matin.yml/runs": {"workflow_runs": [run(998, "2026-11-09T16:10:00Z", duree=4000)]},
               "/actions/runs/998/jobs": jobs("success")}
    r, o, ap = bac.lancer(s, donnees, {"EVENEMENT": "schedule", "MAINTENANT": str(epoch("2026-11-10T14:23:00Z"))})
    verifier("RG-01 : premier réveil à 15:23 Paris, banc de la veille : lancé en retard",
             o.get("lancer") == "oui" and o.get("retard") == "oui", r.stdout + r.stderr)
    verifier("RG-01 : run de la veille ignoré sans lire ses jobs", not any("/runs/998/jobs" in a["chemin"] for a in ap), str(ap))
    # réveil à 05:23 Paris sans banc ce jour-là : lancé, pas en retard ; un réveil sauté du jour (10 s) n'est pas lu
    donnees = {"/actions/workflows/matin.yml/runs": {"workflow_runs": [run(997, "2026-11-10T03:05:00Z", duree=9)]},
               "/actions/runs/997/jobs": jobs("skipped")}
    r, o, ap = bac.lancer(s, donnees, {"EVENEMENT": "schedule", "MAINTENANT": str(epoch("2026-11-10T04:23:00Z"))})
    verifier("réveil à 05:23 Paris : lancé, pas en retard", o.get("lancer") == "oui" and o.get("retard") == "non", r.stdout + r.stderr)
    verifier("réveil sauté de 10 s : jobs non lus (quota)", not any("/runs/997/jobs" in a["chemin"] for a in ap), str(ap))
    # RG-03 : banc complet déjà passé ce jour-là (lancé à la main à 01:34 Paris) → rien, même à 18:23
    donnees = {"/actions/workflows/matin.yml/runs": {"workflow_runs": [run(996, "2026-11-10T00:34:00Z", duree=4700)]},
               "/actions/runs/996/jobs": jobs("success")}
    r, o, _ = bac.lancer(s, donnees, {"EVENEMENT": "schedule", "MAINTENANT": str(epoch("2026-11-10T17:23:00Z"))})
    verifier("RG-03 : banc déjà passé aujourd'hui : rien", o.get("lancer") == "non", r.stdout + r.stderr)
    # RG-03 : banc du jour encore en cours (compilation en cours) → rien
    donnees = {"/actions/workflows/matin.yml/runs": {"workflow_runs": [run(995, "2026-11-10T03:00:00Z", etat="in_progress", duree=600)]},
               "/actions/runs/995/jobs": {"jobs": [{"name": "compilation", "conclusion": None, "status": "in_progress"}]}}
    r, o, _ = bac.lancer(s, donnees, {"EVENEMENT": "schedule", "MAINTENANT": str(epoch("2026-11-10T04:23:00Z"))})
    verifier("RG-03 : banc du jour en cours : rien", o.get("lancer") == "non", r.stdout + r.stderr)
    # banc du jour en échec à la compilation : il a tourné, pas de 2e banc (jamais deux fois le même jour)
    donnees = {"/actions/workflows/matin.yml/runs": {"workflow_runs": [run(994, "2026-11-10T03:00:00Z", duree=900)]},
               "/actions/runs/994/jobs": jobs("failure")}
    r, o, _ = bac.lancer(s, donnees, {"EVENEMENT": "schedule", "MAINTENANT": str(epoch("2026-11-10T06:23:00Z"))})
    verifier("banc du jour rouge : pas de 2e banc", o.get("lancer") == "non", r.stdout + r.stderr)
    # liste des runs illisible : code 1 (run rouge, jamais un banc lancé au hasard)
    r, o, _ = bac.lancer(s, {}, {"EVENEMENT": "schedule", "MAINTENANT": str(epoch("2026-11-10T06:23:00Z"))})
    verifier("liste illisible : rouge", r.returncode == 1 and "lancer" not in o, r.stdout + r.stderr)


def ecrire_jour(bac, texte):
    f = bac.d / "jour.txt"
    f.write_text(texte, encoding="utf-8")
    return str(f)


def test_etat(bac):
    m = str(epoch("2026-11-10T09:00:00Z"))
    r, _, _ = bac.lancer("etat_captures_matin.sh", {}, {"JOUR_TXT": "absent", "MAINTENANT": m})
    verifier("état : Release absente → code 2, ancienne Release donnée",
             r.returncode == 2 and "apercu-ancien-20261109-0925" in r.stdout, r.stdout + r.stderr)
    f = ecrire_jour(bac, "Captures des bancs du matin — run 12, commit abc, 2026-11-09 02:50 heure de Paris\n")
    r, _, _ = bac.lancer("etat_captures_matin.sh", {}, {"JOUR_TXT": f, "MAINTENANT": m})
    verifier("état : captures de la veille → code 1", r.returncode == 1 and "2026-11-09 02:50" in r.stdout, r.stdout + r.stderr)
    f = ecrire_jour(bac, "Captures des bancs du matin — run 12, commit abc, 2026-11-10 05:31 heure de Paris\n")
    r, _, _ = bac.lancer("etat_captures_matin.sh", {}, {"JOUR_TXT": f, "MAINTENANT": m})
    verifier("état : captures du jour → code 0", r.returncode == 0 and "captures du jour" in r.stdout, r.stdout + r.stderr)


def corps_poste(appels):
    for a in appels:
        if a["methode"] == "POST" and a["chemin"].endswith("/comments"):
            return next(x[5:] for x in a["args"] if x.startswith("body="))
    return None


def test_messages(bac):
    issues = {"/issues": [{"number": 242, "title": "🧪 Recette QA du matin"}], "/issues/242/comments": [],
              "/actions/workflows/matin.yml/runs": {"workflow_runs": []}}
    resultats = "compilation : success\npublication des captures : success"
    m = str(epoch("2026-11-10T09:00:00Z"))
    # RG-06 : captures du jour absentes → jamais « Recette QA prête »
    r, _, ap = bac.lancer("recette_prete.sh", issues, {"JOUR_TXT": "absent", "MAINTENANT": m, "RESULTATS": resultats})
    c = corps_poste(ap) or ""
    verifier("recette : sans captures du jour, pas « Recette QA prête »",
             "Recette QA prête" not in c and "captures du jour manquantes" in c and "apercu-ancien" in c, r.stdout + r.stderr + c)
    f = ecrire_jour(bac, "Captures des bancs du matin — run 12, commit abc, 2026-11-10 05:31 heure de Paris\n")
    r, _, ap = bac.lancer("recette_prete.sh", issues, {"JOUR_TXT": f, "MAINTENANT": m, "RESULTATS": resultats})
    c = corps_poste(ap) or ""
    verifier("recette : captures du jour → « Recette QA prête »",
             "Recette QA prête" in c and "manquantes" not in c and "releases/tag/captures-matin" in c, r.stdout + r.stderr + c)
    # retard : message posté, sans « Recette QA prête », avec l'heure prévue (+1 h 30 : 10:00 Paris → 11:30)
    r, _, ap = bac.lancer("retard_matin.sh", issues, {"JOUR_TXT": "absent", "MAINTENANT": m})
    c = corps_poste(ap) or ""
    verifier("retard : message posté sans « Recette QA prête »",
             "Bancs du matin en retard" in c and "Recette QA prête" not in c and "11:30" in c, r.stdout + r.stderr + c)
    # retard déjà annoncé pour ce run : rien de plus
    deja = dict(issues, **{"/issues/242/comments": [{"body": "x <!-- wfs-retard-matin-12 -->"}]})
    r, _, ap = bac.lancer("retard_matin.sh", deja, {"JOUR_TXT": "absent", "MAINTENANT": m})
    verifier("retard : pas de doublon", corps_poste(ap) is None and r.returncode == 0, r.stdout + r.stderr)


if __name__ == "__main__":
    b = Bac()
    test_heure(b)
    test_etat(b)
    test_messages(b)
    print(f"\n{'ÉCHEC : ' + ', '.join(ECHECS) if ECHECS else 'tout est vert'}")
    sys.exit(1 if ECHECS else 0)
