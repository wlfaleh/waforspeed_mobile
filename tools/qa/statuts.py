#!/usr/bin/env python3
"""WaForSpeed — statuts automatiques des tickets (WFS-196, épic E7).

À la fin de chaque run de build.yml sur main (workflow statuts.yml), passe en « ✅ Terminé (CI main #N) » les tickets
🟣 Fusionné de TACHES.md dont le commit de fusion est contenu dans le commit du run (RG-02), note au journal un run rouge
sans rien changer (RG-03) et liste pour le PO les lignes qu'il ne sait pas lire, sans jamais deviner (RG-04).

Python 3, bibliothèque standard seulement (RG-07). Ne modifie que TACHES.md, track-agent.md et journal.md (RG-06).

WFS-236 : un run de main « cancelled » dont seuls les jobs publication-* ont été annulés, avant d'avoir démarré, et dont
tous les autres jobs sont verts compte comme vert (--jobs-json : liste des jobs du run, telle que la rend l'API GitHub).

Usage :
  python3 tools/qa/statuts.py --run-number 812 --sha <commit du run> --conclusion success [--evenement push]
                              [--jobs-en-echec "endurance,android"] [--jobs-json jobs.json] [--depot .]
                              [--date "AAAA-MM-JJ hh:mm:ss"]
Code de sortie : 0 (fichiers modifiés ou non) ; 2 : erreur d'utilisation.
"""
import argparse
import datetime
import json
import re
import subprocess
import sys
from pathlib import Path

COLONNES = 10                 # | Priorité | Clé | Titre | Épic | Statut | Responsable | Dépend de | Branche | PR | QA |
COL_CLE, COL_STATUT, COL_PR = 1, 4, 8
VIOLET, VERT = "🟣", "✅"
TITRE_TABLEAU = "| Priorité | Clé | Titre | Épic | Statut |"
RE_CLE = re.compile(r"^WFS-\d+$")
RE_PR = re.compile(r"#(\d+)\b")
RE_HASH = re.compile(r"\b[0-9a-f]{7,40}\b")
AUTEUR = "robot statuts"


def statut_termine(run_number):
    return f"{VERT} Terminé (CI main #{run_number})"


# ---------------------------------------------------------------- git (lecture seule)
class Git:
    def __init__(self, depot):
        self.depot = str(depot)
        self._sujets = None

    def _git(self, *args):
        return subprocess.run(["git", "-C", self.depot, *args], capture_output=True, text=True)

    def commit_existe(self, ref):
        return self._git("cat-file", "-e", f"{ref}^{{commit}}").returncode == 0

    def complet(self, ref):
        r = self._git("rev-parse", "--verify", "--quiet", f"{ref}^{{commit}}")
        return r.stdout.strip() if r.returncode == 0 else None

    def est_ancetre(self, commit, sha):
        return self._git("merge-base", "--is-ancestor", commit, sha).returncode == 0

    def commit_de_pr(self, pr):
        """Commit de fusion de la PR #pr : squash « … (#pr) » ou « Merge pull request #pr from ». Cherché dans tout le dépôt
        (pas seulement l'historique du run) : une PR fusionnée après le run est trouvée, puis écartée par est_ancetre."""
        if self._sujets is None:
            r = self._git("log", "--all", "--format=%H%x09%s")
            self._sujets = [l.split("\t", 1) for l in r.stdout.splitlines() if "\t" in l] if r.returncode == 0 else []
        squash = re.compile(rf"\(#{pr}\)")
        fusion = re.compile(rf"^Merge pull request #{pr}\b")
        for h, sujet in self._sujets:
            if squash.search(sujet) or fusion.search(sujet):
                return h
        return None


# ---------------------------------------------------------------- lecture des lignes
def cellules(ligne):
    """Cases d'une ligne de tableau Markdown (sans les bords) ; None si ce n'est pas une ligne de tableau."""
    s = ligne.rstrip("\n")
    if not (s.startswith("|") and s.rstrip().endswith("|")):
        return None
    return [c.strip() for c in s.strip()[1:-1].split("|")]


def remplacer_case(ligne, index, nouveau):
    """Remplace le texte de la case `index` en gardant tout le reste de la ligne à l'identique."""
    morceaux = ligne.rstrip("\n").split("|")   # morceaux[0] = avant le 1er « | », morceaux[index + 1] = la case
    morceaux[index + 1] = f" {nouveau} "
    return "|".join(morceaux) + ("\n" if ligne.endswith("\n") else "")


