# WaForSpeed Mobile — journal

Journal du projet (tickets WFS-XX de `TACHES.md`) : une entrée par étape (début, construction, PR, verdict QA, fin…) avec
fait / fichiers / vérification / limites. **Nouvelles entrées à la fin du fichier.** Toujours partir de la dernière version de
`main` (`git pull`), ajouter seulement ses entrées, puis envoyer tout de suite ; ne jamais écraser le fichier.
**Chaque entrée commence par la date et l’heure UTC complètes, jusqu’à la seconde** : `### AAAA-MM-JJ hh:mm:ss — titre (auteur)`, heure obtenue avec `date -u "+%Y-%m-%d %H:%M:%S"` au moment d’écrire, jamais devinée ni recopiée (une date seule, sans heure, est refusée).
Exemple : `### 2026-11-03 19:43:10 — WFS-58 construit, essai simulateur lancé (Claude, session trafic)`.
> **Archive (règle de l'utilisateur, 2026-11-06)** : ce fichier ne garde que les **2 derniers jours**. Les entrées plus anciennes sont dans `journal/archive/AAAA-MM-JJ.md` (un fichier par jour) ; ne les relire qu'en cas de besoin précis. Le PO déplace chaque jour les entrées de plus de 2 jours.

### 2026-11-26 11:27:10 — Statuts automatiques, bancs du matin #85 (robot statuts)
- Run rouge (schedule, commit f41caa2), aucun statut changé. Job(s) en échec : banc (texte), banc (ecrans-2400), publication-captures.
- Tickets 🟣 contenus dans ce run : aucun.

### 2026-11-26 12:15:31 — Archives automatiques (robot archives)
- Journal : 9 entrée(s) de plus de 2 jours déplacée(s) vers `journal/archive/` (2026-11-24).
- TACHES.md : 0 ligne(s) de ticket terminé, annulé ou abandonné déplacée(s) vers `archive/taches-terminees.md`.

### 2026-11-27 10:30:38 — WFS-611 attribué au lead dev en renfort (po-scrum-master)
- Ordre de l'utilisateur (2026-11-27) : minimum 40 img/s en qualité top, objectif 60 gardé. Ticket **WFS-611** (P1 Urgent, E4, [TECHNIQUE]) créé, 🔧 à Claude, lead dev (01UY1Vv1NH) (session_01UY1Vv1NH1rKiiKnxJVinu2), branche `feature/waforspeed-611` (dernier envoi 2026-11-27 10:30:20 UTC).
- Zone (exception PO) : `src/render/**`, `shaders/**`, `src/platform/chauffe*`, `src/platform/android_main.cpp` (accroche JNI), `JeuActivity.java` (accroche, 6 lignes), `tests/**`, `CMakeLists.txt`. Règles : `regles-de-gestion/WFS-611.md` (RG-01 à RG-05). Relecture et fusion par une autre instance du lead dev ; après fusion 🟡 À valider (utilisateur).


### 2026-11-27 12:00:59 — Archives automatiques (robot archives)
- Journal : 1 entrée(s) de plus de 2 jours déplacée(s) vers `journal/archive/` (2026-11-25).
- TACHES.md : 0 ligne(s) de ticket terminé, annulé ou abandonné déplacée(s) vers `archive/taches-terminees.md`.
