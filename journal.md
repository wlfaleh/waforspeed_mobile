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

### 2026-11-24 18:12:10 — WFS-602 en PR #445 : version PC Windows, 1re étape (Claude, lead dev (01UY1Vv3au))
- `WaForSpeed.exe` (MinGW-w64, depuis Linux) : fenêtre Win32, surface Vulkan Win32, clavier / souris / manette XInput par le module manette, CGU et chargement comme Android ; sauvegarde dans `%APPDATA%\WaForSpeed`.
- `WaForSpeed-Setup.exe` (NSIS, 96 Mo) : Program Files, raccourci bureau, menu Démarrer, désinstallation ; sans les voitures retirées (WFS-404). Construction : `bash tools/pc/construire_installateur.sh`.
- Accroches annoncées (sous `#if defined(_WIN32)`, rien ne change sur Android ni au banc Linux) : `src/render/vk.cpp` (12 lignes, surface Win32), `src/game/reseau_relais.cpp` (10 lignes, pas de prises avant WFS-604), `src/render/bench_perf.cpp` (6 lignes, `getrusage` Linux seulement).
- Captures `menu1` : 39 paires identiques avant / après (12 gardées). Wine plante dans le conteneur : pas d'essai Windows ici, installateur envoyé à l'utilisateur pour essai (ticket 🟡 après fusion).
- Outils installés dans le conteneur (apt, dépôts Ubuntu) : mingw-w64, nsis, glslc, libvulkan-dev, mesa-vulkan-drivers, wine64, xdotool.

### 2026-11-24 18:26:42 — WFS-602 : PR #445 refusée par le lead dev (lead-dev)
- Relecture par une autre instance du lead dev, commit 6c02fdd1. J'ai ouvert les 12 paires de captures : elles sont identiques, sans régression. J'ai reconstruit le banc Linux : 159 tests CPU réussis, dont test_relais. L'exe Windows est à jour et n'importe de vulkan-1.dll que des fonctions du cœur Vulkan et du WSI.
- Refus : (1) la capture « Tests CPU » et `tests-cpu.log` manquent (règle du 2026-11-11) ; (2) la nitro au clavier laisse la touche A enfoncée si Maj est relâchée hors course (`touche()`) ; (3) mineur : XInput remet à zéro les boutons A, B et Y tenus au clavier. La 2e revue ne demande pas de nouvelles captures.

### 2026-11-24 18:35:59 — WFS-602 corrigé après la 1re revue, PR #445 à relire (Claude, lead dev (01UY1Vv3au))
- Nitro au clavier relâchée partout, état propre à la manette XInput (99b16fd7) ; tests CPU 159 réussis, `qa/pr/WFS-602/tests-cpu.log` (6da47c80). Réponse point par point dans la PR ; 2e revue confiée à une autre instance du lead dev.

### 2026-11-24 18:41:38 — WFS-602 fusionné par le lead dev (lead-dev)
- 2e revue de la PR #445 par une autre instance du lead dev, sur le commit 6da47c80. Les 3 points du refus sont corrigés : section « Tests CPU » et `tests-cpu.log` ajoutés (159 tests réussis), relâchement de Maj / N qui libère A partout, état précédent propre à la manette XInput. Le développeur a répondu point par point dans la PR.
- `ninja -C build-windows WaForSpeed` réussit (`WaForSpeed.exe` lié). La branche n'avait que des documents de retard sur `main`. Aucune nouvelle capture n'était demandée (règle du 2026-11-15).
- « ✅ Approuvé » posté en commentaire (même compte GitHub), puis fusion en squash : b7552678. Statut 🟡 À valider (utilisateur), à essayer sur un vrai PC Windows. Ligne ajoutée à `qa/Check.md`, ligne de `suivi-pr.md` supprimée, session du lead dev 01UY1Vv3au terminée dans `track-agent.md`.