def commit_de_fusion(cases, git, sha):
    """Commit de fusion d'une ligne 🟣 : PR de la case PR, sinon « #NNN » de la case Statut, sinon hash cité ; None si introuvable."""
    statut, pr_case = cases[COL_STATUT], cases[COL_PR]
    prs = RE_PR.findall(pr_case) or RE_PR.findall(statut)
    for pr in prs:
        c = git.commit_de_pr(pr)
        if c:
            return c, f"PR #{pr}"
    for h in RE_HASH.findall(statut):
        c = git.complet(h)
        if c:
            return c, f"commit {h}"
    return None, ("PR " + ", ".join("#" + p for p in prs) + " introuvable dans le dépôt") if prs else "ni PR ni commit cité"


# ---------------------------------------------------------------- run annulé à la publication (WFS-236)
PREFIXE_PUBLICATION = "publication-"


def lire_jobs(texte):
    """Jobs d'un run depuis la réponse de l'API (objet {"jobs": [...]}, liste, ou un objet JSON par ligne : gh api --paginate
    --jq '.jobs[]'). None si le texte est vide ou illisible (RG-05 d)."""
    texte = (texte or "").strip()
    if not texte:
        return None
    try:
        donnees = json.loads(texte)
    except ValueError:
        try:
            donnees = [json.loads(l) for l in texte.splitlines() if l.strip()]
        except ValueError:
            return None
    if isinstance(donnees, dict):
        donnees = donnees.get("jobs")
    if not isinstance(donnees, list) or not all(isinstance(j, dict) and isinstance(j.get("name"), str) for j in donnees):
        return None
    return donnees


def publication_jamais_demarree(job):
    """RG-02 bis : job publication-* annulé avant de démarrer : aucune étape, ou toutes ses étapes sautées (« skipped ») ou
    jamais lancées (« queued » / « pending » sans conclusion). Une étape lancée (en cours, réussie, en échec, annulée) → False."""
    if job.get("conclusion") not in ("cancelled", "skipped"):
        return False
    for e in job.get("steps") or []:
        if not isinstance(e, dict):
            return False
        if e.get("conclusion") == "skipped":
            continue
        if e.get("status") in ("queued", "pending", "waiting") and e.get("conclusion") is None:
            continue
        return False
    return True


def annule_seulement_a_la_publication(jobs):
    """RG-02 : vrai si tous les jobs hors publication-* sont « success » et que chaque publication-* est réussie ou annulée
    avant d'avoir démarré (RG-02 bis), avec au moins une publication annulée et au moins un job de construction ; sinon
    (job annulé, sauté, en échec, liste vide ou illisible) : faux (RG-03)."""
    if not jobs:
        return False
    construction = [j for j in jobs if not j["name"].startswith(PREFIXE_PUBLICATION)]
    publications = [j for j in jobs if j["name"].startswith(PREFIXE_PUBLICATION)]
    if not construction or any(j.get("conclusion") != "success" for j in construction):
        return False
    annulees = 0
    for j in publications:
        if j.get("conclusion") == "success":
            continue
        if not publication_jamais_demarree(j):
            return False
        annulees += 1
    return annulees > 0


# ---------------------------------------------------------------- traitement
class Resultat:
    def __init__(self):
        self.termines = []      # (clé, preuve)
        self.violets_du_run = []  # tickets 🟣 contenus dans le run (run rouge)
        self.a_corriger = []    # (fichier, ligne n°, raison) : RG-04
        self.taches = None      # nouveau texte de TACHES.md (None : inchangé)
        self.agents = None      # nouveau texte de track-agent.md (None : inchangé)


