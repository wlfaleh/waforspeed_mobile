#!/usr/bin/env python3
"""Test de tools/qa/archives.py (WFS-198, RG-08) sur un dépôt d'exemple créé dans un dossier temporaire.

Cas : entrée vieille → archivée ; entrée d'hier → gardée ; ligne ✅ → archivée et recopiée dans le fichier de règles ;
ligne 🟣 → gardée ; ligne mal formée → gardée et listée ; écart de comptage → rien d'écrit (code 1).
Usage : python3 tools/qa/test_archives.py   (code de sortie 0 si tout est vert)
"""
import sys
import tempfile
from pathlib import Path

ICI = Path(__file__).resolve().parent
sys.path.insert(0, str(ICI))
import archives  # noqa: E402

import datetime  # noqa: E402

AUJOURDHUI = datetime.date(2026, 11, 8)
MAINTENANT = "2026-11-08 03:07:12"
JOURNAL = (
    "# Journal\n\nEn-tête du journal : ne bouge pas.\n\n"
    "### 2026-11-05 10:00:00 — Vieille entrée (agent 1)\n- texte ancien\n\n"
    "### 2026-11-06 11:00:00 — Entrée d'avant-hier (agent 2)\n- texte avant-hier\n\n"
    "### Entrée sans date (agent 3)\n- à corriger\n\n"
    "### 2026-11-07 12:00:00 — Entrée d'hier (agent 1)\n- texte hier\n\n"
    "### 2026-11-08 01:00:00 — Entrée d'aujourd'hui (agent 2)\n- texte du jour\n"
)
ARCHIVE_05 = archives.ENTETE_JOUR.format(d="2026-11-05") + "### 2026-11-05 09:00:00 — Déjà archivée (agent 4)\n- texte\n"
ENTETE_T = "| Priorité | Clé | Titre | Épic | Statut | Responsable | Dépend de | Branche | PR | QA |\n|---|---|---|---|---|---|---|---|---|---|\n"
L_FINI = "| P2 Haute | WFS-21 | Ticket terminé | E3 | ✅ Terminé (CI main #700) | agent 1 | — | `feature/waforspeed-21` | #21 |  |\n"
L_ANNULE = "| P3 Moyenne | WFS-22 | Ticket annulé | E3 | Annulé (doublon) | agent 1 | — | — | — |  |\n"
L_FUSION = "| P2 Haute | WFS-23 | Ticket fusionné | E2 | 🟣 Fusionné (PR #23) | agent 2 | — | `feature/waforspeed-23` | #23 |  |\n"
L_MAL = "| P2 Haute | WFS-24 | Ligne mal formée | E4 | ✅ Terminé | agent 3 | — |\n"
L_COURS = "| P3 Moyenne | WFS-25 | En cours | E6 | 🔧 En cours | agent 5 | — | `feature/waforspeed-25` |  |  |\n"
TACHES = (
    "# Tâches\n\n| Épic | Zone |\n|---|---|\n| E3 | WFS-21 ✅ cité dans un autre tableau |\n\n" + ENTETE_T +
    L_FINI + L_ANNULE + L_FUSION + L_MAL + L_COURS + "\n## Notes\nTexte libre.\n"
)
REGLE_21 = "# WFS-21 — Ticket terminé\n\n## Règles\n- RG-01\n"
REGLE_22 = "# WFS-22\n\n## Suivi repris de `TACHES.md`\nDéjà repris.\n"
UTILISATEUR = "# Demandes\n- demande ouverte\n"

echecs = 0


def verifie(cond, quoi):
    global echecs
    print(f"archives : {quoi:<80} → {'OK' if cond else 'ÉCHEC'}")
    if not cond:
        echecs += 1


def depot(d):
    fichiers = {
        "journal.md": JOURNAL, "journal/archive/2026-11-05.md": ARCHIVE_05, "TACHES.md": TACHES,
        "archive/taches-terminees.md": "# Tickets terminés\n\n" + ENTETE_T,
        "regles-de-gestion/WFS-21.md": REGLE_21, "regles-de-gestion/WFS-22.md": REGLE_22,
        "tache utilisateur.md": UTILISATEUR,
    }
    for nom, texte in fichiers.items():
        p = d / nom
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(texte, encoding="utf-8")


def instantane(d):
    return {str(p.relative_to(d)): p.read_text(encoding="utf-8") for p in sorted(d.rglob("*")) if p.is_file()}


