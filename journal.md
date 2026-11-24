# WaForSpeed Mobile — journal

Journal du projet (tickets WFS-XX de `TACHES.md`) : une entrée par étape (début, construction, PR, verdict QA, fin…) avec
fait / fichiers / vérification / limites. **Nouvelles entrées à la fin du fichier.** Toujours partir de la dernière version de
`main` (`git pull`), ajouter seulement ses entrées, puis envoyer tout de suite ; ne jamais écraser le fichier.
**Chaque entrée commence par la date et l’heure UTC complètes, jusqu’à la seconde** : `### AAAA-MM-JJ hh:mm:ss — titre (auteur)`, heure obtenue avec `date -u "+%Y-%m-%d %H:%M:%S"` au moment d’écrire, jamais devinée ni recopiée (une date seule, sans heure, est refusée).
Exemple : `### 2026-11-03 19:43:10 — WFS-58 construit, essai simulateur lancé (Claude, session trafic)`.
> **Archive (règle de l'utilisateur, 2026-11-06)** : ce fichier ne garde que les **2 derniers jours**. Les entrées plus anciennes sont dans `journal/archive/AAAA-MM-JJ.md` (un fichier par jour) ; ne les relire qu'en cas de besoin précis. Le PO déplace chaque jour les entrées de plus de 2 jours.

### 2026-11-23 11:26:31 — Statuts automatiques, bancs du matin #73 (robot statuts)
- Run vert (schedule, commit d143a9c) : WFS-555 ✅ (PR #425, e02540c), WFS-564 ✅ (PR #429, 5f38a6b), WFS-496 ✅ (PR #428, 5e656ac), WFS-503 ✅ (PR #427, cb7932d), WFS-589 ✅ (PR #440, 1d55a33), WFS-408 ✅ (PR #434, ba42aed).

### 2026-11-23 12:15:12 — Archives automatiques (robot archives)
- Journal : 9 entrée(s) de plus de 2 jours déplacée(s) vers `journal/archive/` (2026-11-21).
- TACHES.md : 6 ligne(s) de ticket terminé, annulé ou abandonné déplacée(s) vers `archive/taches-terminees.md` (WFS-555, WFS-564, WFS-496, WFS-503, WFS-589, WFS-408).

### 2026-11-23 13:36:24 — Vidéo de l'utilisateur « grave » : WFS-600 et WFS-601 créés (po-scrum-master)
- Vidéo du 2026-11-23 (15 s, garage → GT5 → Corniche) analysée image par image (1 et 4 images / s).
- **ANO-318 (Bloquante)** : à 30-57 km/h en ligne droite, un coup de volant fait pivoter la caisse de 30-45° (avance en crabe) ; au freinage elle glisse de travers puis recule en tournant jusqu'à être perpendiculaire à la route. → **WFS-600** (P1, E3, `@conduite`, en tête de tous les tickets prêts). Le code de conduite (`player.*`, `roues.*`, `direction.*`) n'a pas changé ces derniers jours : défaut ancien, pas une régression d'hier.
- **ANO-319 (Majeure)** : skin Camouflage de la GT5 au garage, voiture verte unie en course. → **WFS-601** (P1, E18 côté course seulement ; garage ⛔ non modifié).
- Preuves : `regles-de-gestion/preuves/WFS-600/`. À noter : la GT5 vient d'un modèle CC BY-NC-SA (commentaire de `src/game/cars.h`) : à signaler au juriste avant la sortie publique.

### 2026-11-23 20:03:05 — Correction du PO : ANO-319 mal lue, WFS-601 réécrit (po-scrum-master)
- Remarque de l'utilisateur : « regarde la voiture choisie et la voiture en course ». Zooms en pleine définition : en course, la GT5 n'est pas celle du garage : verte unie, sans skin, **sans vitres** (on voit l'intérieur), **lunette arrière en trou noir**, modèle pauvre. Ma 1re lecture (« skin absente ») était incomplète : faute du PO.
- ANO-319 passe **Bloquante** ; WFS-601 réécrit (même voiture qu'au garage, carrosserie fermée, 17 voitures vérifiées), **à égalité en tête avec WFS-600**.
- Correction de mon entrée précédente : la GT5 (id `mustang`) est un modèle **Corvette C7 CC BY 4.0** (WFS-359) ; la mention CC BY-NC-SA de `cars.h` parle de l'ancien modèle. Pas de sujet de licence.

### 2026-11-24 13:32:21 — Archives automatiques (robot archives)
- Journal : 58 entrée(s) de plus de 2 jours déplacée(s) vers `journal/archive/` (2026-11-22).
- TACHES.md : 0 ligne(s) de ticket terminé, annulé ou abandonné déplacée(s) vers `archive/taches-terminees.md`.

### 2026-11-24 17:27:45 — E24 « Version PC Windows » créée, WFS-602 au lead dev en renfort (po-scrum-master)
- Demande de l'utilisateur (2026-11-24, session lead dev 01UY1Vv3au) : version PC Windows aux mêmes fonctions, installateur Windows (pas de Steam), pas d'achats sur PC au début, cross-play duel PC ↔ Android par le relais. Sujet urgent confié au lead dev (règle « lead dev en renfort »).
- Épic **E24** (`regles-de-gestion/E24-version-pc.md`) : WFS-602 (P1, fenêtre + boucle + clavier / souris / manette + installateur MinGW / NSIS), 603 (son WASAPI), 604 (TLS + duel cross-play), 605 (paquets et sauvegarde, `@cartes`), 606 (réglages graphiques), 607 ([TECHNIQUE] CI Windows, `@cicd`), 608 (équité du duel, décision de l'utilisateur à prendre). 603 à 608 ⏳ En attente de 602.
- **WFS-602** 🔧 attribué à Claude, lead dev (01UY1Vv3au) (session_01UY1Vv3aubodvdU7e7M2as8), branche `feature/waforspeed-602`, règles `regles-de-gestion/WFS-602.md`. Zone : `src/platform/windows_main.cpp` (nouveau), bloc `WIN32` de `CMakeLists.txt`, `tools/pc/**` ; aucun changement du jeu Android. PR relue et fusionnée par une autre instance du lead dev. Priorité Android maintenue pour les autres sessions.