def traiter(taches, agents, git, sha, run_number, vert):
    """Calcule les changements (sans écrire). vert : run réussi → ✅ ; sinon seulement la liste des 🟣 contenus."""
    R = Resultat()
    lignes = taches.splitlines(keepends=True)
    dans_tableau = False
    for n, ligne in enumerate(lignes, 1):
        if ligne.startswith(TITRE_TABLEAU):
            dans_tableau = True
            continue
        if dans_tableau and not ligne.startswith("|"):
            dans_tableau = False
        if not dans_tableau or ligne.startswith("|---"):
            continue
        cases = cellules(ligne)
        if cases is None:
            continue
        if len(cases) != COLONNES:
            if VIOLET in ligne:   # RG-04 : on ne devine pas quelle case est le statut
                R.a_corriger.append(("TACHES.md", n, f"{len(cases)} cases au lieu de {COLONNES}"))
            continue
        cle, statut = cases[COL_CLE], cases[COL_STATUT]
        if not statut.startswith(VIOLET):
            # RG-04 : un « 🟣 Fusionné » écrit dans une autre case que Statut (la case Dépend de peut citer « WFS-XX 🟣 »)
            if any(c.startswith(VIOLET + " Fusionné") for i, c in enumerate(cases) if i != COL_STATUT):
                R.a_corriger.append(("TACHES.md", n, f"{cle} : « 🟣 Fusionné » hors de la case Statut"))
            continue
        if not RE_CLE.match(cle):
            R.a_corriger.append(("TACHES.md", n, f"clé illisible « {cle} »"))
            continue
        commit, preuve = commit_de_fusion(cases, git, sha)
        if not commit:
            R.a_corriger.append(("TACHES.md", n, f"{cle} : {preuve}"))
            continue
        if not git.est_ancetre(commit, sha):
            continue   # pas encore dans ce run : le run suivant tranchera
        if not vert:
            R.violets_du_run.append(cle)
            continue
        lignes[n - 1] = remplacer_case(ligne, COL_STATUT, statut_termine(run_number))
        R.termines.append((cle, f"{preuve}, {commit[:7]}"))
    if not vert or not R.termines:
        return R
    cles = {c for c, _ in R.termines}
    # ligne de détail « **WFS-XX — titre** · épic · priorité · 🟣 … — responsable… » : le statut va de 🟣 au premier « — »
    for n, ligne in enumerate(lignes, 1):
        m = re.match(r"^\*\*(WFS-\d+) — ", ligne)
        if not m or m.group(1) not in cles:
            continue
        i = ligne.find(" · " + VIOLET)
        if i < 0:
            continue
        debut = i + 3
        fin = ligne.find(" — ", debut)
        if fin < 0:
            R.a_corriger.append(("TACHES.md", n, f"{m.group(1)} : ligne de détail sans « — » après le statut"))
            continue
        lignes[n - 1] = ligne[:debut] + statut_termine(run_number) + ligne[fin:]
    R.taches = "".join(lignes)
    if agents is not None:
        R.agents = maj_agents(agents, cles, run_number, R)
    return R


def maj_agents(agents, cles, run_number, R):
    """track-agent.md : dans la case « État », « WFS-XX (🟣 …) » devient « WFS-XX (✅ CI main #N) » ; autre forme : listée."""
    lignes = agents.splitlines(keepends=True)
    for n, ligne in enumerate(lignes, 1):
        cases = cellules(ligne)
        if cases is None or len(cases) != 5 or ligne.startswith("|---"):
            continue
        etat = cases[4]
        nouveau = etat
        for cle in cles:
            if cle not in etat:
                continue
            motif = re.compile(rf"{re.escape(cle)} \({VIOLET}[^()]*\)")
            if motif.search(nouveau):
                nouveau = motif.sub(f"{cle} ({VERT} CI main #{run_number})", nouveau)
            elif re.search(rf"{re.escape(cle)}\b[^;]*{VIOLET}", nouveau):
                R.a_corriger.append(("track-agent.md", n, f"{cle} : forme de l'État non reconnue"))
        if nouveau != etat:
            lignes[n - 1] = remplacer_case(ligne, 4, nouveau)
    return "".join(lignes)


