#!/usr/bin/env python3
"""Test de tools/qa/statuts.py (WFS-196, RG-07) sur un dépôt git d'exemple créé dans un dossier temporaire.

Cas : ligne 🟣 contenue dans le run → ✅ (tableau, ligne de détail, track-agent.md) ; 🟣 fusionnée après le run → inchangée ;
ligne mal formée → inchangée et listée au journal ; run rouge → aucun statut changé, entrée au journal ; run annulé → rien.
Usage : python3 tools/qa/test_statuts.py   (code de sortie 0 si tout est vert)
"""
import subprocess
import sys
import tempfile
from pathlib import Path

ICI = Path(__file__).resolve().parent
sys.path.insert(0, str(ICI))
import statuts  # noqa: E402

ENTETE = "| Priorité | Clé | Titre | Épic | Statut | Responsable | Dépend de | Branche | PR | QA |\n|---|---|---|---|---|---|---|---|---|---|\n"
TACHES = (
    "# Tâches\n\n" + ENTETE +
    "| P2 Haute | WFS-11 | Ticket fusionné dans le run | E3 | 🟣 Fusionné (PR #11, fusion par confiance) | agent 2 | WFS-10 ✅ | `feature/waforspeed-11` | #11 |  |\n"
    "| P2 Haute | WFS-12 | Ticket fusionné après le run | E2 | 🟣 Fusionné (PR #12) | agent 1 | WFS-11 🟣 | `feature/waforspeed-12` | #12 |  |\n"
    "| P2 Haute | WFS-13 | Ligne mal formée | E4 | 🟣 Fusionné (PR #11) | agent 3 | — | `feature/waforspeed-13` |\n"
    "| P3 Moyenne | WFS-14 | Statut dans la mauvaise case | E5 | agent 4 | 🟣 Fusionné (PR #11) | — | `feature/waforspeed-14` |  |  |\n"
    "| P3 Moyenne | WFS-15 | En cours | E6 | 🔧 En cours | agent 5 | — | `feature/waforspeed-15` |  |  |\n"
    "| P3 Moyenne | WFS-16 | PR inconnue | E8 | 🟣 Fusionné (PR #99) | agent 6 | — | `feature/waforspeed-16` | #99 |  |\n"
    "\n**WFS-11 — Ticket fusionné dans le run** · E3 · P3 Moyenne · 🟣 Fusionné (PR #11, squash) — agent 2, branche `feature/waforspeed-11`\n"
    "**WFS-12 — Ticket fusionné après le run** · E2 · P2 Haute · 🟣 Fusionné (PR #12) — agent 1\n"
)
AGENTS = (
    "| Agent (session) | Ticket en cours | Ce qu'il fait | Branche, dernier envoi (UTC) | État |\n|---|---|---|---|---|\n"
    "| agent 2 |  |  | 11 : fusionnée 21:13 | ✅ libre ; suit la CI de `main` de WFS-11 (🟣) |\n"
    "| agent 1 |  |  | 12 : fusionnée 21:20 | ✅ libre ; suit la CI de `main` de WFS-12 (🟣) |\n"
)

echecs = 0


def verifie(cond, quoi):
    global echecs
    print(f"statuts : {quoi:<80} → {'OK' if cond else 'ÉCHEC'}")
    if not cond:
        echecs += 1


def git(d, *args):
    return subprocess.run(["git", "-C", str(d), *args], check=True, capture_output=True, text=True).stdout.strip()


def depot(d):
    git(d, "init", "-q", "-b", "main")
    git(d, "config", "user.email", "test@exemple.invalid"); git(d, "config", "user.name", "test")
    (d / "TACHES.md").write_text(TACHES, encoding="utf-8")
    (d / "track-agent.md").write_text(AGENTS, encoding="utf-8")
    (d / "journal.md").write_text("# Journal\n", encoding="utf-8")
    git(d, "add", "-A"); git(d, "commit", "-q", "-m", "départ")
    (d / "a.txt").write_text("a"); git(d, "add", "-A")
    git(d, "commit", "-q", "-m", "WFS-11 : ticket du run (#11) (agent 2)")
    run = git(d, "rev-parse", "HEAD")
    (d / "b.txt").write_text("b"); git(d, "add", "-A")
    git(d, "commit", "-q", "-m", "WFS-12 : ticket fusionné après le run (#12) (agent 1)")
    return run