def cas_normal():
    with tempfile.TemporaryDirectory() as t:
        d = Path(t)
        depot(d)
        code = archives.menage(str(d), AUJOURDHUI, MAINTENANT)
        verifie(code == 0, "run normal : code de sortie 0")
        j = (d / "journal.md").read_text(encoding="utf-8")
        a05 = (d / "journal/archive/2026-11-05.md").read_text(encoding="utf-8")
        a06 = (d / "journal/archive/2026-11-06.md").read_text(encoding="utf-8") if (d / "journal/archive/2026-11-06.md").exists() else ""
        verifie(j.startswith("# Journal\n\nEn-tête du journal : ne bouge pas.\n\n"), "en-tête de journal.md inchangé")
        verifie("Vieille entrée" not in j and a05.endswith("### 2026-11-05 10:00:00 — Vieille entrée (agent 1)\n- texte ancien\n\n"),
                "entrée vieille → ajoutée à la fin de son fichier d'archive existant")
        verifie(a05.index("Déjà archivée") < a05.index("Vieille entrée"), "ordre gardé dans l'archive")
        verifie("avant-hier" not in j and a06.startswith("# Journal archivé — 2026-11-06\n") and "Entrée d'avant-hier" in a06,
                "entrée d'avant-hier → nouveau fichier d'archive avec l'en-tête")
        verifie("Entrée d'hier" in j and "Entrée d'aujourd'hui" in j, "entrées d'hier et d'aujourd'hui gardées")
        verifie("### Entrée sans date" in j, "entrée sans date gardée")
        robot = j[j.rindex("\n### ") + 1:]
        verifie(robot.startswith("### 2026-11-08 03:07:12 — Archives automatiques (robot archives)\n"), "entrée du robot au journal")
        verifie("2 entrée(s)" in robot and "2 ligne(s)" in robot and "(WFS-21, WFS-22)" in robot, "nombres déplacés dans l'entrée du robot")
        verifie("Entrée sans date" in robot and "WFS-24" in robot, "points à corriger listés pour le PO")

        tch = (d / "TACHES.md").read_text(encoding="utf-8")
        arc = (d / "archive/taches-terminees.md").read_text(encoding="utf-8")
        verifie(L_FINI not in tch and L_ANNULE not in tch and arc.endswith(ENTETE_T + L_FINI + L_ANNULE),
                "lignes ✅ et annulée → archivées telles quelles, dans l'ordre")
        verifie(L_FUSION in tch and L_COURS in tch, "lignes 🟣 et 🔧 gardées")
        verifie(L_MAL in tch, "ligne mal formée gardée")
        verifie("WFS-21 ✅ cité dans un autre tableau" in tch and tch.endswith("\n## Notes\nTexte libre.\n"),
                "autres tableaux et texte de TACHES.md inchangés")
        r21 = (d / "regles-de-gestion/WFS-21.md").read_text(encoding="utf-8")
        r22 = (d / "regles-de-gestion/WFS-22.md").read_text(encoding="utf-8")
        verifie(r21 == REGLE_21 + "\n## Suivi repris de `TACHES.md`\n" + L_FINI, "ligne ✅ recopiée dans regles-de-gestion/WFS-21.md")
        verifie(r22 == REGLE_22, "section de suivi déjà là → fichier de règles inchangé")
        verifie((d / "tache utilisateur.md").read_text(encoding="utf-8") == UTILISATEUR, "tache utilisateur.md jamais touché")

        avant = instantane(d)
        code = archives.menage(str(d), AUJOURDHUI, "2026-11-08 03:10:00")
        apres = instantane(d)
        diff = [k for k in apres if avant.get(k) != apres[k]]
        verifie(code == 0 and diff == ["journal.md"], "2e run : seuls les points à corriger sont redits au journal")


def cas_ecart():
    with tempfile.TemporaryDirectory() as t:
        d = Path(t)
        depot(d)
        avant = instantane(d)
        vrai = archives.compter_tickets
        appels = []

        def faux(texte):  # 2 premiers appels : avant ; ensuite : après, avec une ligne « perdue »
            appels.append(1)
            return vrai(texte) - (1 if len(appels) == 3 else 0)

        archives.compter_tickets = faux
        try:
            code = archives.menage(str(d), AUJOURDHUI, MAINTENANT)
        finally:
            archives.compter_tickets = vrai
        verifie(code == 1, "écart de comptage : code de sortie 1")
        verifie(instantane(d) == avant, "écart de comptage : aucun fichier écrit")


def cas_rien():
    with tempfile.TemporaryDirectory() as t:
        d = Path(t)
        (d / "journal.md").write_text("# Journal\n\n### 2026-11-08 01:00:00 — Du jour (agent 1)\n- x\n", encoding="utf-8")
        (d / "TACHES.md").write_text("# Tâches\n\n" + ENTETE_T + L_COURS, encoding="utf-8")
        avant = instantane(d)
        code = archives.menage(str(d), AUJOURDHUI, MAINTENANT)
        verifie(code == 0 and instantane(d) == avant, "rien à ranger : aucun fichier écrit, pas d'entrée du robot")


def cas_archive_neuve():
    with tempfile.TemporaryDirectory() as t:
        d = Path(t)
        (d / "journal.md").write_text("# Journal\n", encoding="utf-8")
        (d / "TACHES.md").write_text("# Tâches\n\n" + ENTETE_T + L_FINI, encoding="utf-8")
        code = archives.menage(str(d), AUJOURDHUI, MAINTENANT)
        arc = (d / "archive/taches-terminees.md").read_text(encoding="utf-8")
        verifie(code == 0 and arc.endswith(ENTETE_T + L_FINI), "archive des tickets absente → créée avec l'en-tête du tableau")


cas_normal()
cas_ecart()
cas_rien()
cas_archive_neuve()
print("archives : %s" % ("TOUT EST VERT" if echecs == 0 else "%d ÉCHEC(S)" % echecs))
sys.exit(1 if echecs else 0)