def entree_journal(R, date, run_number, sha, conclusion, jobs_echec, evenement, publication_annulee=False):
    """RG-03 / RG-05 : une entrée au journal par run traité (None : rien à écrire). publication_annulee (WFS-236, RG-02) :
    run « cancelled » traité comme vert, annulé seulement à la publication."""
    lignes = []
    if conclusion == "success":
        if not R.termines and not R.a_corriger:
            return None
        nature = "Run vert, annulé seulement à la publication" if publication_annulee else "Run vert"
        lignes.append(f"- {nature} ({evenement}, commit {sha[:7]}) : " + (
            ", ".join(f"{c} ✅ ({p})" for c, p in R.termines) if R.termines else "aucun ticket 🟣 à passer en ✅") + ".")
    else:
        lignes.append(f"- Run rouge ({evenement}, commit {sha[:7]}), aucun statut changé. Job(s) en échec : "
                      + (", ".join(jobs_echec) if jobs_echec else "non précisés") + ".")
        lignes.append("- Tickets 🟣 contenus dans ce run : " + (", ".join(R.violets_du_run) if R.violets_du_run else "aucun") + ".")
    for f, n, raison in R.a_corriger:
        lignes.append(f"- À corriger par le PO : `{f}` l. {n} : {raison} (ligne laissée telle quelle).")
    return f"\n### {date} — Statuts automatiques, CI main #{run_number} ({AUTEUR})\n" + "\n".join(lignes) + "\n"


def main(argv=None):
    p = argparse.ArgumentParser(description="Statuts automatiques des tickets (WFS-196)")
    p.add_argument("--run-number", required=True, type=int)
    p.add_argument("--sha", required=True)
    p.add_argument("--conclusion", required=True)
    p.add_argument("--evenement", default="push")
    p.add_argument("--jobs-en-echec", default="")
    p.add_argument("--jobs-json", default=None, help="WFS-236 : fichier des jobs du run (API GitHub), pour un run annulé")
    p.add_argument("--depot", default=".")
    p.add_argument("--date", default=None)
    a = p.parse_args(argv)
    depot = Path(a.depot)
    git = Git(depot)
    if not git.commit_existe(a.sha):
        print(f"statuts : commit du run {a.sha} absent du dépôt (historique complet requis)", file=sys.stderr)
        return 2
    # RG-03 : annulé, sauté… : rien. Run de nuit (schedule) : seul le banc d'endurance tourne, ce n'est pas une CI complète :
    # un vert de nuit ne passe rien en ✅ ; un rouge de nuit est noté au journal.
    rouge = a.conclusion in ("failure", "timed_out", "startup_failure")
    vert = a.conclusion == "success" and a.evenement != "schedule"
    # WFS-236 (RG-01, RG-02) : run annulé (hors nuit) dont seules les publications ont été annulées avant de démarrer → vert
    publication_annulee = False
    if a.conclusion == "cancelled" and a.evenement != "schedule" and a.jobs_json:
        try:
            texte_jobs = Path(a.jobs_json).read_text(encoding="utf-8")
        except OSError:
            texte_jobs = ""
        publication_annulee = annule_seulement_a_la_publication(lire_jobs(texte_jobs))
        vert = publication_annulee
        print(f"statuts : run #{a.run_number} annulé : " + ("seulement à la publication, traité comme vert" if vert
              else "un job de construction ou de test n'est pas vert, ou liste des jobs illisible : rien"))
    if not rouge and not vert:
        print(f"statuts : run #{a.run_number} {a.conclusion} ({a.evenement}) : rien à faire")
        return 0
    taches_f, agents_f, journal_f = depot / "TACHES.md", depot / "track-agent.md", depot / "journal.md"
    taches = taches_f.read_text(encoding="utf-8")
    agents = agents_f.read_text(encoding="utf-8") if agents_f.exists() else None
    R = traiter(taches, agents, git, a.sha, a.run_number, vert)
    date = a.date or datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%d %H:%M:%S")
    jobs = [j.strip() for j in a.jobs_en_echec.split(",") if j.strip()]
    entree = entree_journal(R, date, a.run_number, a.sha, "success" if vert else "failure", jobs, a.evenement, publication_annulee)
    if R.taches is not None and R.taches != taches:
        taches_f.write_text(R.taches, encoding="utf-8")
    if R.agents is not None and agents is not None and R.agents != agents:
        agents_f.write_text(R.agents, encoding="utf-8")
    if entree:
        texte = journal_f.read_text(encoding="utf-8") if journal_f.exists() else ""
        journal_f.write_text(texte.rstrip("\n") + "\n" + entree, encoding="utf-8")
    print(f"statuts : run #{a.run_number} {'vert' if vert else 'rouge'} : {len(R.termines)} ticket(s) en ✅"
          f" ({', '.join(c for c, _ in R.termines) or '-'}), {len(R.a_corriger)} ligne(s) à corriger par le PO")
    return 0


if __name__ == "__main__":
    sys.exit(main())