def lancer(d, run, conclusion, numero=812, jobs=""):
    return statuts.main(["--run-number", str(numero), "--sha", run, "--conclusion", conclusion, "--depot", str(d),
                         "--date", "2026-11-06 22:00:00", "--jobs-en-echec", jobs])


def main():
    with tempfile.TemporaryDirectory() as t:
        d = Path(t); run = depot(d)
        # run vert
        verifie(lancer(d, run, "success") == 0, "run vert : sortie 0")
        T = (d / "TACHES.md").read_text(encoding="utf-8")
        A = (d / "track-agent.md").read_text(encoding="utf-8")
        J = (d / "journal.md").read_text(encoding="utf-8")
        l11 = next(l for l in T.splitlines() if "| WFS-11 |" in l)
        verifie("| ✅ Terminé (CI main #812) | agent 2 | WFS-10 ✅ |" in l11, "WFS-11 (contenu dans le run) : case Statut seule → ✅ Terminé (CI main #812)")
        verifie(l11.count("|") == 11 and "Ticket fusionné dans le run" in l11, "WFS-11 : le reste de la ligne est inchangé")
        verifie("· ✅ Terminé (CI main #812) — agent 2, branche" in T, "WFS-11 : ligne de détail passée en ✅")
        verifie("| 🟣 Fusionné (PR #12) | agent 1 | WFS-11 🟣 |" in T and "· 🟣 Fusionné (PR #12) — agent 1" in T,
                "WFS-12 (fusionné après le run) : inchangé, y compris « WFS-11 🟣 » de Dépend de")
        verifie("| WFS-13 | Ligne mal formée | E4 | 🟣 Fusionné (PR #11) |" in T and "l. 7 : 8 cases au lieu de 10" in J,
                "ligne mal formée (8 cases) : inchangée et listée au journal")
        verifie("agent 4 | 🟣 Fusionné (PR #11) |" in T and "WFS-14 : « 🟣 Fusionné » hors de la case Statut" in J,
                "statut dans la mauvaise case : inchangé et listé")
        verifie("| 🟣 Fusionné (PR #99) |" in T and "WFS-16 : PR #99 introuvable" in J, "PR introuvable : inchangée et listée")
        verifie("| 🔧 En cours |" in T, "ticket en cours : inchangé")
        verifie("WFS-11 (✅ CI main #812)" in A and "WFS-12 (🟣)" in A, "track-agent.md : État de WFS-11 → ✅, WFS-12 inchangé")
        verifie("### 2026-11-06 22:00:00 — Statuts automatiques, CI main #812 (robot statuts)" in J and "WFS-11 ✅ (PR #11" in J,
                "journal : entrée datée du run, tickets passés en ✅")
        # second passage sur le même run : plus rien à changer, pas de double ✅
        avant = (d / "TACHES.md").read_text(encoding="utf-8")
        lancer(d, run, "success")
        verifie((d / "TACHES.md").read_text(encoding="utf-8") == avant, "même run relancé : TACHES.md inchangé")
    with tempfile.TemporaryDirectory() as t:
        d = Path(t); run = depot(d)
        lancer(d, run, "failure", 813, "endurance, android")
        T = (d / "TACHES.md").read_text(encoding="utf-8")
        J = (d / "journal.md").read_text(encoding="utf-8")
        verifie(T == TACHES and (d / "track-agent.md").read_text(encoding="utf-8") == AGENTS, "run rouge : aucun statut changé")
        verifie("CI main #813 (robot statuts)" in J and "endurance, android" in J and "contenus dans ce run : WFS-11." in J,
                "run rouge : entrée au journal (jobs en échec, tickets 🟣 du run)")
        avantJ = J
        lancer(d, run, "cancelled", 814)
        verifie((d / "journal.md").read_text(encoding="utf-8") == avantJ and (d / "TACHES.md").read_text(encoding="utf-8") == TACHES,
                "run annulé : rien")
        statuts.main(["--run-number", "815", "--sha", run, "--conclusion", "success", "--evenement", "schedule", "--depot", str(d)])
        verifie((d / "TACHES.md").read_text(encoding="utf-8") == TACHES, "run de nuit (endurance seule) vert : aucun ✅")
    print(f"statuts : {'tout est OK' if not echecs else str(echecs) + ' échec(s)'}")
    return 1 if echecs else 0


if __name__ == "__main__":
    sys.exit(main())
