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

### 2026-11-27 14:23:57 — Ordre de l'utilisateur : suite de WFS-611 dans la même session ; WFS-612 et WFS-613 créés (po-scrum-master)
- Ordre de l'utilisateur (2026-11-27, session du lead dev 01UY1Vv1NH) : « Tu dois tout faire tout les tâche créer par le po dans cette sesssion tu dois les faires dans cette session ». Plus récent que la règle « un seul sujet par session », il passe avant elle pour cette session.
- Créés (RG-05 de WFS-611), [TECHNIQUE] P1, E4, zone `src/render/**`, `shaders/**`, `tests/**`, `CMakeLists.txt` : **WFS-612** (glow de `post.frag` sur image réduite) et **WFS-613** (lumières de nuit de `eclairage.glsl` limitées par tuile / objet, portée bornée). Règles : `regles-de-gestion/WFS-612.md`, `regles-de-gestion/WFS-613.md`. WFS-609 sauté (branche déjà présente).
- Attribués à Claude, lead dev (01UY1Vv1NH) (session_01UY1Vv1NH1rKiiKnxJVinu2), « 📋 Prêt (suite de WFS-611, même session) », après la fusion de WFS-611 ; fusion par une autre instance du lead dev ; 🟡 À valider (utilisateur) après fusion.

### 2026-11-27 15:29:07 — WFS-611 / 612 / 613 : APK d'essai avant fusion, CI lancée sur la branche (Claude, lead dev (01UY1Vv1NH))
- Demande de l'utilisateur : « Tu peux me générer un fichier avant de merger ? » → un APK qui contient les trois tickets de performance, à essayer sur le téléphone avant la fusion.
- Branche `feature/waforspeed-613` (a207e551) : WFS-613 + reprise de WFS-611 et WFS-612 + synchronisation avec main ; compilation locale du moteur et tests test_familles_pipeline, test_halo_reduit, test_tuiles_lumieres OK.
- Lancement à la main de `build.yml` (US techniques : CI de branche permise, une à la fois) : run #1167 (46025288758) ; il vérifie compilation, tests, bancs et APK ; APK dans la Release `apercu-waforspeed-613`.
- Mesures déjà faites au banc PC : WFS-612 passe finale 54,6 → 22,9 ms (lavapipe), écart moyen ≤ 0,0014 niveau ; WFS-613 10 / 10 captures identiques au pixel près, 71 % des couples (tuile, lumière) écartés.

### 2026-11-27 16:35:17 — WFS-611 / 612 / 613 bloqués par l'utilisateur (Claude, lead dev (01UY1Vv1NH))
- Ordre de l'utilisateur : « Je merge pas et bloque les PRs. J'ai commencé le test, il y a des trucs bizarres. J'ai lancé le test auto, je te renvoie le rapport dès qu'il finit. »
- Aucune PR ouverte, aucune fusion ; statut des trois tickets : ⛔ Bloqué par l'utilisateur (APK d'essai `apercu-waforspeed-613`, run #1167). Aucun lead dev (passage de l'heure compris) n'ouvre ni ne fusionne de PR pour ces tickets avant le rapport de l'utilisateur et sa levée du blocage.

### 2026-11-27 17:26:16 — WFS-614 créé et attribué au lead dev (01UY1Vv1NH) (po-scrum-master)
- Rapport du banc du téléphone (WFS-420, S21 FE, Ville Basse 45 %) sur l'APK d'essai 611-613 : nuit 42,5 ms (avant 62), jour 24,1 ms ; l'utilisateur : « une catastrophe », « essaie d'améliorer plus ». Coût restant dans les matières des pixels, famille inconnue ; 7 essais lents (≈ 70 ms) inexpliqués.
- **WFS-614** [TECHNIQUE] P1, E4 : trois interrupteurs de couleur unie (sols, décor, voitures), mesurés par le banc ; fréquence réelle de l'écran et plafond img/s par essai dans le rapport. Règles : `regles-de-gestion/WFS-614.md`.
- Attribué à Claude, lead dev (01UY1Vv1NH) (session_01UY1Vv1NH1rKiiKnxJVinu2), ordre de l'utilisateur (tout dans cette session). Branche `feature/waforspeed-614` partie de `feature/waforspeed-613`. ⛔ Pas de PR ni de fusion avant la levée du blocage ; ensuite 🟡 À valider (utilisateur).

### 2026-11-27 17:59:17 — WFS-614 : APK de diagnostic, CI lancée sur la branche (Claude, lead dev (01UY1Vv1NH))
- Suite du banc du téléphone de l'utilisateur (S21 FE, 17:14) : nuit 42,5 ms (avant 62), jour 24,1 ms (40 img/s) ; coût restant dans les matières (couleur unie : GPU 37 → 12 ms) ; 7 essais dans un état lent inexpliqué (≈ 70 ms).
- `feature/waforspeed-614` (6ce0c43f, contient 611 à 613) : 3 interrupteurs « couleur unie » par famille (sols, décor, voitures) mesurés par le banc automatique (34 essais), fréquence réelle de l'écran, plafond et cadence écrits pour chaque essai ; tests CPU 162 / 162.
- Lancement à la main de `build.yml` (US technique) : run #1168 (46044033746) ; APK publié dans la Release `apercu-waforspeed-614` à 17:51 UTC. Aucune PR, aucune fusion (blocage de l'utilisateur).
