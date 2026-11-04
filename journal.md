# WaForSpeed Mobile — journal

Journal du projet (tickets WFS-XX de `TACHES.md`) : une entrée par étape (début, construction, PR, verdict QA, fin…) avec
fait / fichiers / vérification / limites. **Nouvelles entrées à la fin du fichier.** Toujours partir de la dernière version de
`main` (`git pull`), ajouter seulement ses entrées, puis envoyer tout de suite ; ne jamais écraser le fichier.
**Chaque entrée commence par la date et l’heure UTC complètes, jusqu’à la seconde** : `### AAAA-MM-JJ hh:mm:ss — titre (auteur)`, heure obtenue avec `date -u "+%Y-%m-%d %H:%M:%S"` au moment d’écrire, jamais devinée ni recopiée (une date seule, sans heure, est refusée).
Exemple : `### 2026-11-03 19:43:10 — WFS-58 construit, essai simulateur lancé (Claude, session trafic)`.

### 2026-11-02 — Démarrage du projet natif
**Fait**
- Dépôt créé, choix techniques : C++20, Vulkan, Android NativeActivity, aucun moteur de jeu tiers.
- Tableau des tâches (5 livraisons de 5 fonctionnalités).
- Skills pour les agents : `waforspeed-natif` (règles du moteur natif, processus, construction), `design-street-racing`
  (direction artistique, reprise de la version web), `waforspeed-3d` (référence de la version navigateur). `CLAUDE.md` à la racine.

### 2026-11-02 — Début F1 à F5 (Claude, session moteur-natif)
Réservation de la livraison 1 complète. Fichiers prévus : `src/**`, `shaders/**`, `tools/convert_car.mjs`, `assets/**`,
`CMakeLists.txt`, `android/**`, `.github/workflows/build.yml`.

### 2026-11-02 — F1 à F5 (livraison 1) : code écrit, première construction en cours
**Fait**
- **F2 Noyau** : `src/core/jobs.*` (un thread par cœur, `parallelFor`, compteurs), `memory.h` (arène par image, réserve d'objets),
  `math.h` (vecteurs, matrices, perspective Vulkan à profondeur inversée, générateur `Mulberry32` identique au jeu web), `log.h`.
- **F3 Rendu Vulkan** : `src/render/vk.*` (instance, GPU, swapchain avec pré-rotation, tampons, textures avec mipmaps GPU),
  `renderer.*` (passe HDR 16 bits + MSAA x4 en mémoire transitoire, ciel analytique, PBR GGX, vernis de peinture, vitres triées,
  passe finale ACES + halo + vignette + tramage, interface 2D). Shaders dans `shaders/`.
- **F4 Voitures** : `tools/convert_car.mjs` (glTF compressé du jeu web → `.wfm` binaire, textures WebP → JPEG/PNG ≤ 1024 px,
  regroupement par matériau, détection peinture / feux / vitres / roues). Golf 8 GTI, BMW M4, Porsche 911, Lamborghini Gallardo
  (4,6 à 6,7 Mo, ~100-125 k triangles chacune). Chargement `model_loader.cpp` : textures décodées en parallèle sur tous les cœurs.
- **F5 Route et conduite** : `src/game/track.*` (port exact de `track.js`), `player.*` (port de la conduite de `update.js` :
  braquage progressif, adhérence, centrifuge, moteur/frein/frottements, nitro, bordures), `game.*` (route par morceaux de 38 m
  générés en parallèle : chaussée avec marquages procéduraux, bordures, trottoirs, bas-côtés ; caméra de poursuite proche/haute ;
  commandes tactiles : glisser à gauche = volant, à droite frein / accélérateur / nitro ; jauges vitesse et nitro).
- **F1 Construction** : `CMakeLists.txt`, projet Gradle `android/`, `.github/workflows/build.yml` (banc d'essai Linux avec captures,
  APK arm64, Release `apercu`).

**Vérification**
- Tout le C++ compile sans erreur ni avertissement (g++ 13, `-Wall -Wextra`).
- Test `tests/test_track.cpp` : la piste native est identique à celle du jeu web sur les 4 cartes (mêmes nombres de segments,
  mêmes points 3D à 10⁻⁵ près). Conduite : Golf 0 → 181 km/h en 10 s.

**Pas encore vérifié**
- Compilation des shaders, banc d'essai Vulkan et APK : première construction GitHub en cours.
- Rendu réel sur téléphone (pré-rotation, tactile, performances) : à tester sur l'appareil de l'utilisateur.

### 2026-11-02 — Premières constructions et premier essai sur téléphone (Claude, session moteur-natif)
**Fait**
- CI : banc d'essai Linux OK du premier coup (shaders compilés, test de la piste OK, 18 captures).
  APK : 2 erreurs de configuration corrigées (numéro de version Gradle, chemin de `glslc` du NDK) → APK 23 Mo publié.
- Captures examinées et corrigées : caméra en retard (voiture invisible en course), voiture qui flottait (ajout d'une ombre
  de contact), route trop claire et bleutée (reflets du ciel atténués sur les surfaces rugueuses), tache blanche du soleil
  sur la route (le disque solaire ne se reflète plus sur les surfaces rugueuses), bord du sol visible (sol étendu à 260 m).
- **Retour utilisateur sur téléphone** : l'appli démarre et tourne, mais l'image était étirée ~4× en largeur.
  Cause : démarrage en portrait puis passage en paysage, la swapchain gardait la forme portrait.
  Corrections : vérification de la rotation / taille de la surface toutes les 15 images avec recréation,
  caméra et interface calées sur la taille réelle de la fenêtre (`Renderer::setViewSize`), plein écran sous l'encoche.
- Fichiers : `src/render/{vk,renderer}.*`, `src/game/game.*`, `src/platform/*`, `shaders/{common.glsl,scene.frag,post.frag}`,
  `android/app/{build.gradle,src/main/**}`, `CMakeLists.txt`.

**Vérification**
- Construction n° 8 : banc d'essai + APK OK ; captures relues (4 voitures en vitrine, course caméra proche et haute).

**Reste / limites**
- Correction de l'image déformée à confirmer sur le téléphone de l'utilisateur.
- Décor encore vide (bâtiments, arbres, lampadaires = F6), pas d'ombres dynamiques (F7), pas de trafic (F8), pas de son (F10).
- Barre de navigation Android encore visible (mode immersif à faire).

### 2026-11-02 — Organisation, QA automatique (Claude, session moteur-natif)
**Fait**
- Tableau type Jira (tickets WFS-XX, une branche par ticket, synchronisation avec `main` avant validation et avant PR).
- Branche `claude/moteur-natif` reprise en `feature/waforspeed-01` (lot WFS-01→05), PR #1 ouverte vers `main`.
- Règle : ne jamais supprimer de branche (gérées par l'utilisateur).
- QA : agent `qa-lead` (`.claude/agents/qa-lead.md`) qui **regarde les captures** et rend un verdict VALIDÉ / REFUSÉ ;
  captures produites automatiquement par la CI sur un émulateur Android 14 (`tools/qa/emulateur.sh` : démarrage,
  accélération, nitro, braquage, freinage) en plus du banc d'essai. APK aussi en x86_64 pour l'émulateur.
- Règle : le tableau est mis à jour et envoyé sur `main` après chaque avancement.

**En cours**
- Construction de `feature/waforspeed-01` avec le premier test sur émulateur ; ensuite verdict du QA lead sur WFS-01→05.

### 2026-11-02 — Début WFS-08 Trafic multi-thread et collisions (Claude, session trafic)
Réservation de WFS-08. Branche `feature/waforspeed-08`, partie de `feature/waforspeed-01` (le code de la livraison 1
n'est pas encore dans `main`) ; synchronisation avec `main` dès la fusion de la PR #1.
**Plan**
- `src/game/traffic.{h,cpp}` : port exact de `js/game/traffic.js` (addCars, laneFree, stepTraffic, crashTraffic,
  pas fixe 1/60 s, mêmes graines `mulberry32` et `nextRand`), catalogue des dimensions du jeu web pour garder le même tirage.
- Mise à jour en parallèle sur les threads de `jobs()` : décisions calculées sur une copie en lecture seule, écrites ensuite.
- Collisions joueur / trafic et quasi-contacts (port de la partie chocs de `js/game/update.js`).
- Rendu des voitures du trafic dans `Game` (modèles partagés, dessinées seulement à portée de vue en attendant le LOD de WFS-20).
- `tests/test_traffic.cpp` : même trafic que le jeu web (comparaison avec un script Node qui exécute traffic.js).
**Fichiers prévus** : `src/game/traffic.*`, `src/game/game.*`, `src/game/cars.h`, `tests/test_traffic.cpp`,
`tools/qa/` (script de comparaison), `CMakeLists.txt`, `.github/workflows/build.yml` (lancement du test).

### 2026-11-02 — Branche hors processus repérée
`claude/version-web-capacitor` (autre agent, 12h59 → 19h08) : version web emballée avec Capacitor dans `www/`, créée
**avant** la mise en place de `CLAUDE.md` et du tableau. Approche abandonnée (l'utilisateur veut une appli native,
sans navigateur). Hors tableau : **ne pas fusionner, ne pas reprendre**. Branche conservée (on ne supprime pas de branche).
Règle ajoutée à `CLAUDE.md` : même si la session attribue une branche `claude/…`, travailler sur `feature/waforspeed-XX`.

### 2026-11-02 — QA WFS-01→05, passage 1 : REFUSÉ (Claude, session moteur-natif)
Émulateur Android 14 (2400x1080) : installation et démarrage OK, mais scène rendue en portrait 1080x2400 (erreur de
pré-rotation, cause de l'image étirée sur téléphone) et vitesse restée à 0 (0,1 image/s sur GPU logiciel, appuis non vus).
Rapport : `qa/rapports/WFS-01.md`. Corrections poussées sur `feature/waforspeed-01` ; nouveau passage QA à la prochaine construction.

### 2026-11-02 — WFS-08 : code écrit, première construction en cours (Claude, session trafic)
**Fait**
- `src/game/traffic.*` : port de `traffic.js` (addCars, laneFree, stepTraffic, advanceTraffic, crashTraffic) et des chocs
  de `update.js` (hitCar, quasi-contacts). Mode parallèle : la voiture de devant de chaque voiture est cherchée sur
  tous les cœurs (`jobs().parallelFor`), le reste du pas garde l'ordre du jeu web.
- `Player` : accident (tête-à-queue, commandes bloquées) comme le jeu web. `Track::wrap` : même calcul que le JavaScript.
- `Game` : trafic dessiné avec les 4 modèles natifs (chargés une fois, partagés avec le joueur), à la longueur réelle de
  chaque voiture, 14 au plus de 30 m derrière à 300 m devant ; secousse aux chocs ; pilote automatique du banc d'essai
  porté de `autopilot()` (change de file, freine derrière plus lent).
- Fichiers hors du plan initial : `src/game/player.*`, `src/game/track.h`, `src/platform/desktop_main.cpp` (journal du banc).

**Vérification (conteneur)**
- `test_traffic` : **0 écart sur 39 voitures** comparées au jeu web (2 cartes, 20 s) ; mode parallèle déterministe et sans
  chevauchement ; choc arrière à 156 km/h → accident, voiture percutée en glissade. `test_track` inchangé.
- Tout le C++ compile sans avertissement (`-Wall -Wextra`).

**Pas encore vérifié** : shaders, banc d'essai Vulkan, APK et captures (construction GitHub en cours).
**Limites** : 16 des 20 voitures du jeu web s'affichent avec le modèle natif le plus proche (les 20 modèles : WFS-13) ;
pas encore de feux stop ni de clignotants dessinés ; score des quasi-contacts compté mais pas affiché (HUD : WFS-12).

### 2026-11-02 — Fusion exceptionnelle de la PR #1 et début WFS-05 (retours QA) (Claude, session moteur-natif)
- À la demande de l'utilisateur, PR #1 (WFS-01→05) fusionnée dans `main` **avant** le verdict du passage 2 de la QA
  (le test sur émulateur de la construction n° 29 était encore en cours, peut-être bloqué).
- WFS-01 à WFS-04 : ✅. WFS-05 : 🔧, nouvelle branche `feature/waforspeed-05` (partie de `main`) pour traiter les retours
  QA du passage 1 et les faire valider : orientation paysage (pré-rotation), commandes tactiles prises en compte,
  qualité réduite sur GPU logiciel, test émulateur qui se termine.
- Fichiers prévus : `src/render/{vk,renderer}.*`, `src/game/game.*`, `src/platform/android_main.cpp`, `tools/qa/emulateur.sh`,
  `.github/workflows/build.yml`, `qa/rapports/WFS-05.md`.

### 2026-11-02 — Fin WFS-05 et LIVRAISON 1 (Claude, session moteur-natif)
- QA passage 2 **VALIDÉ** (`qa/rapports/WFS-05.md`) : image en paysage 2400x1080 sur émulateur, commandes prises en compte,
  aucun plantage. Limite : émulateur à ~0,2 image/s (GPU logiciel).
- PR #2 fusionnée. WFS-01 à WFS-05 terminés : **livraison 1 « Première course »** dans `main`.
- APK de `main` : Release `apercu` (reconstruite par la CI après la fusion).
- Reste à mesurer sur un vrai téléphone : fluidité (images/s), confort des commandes.

### 2026-11-02 — Organisation en épics, priorisation, PO / Scrum Master (Claude, session moteur-natif)
- Backlog découpé en 8 épics (E0 à E7), chacune avec sa zone de code ; tickets priorisés avec dépendances.
- Nouveaux tickets : WFS-26 (texte), WFS-27 (HUD), WFS-28 (bruitages), WFS-29 (Android immersif), WFS-30 (points
  d'extension du moteur, priorité 1 : catalyseur du travail en parallèle). WFS-12 recentré sur les règles de course (sans HUD).
- Agent `po-scrum-master` : attribue les tickets (règle : un ticket en cours par épic, un par agent).
- WFS-08 (session trafic) conservé en cours dans l'épic E3.


### 2026-11-02 — Début WFS-30 Points d'extension du moteur (Claude, session moteur-natif)
Attribué par le PO. Épic **E0 — Noyau partagé (catalyseur)**. Branche `feature/waforspeed-30`, partie de `main`.
**Zone de code autorisée** : `src/core/**`, `src/game/game.*`, `src/render/renderer.h` (API), `CMakeLists.txt`
(+ documentation dans le skill `waforspeed-natif`). Ne pas toucher `src/game/` hors `game.*` (épic E3, WFS-08 en cours).
**Critères**
- Interface `System` (init / update / render / interface) enregistrée dans `Game` : un nouveau module = un nouveau fichier + 1 ligne d'enregistrement
- CMake ajoute automatiquement les fichiers de `src/world`, `src/ui`, `src/audio`, `src/game`
- Accès en lecture pour les systèmes : piste, joueur, caméra, renderer ; doc dans le skill `waforspeed-natif`
- Aucun changement visible ; CI verte, captures identiques
**Plan**
- `src/core/system.h` : interface `System` + contexte en lecture seule (piste, joueur, caméra, renderer).
- `Game` : liste de systèmes enregistrés, appelés dans la boucle (init, update, render, interface).
- `CMakeLists.txt` : `file(GLOB_RECURSE … CONFIGURE_DEPENDS)` sur `src/world`, `src/ui`, `src/audio`, `src/game`.
- Doc du point d'extension dans le skill `waforspeed-natif` ; vérifier CI verte et captures identiques ; QA par `qa-lead`.
- Attention : WFS-08 (E3) modifie aussi `src/game/game.*` et `CMakeLists.txt` → se synchroniser souvent avec `main`.

### 2026-11-02 — WFS-30 : code poussé, construction en cours (Claude, session moteur-natif)
- `src/core/system.h` (System, GameView, WFS_SYSTEM), appels dans `Game` (lignes d'accroche + bloc en fin de `game.cpp`),
  CMake : sources automatiques, `tests/test_systems.cpp` (OK en local : 2 systèmes, ordre respecté).
- Hors zone E0, lignes d'accroche annoncées : `.github/workflows/build.yml` (lancement de `test_systems`, 1 ligne),
  skill `waforspeed-natif` (doc « Ajouter un module »).
- ⚠️ Pour WFS-08 (session trafic) : `CMakeLists.txt` liste maintenant les sources par dossier (`GLOB_RECURSE`) ; à la
  synchronisation, garder le `GLOB_RECURSE` (plus besoin d'ajouter `traffic.cpp` à la main). `game.*` : seulement
  quelques lignes ajoutées (appels `systems*`) et un bloc à la fin de `game.cpp`.

### 2026-11-02 — Fin WFS-30 (Claude, session moteur-natif)
- QA **VALIDÉ** (`qa/rapports/WFS-30.md`) : rendu identique au pixel près, `test_systems` OK. PR #3 fusionnée.
- Débloqués (dépendance WFS-30 terminée) : WFS-06 (E2 ville), WFS-26 (E5 texte), WFS-10 (E6 sons), WFS-07 (E4 ombres) :
  **quatre épics attribuables en parallèle**, en plus de WFS-08 (E3) en cours.
- Pour ajouter un module : skill `waforspeed-natif`, section « Ajouter un module ».

### 2026-11-02 — Début WFS-06 Ville : bâtiments, trottoirs, lampadaires, arbres (attribué par le PO)
Épic **E2 — Monde et décor**. Branche `feature/waforspeed-06`, partie de `main`. Dépendance WFS-30 ✅.
**Zone de code autorisée** : `src/world/**`, `assets/world/**`, `shaders/world_*`. Tout autre changement du noyau passe par E0 (ou une ligne d'accroche annoncée au Journal).
**Critères**
- Décor généré par la graine de la carte (comme decor.js)
- Instanciation GPU, ≤ 300 appels de dessin
- Captures : rue bordée d'immeubles, lampadaires, arbres
**Plan**
- `src/world/` : générateur de décor à partir de la graine (port de decor.js), bâtiments, trottoirs, lampadaires, arbres.
- Instanciation GPU par type d'objet, dessin borné à ≤ 300 appels.
- Module enregistré via le système `System` de WFS-30 : nouveaux fichiers + 1 ligne d'enregistrement dans `Game`.
- Captures de la CI relues par `qa-lead` avant fusion.

### 2026-11-02 — QA WFS-30 par qa-lead : VALIDÉ
- Banc d'essai `course-cam0-2` identique à WFS-05 (7 pixels différents sur 921 600, écart 1/255) : aucun changement visible.
- Captures émulateur sans régression ; logs sans erreur. Limite : GPU logiciel à 0,5 image/s, performances non jugées.

### 2026-11-02 — WFS-06 : code poussé, construction en cours (Claude, session moteur-natif)
- `src/world/ville.cpp` (seul fichier, zone E2) : système `Ville` (ordre 100), immeubles / lampadaires / arbres d'alignement
  placés par la graine de la carte, morceaux construits en parallèle, ≈ 6 appels de dessin par morceau (≤ 150 au total).
- Écart avec le critère « instanciation GPU » : maillages fusionnés par morceau (même objectif : peu d'appels de dessin),
  l'instanciation demanderait de changer le renderer (zone E0/E4).

### 2026-11-02 — WFS-08 : synchronisé avec main, trafic branché en système enfichable (Claude, session trafic)
**Fait**
- Synchronisation avec `main` (livraison 1 + WFS-30) ; conflits résolus en reprenant `main`, puis le trafic est rangé
  dans la zone E3 : `src/game/traffic_system.*` (module `WFS_SYSTEM`, ordre 50) — simulation, chocs, dessin, aide au
  pilote automatique, scènes du banc d'essai `trafic-cam0`, `trafic-cam1`, `trafic-choc`.
- **Lignes d'accroche du noyau (8 lignes)** : `src/core/system.h` + `System::collide(dt, Player&, GameView)` (seul point
  où un système modifie la voiture du joueur : chocs) et `System::autopilot(dt, DriveInput&, GameView)` ;
  `src/game/game.cpp` : leurs deux appels + tête-à-queue affiché (`heading + spinYaw`).
- Hors zone (accroches) : 1 ligne dans `src/platform/desktop_main.cpp`, 2 dans `CMakeLists.txt` (test_traffic),
  1 dans `.github/workflows/build.yml` ; script de référence déplacé dans `tests/trafic_web.cjs`.
- Zone E3 aussi : `src/game/player.*` (accident), `src/game/track.h` (`wrap` identique au JavaScript).

**Vérification (conteneur)** : `test_traffic` 0 écart sur 39 voitures, mode parallèle déterministe, choc OK ;
`test_track`, `test_systems` OK ; tout compile sans avertissement. Construction GitHub en cours, puis QA par `qa-lead`.
**Limite** : le module charge ses propres modèles (la voiture du joueur est en double en mémoire, à partager via E0).

### 2026-11-02 — QA WFS-06 par qa-lead : VALIDÉ
Captures regardées : immeubles des deux côtés, lampadaires et arbres sur les trottoirs, voiture visible, rien de cassé. Constat majeur : la capture `qa-03-acceleration-8s` est identique à `qa-01-demarrage` (pas de preuve de déplacement). Limites : nombre d'appels de dessin et graine non vérifiables sur captures. Rapport : `qa/rapports/WFS-06.md`.

### 2026-11-02 — Fin WFS-06 (Claude, session moteur-natif)
- QA **VALIDÉ par qa-lead** (`qa/rapports/WFS-06.md`), PR #4 fusionnée. Ville : 899 immeubles, 222 lampadaires, 439 arbres.
- Retours QA à traiter plus tard : capture `qa-03` identique à `qa-01` (émulateur trop lent pour rafraîchir l'écran, à
  corriger dans le scénario QA, épic E7) ; lampadaires et arbres peu lisibles de loin (mineur).
- Débloqués dans E2 : WFS-14 (cartes) et WFS-19 (piétons) passent en ⏳ Prêt. WFS-20 attend encore WFS-08.

### 2026-11-02 — Début WFS-31 (décision de l'utilisateur, Claude, session moteur-natif)
L'utilisateur décide : QA de chaque ticket sur le **banc d'essai PC** seulement ; **simulateur Android à la fin de chaque
épic** (ticket « Vérification simulateur EX » préparé par le PO, rapport QA simulateur). Ticket WFS-31 (E7, priorité 0 :
débloque WFS-08 dont la publication échouait sur une capture d'émulateur vide).
Fichiers : `.github/workflows/{build,simulateur}.yml`, `.claude/agents/{qa-lead,po-scrum-master}.md`, `.claude/skills/qa-lead/`, `CLAUDE.md`.

### 2026-11-02 — ⚠️ MESSAGE pour la session trafic (WFS-08), de Claude (session moteur-natif)
1. Ta construction n° 59 (commit `61e2ecf`) a réussi (APK, banc d'essai, émulateur) mais la **publication a échoué** :
   « HTTP 400 Bad Content-Length » sur `qa-01-demarrage.png`, capture d'émulateur **vide** (0 octet). Pas un défaut du trafic.
2. **Décision de l'utilisateur (WFS-31)** : QA de chaque ticket sur le **banc d'essai PC seulement** (`vitrine-*`, `course-*`,
   tes `trafic-*`) ; l'émulateur est retiré de `build.yml` et ne tourne qu'**en fin d'épic** (`simulateur.yml`, ticket
   « Vérification simulateur EX » créé par le PO) ; captures vides ignorées à la publication.
3. **À faire de ton côté** dès que WFS-31 est ✅ dans ce tableau : synchroniser `feature/waforspeed-08` avec `main`
   (contient aussi WFS-06 : `src/world/ville.cpp`, aucun fichier commun avec toi) ; pour `.github/workflows/build.yml`,
   garder la version de `main` (sans job `qa-emulateur`) et y remettre ta ligne `test_traffic` dans l'étape des tests ;
   puis lancer l'agent `qa-lead` sur les captures du banc PC.

### 2026-11-02 — Fin WFS-31 (Claude, session moteur-natif)
- QA **VALIDÉ par qa-lead** (`qa/rapports/WFS-31.md`), PR #5 fusionnée. Construction ≈ 2 min 40 ; publication sans émulateur.
- ✅ **Session trafic (WFS-08) : tu peux maintenant synchroniser `feature/waforspeed-08` avec `main`** (voir le message plus haut).
- Épics terminées sans vérification simulateur : E1 (vérifiée par WFS-05 sur émulateur), E0 (aucun changement visible) :
  pas de ticket « Vérification simulateur » nécessaire pour elles.

### 2026-11-02 — WFS-08 : PR #6 ouverte (Claude, session trafic)
- Branche synchronisée avec `main` (WFS-06 inclus, aucun conflit). PR : https://github.com/wlfaleh/waforspeed_mobile/pull/6
- Captures banc PC de la construction précédente : `trafic-cam1` (trafic de près) et `trafic-choc` (accident en 0,43 s,
  voiture percutée en glissade, tête-à-queue 1,54 rad) correctes ; `trafic-cam0` refaite (joueur placé sur le trottoir
  par la scène de test, corrigé). Construction `66d27be` en cours, puis QA par `qa-lead` sur les captures du banc PC.

### 2026-11-02 — Début WFS-26 Rendu de texte (police, nombres, libellés) (attribué par le PO)
Épic **E5 — Interface et contenu**. Branche `feature/waforspeed-26`, partie de `main`. Dépendance WFS-30 ✅.
**Zone de code autorisée** : `src/ui/**`, `shaders/ui*`, `assets/fonts/**`. Tout autre changement du noyau (ex. API 2D du renderer dans `src/render/`) passe par une ligne d'accroche annoncée au Journal, ou par E0.
**Critères**
- Police (licence libre) en atlas, texte en pixels logiques, accents français
- Lisible sur 2400x1080 et petits écrans
- Capture : texte d'essai net
**Plan**
- `assets/fonts/` : police libre convertie en atlas, licence incluse.
- `src/ui/` : module de texte (libellés, nombres, accents) en pixels logiques, mise à l'échelle selon la taille réelle de la surface.
- `shaders/ui*` : échantillonnage de l'atlas ; scène d'essai du banc PC pour la capture, relue par `qa-lead`.

### 2026-11-02 — Fin WFS-08 Trafic multi-thread et collisions (Claude, session trafic)
**⚠️ Exception à la règle de QA** : PR #6 fusionnée **à la demande explicite de l'utilisateur, avant le verdict de
`qa-lead`** (construction `66d27be` encore en cours). La QA sur les captures du banc PC reste à faire après coup
(`qa/rapports/WFS-08.md`) ; un éventuel REFUSÉ donnera un ticket de correction.
**Fait** : trafic porté de `traffic.js` (0 écart sur 39 voitures avec le jeu web), mode parallèle, chocs et quasi-contacts,
accident du joueur, module `traffic_system` (dessin, pilote automatique, scènes `trafic-*` du banc d'essai).
**Fichiers** : `src/game/traffic*.{h,cpp}`, `src/game/player.*`, `src/game/track.h`, `tests/test_traffic.cpp`,
`tests/trafic_web*.{cjs,txt}` ; accroches : `src/core/system.h`, `src/game/game.cpp` (8 lignes), `desktop_main.cpp` (1),
`CMakeLists.txt` (2), `build.yml` (1).
**Vérification** : tests (trafic, piste, systèmes) OK en local et en CI ; captures banc PC de `61e2ecf` regardées par le
développeur (trafic de près, choc) — pas de verdict QA.
**Limites** : 16 voitures du jeu web affichées avec le modèle natif le plus proche (WFS-13) ; pas de feux stop ni de
clignotants ; score des quasi-contacts non affiché (WFS-12) ; modèles chargés en double (joueur + trafic).
Tickets qui dépendaient de WFS-08 (WFS-12, WFS-28, WFS-20) : à passer en « Prêt » par le PO.

### 2026-11-02 — PO : tickets débloqués par WFS-08
- WFS-08 ✅ (fusion exceptionnelle, QA après coup par `qa-lead` à faire : `qa/rapports/WFS-08.md`).
- Passent en ⏳ Prêt : **WFS-12** (E3, dépend de WFS-08 ✅) et **WFS-20** (E4, dépend de WFS-06 ✅ et WFS-08 ✅).
- Reste ⏳ En attente : **WFS-28** (E6), qui attend encore WFS-10.

### 2026-11-02 — Début WFS-10 Sons de moteur (AAudio, régime et rapports) (attribué par le PO à Claude, session trafic)
Épic **E6 — Audio**. Branche `feature/waforspeed-10`, partie de `main`. Dépendance WFS-30 ✅.
Choix du PO : ticket ⏳ Prêt de meilleure priorité (5) ; épic E6 libre (E5 occupée par WFS-26, E3 libre mais WFS-12 moins prioritaire).
**Zone de code autorisée** : `src/audio/**`, `assets/audio/**`. Lecture seule de l'état du joueur via `GameView` (WFS-30).
Lignes d'accroche possibles (≤ 10, à annoncer au Journal) : liaison `aaudio` dans `CMakeLists.txt`, pause / reprise du son
dans `src/platform/`. Tout autre changement du noyau passe par un ticket E0.
**Critères**
- Sons du jeu web, régime qui suit l'accélérateur et les rapports
- Pas de craquement, coupure en pause
**Plan**
- `assets/audio/` : sons de moteur du jeu web (`wlfaleh/waaps`, `waforspeed/`) convertis, licence vérifiée.
- `src/audio/` : sortie AAudio (Android) sur son propre fil, sortie muette ou fichier sur le banc d'essai PC ; mélangeur sans
  allocation dans le rappel audio ; régime calculé comme le jeu web (vitesse, rapports, accélérateur), fondu entre boucles.
- Module enregistré par `WFS_SYSTEM` ; coupure en pause / perte de focus.
- Preuve sur le banc PC : journal du régime et du rapport par seconde (+ courbe ou enregistrement WAV en artefact) relue par `qa-lead`.

### 2026-11-02 — WFS-10 : code écrit, construction verte (Claude, session trafic)
**Fait**
- `src/audio/engine_synth.*` : port de `engineUpdate()` / `engineVoice()` du jeu web (boîte à 5 rapports, régime, charge,
  deux voix sur le vrai enregistrement : lever de pied filtrée / en charge saturée, filtres biquad et lissage du Web Audio,
  onduleur pour l'électrique) ; rendu sans allocation ni verrou.
- `src/audio/audio_out_android.cpp` : AAudio mono flottant 48 kHz, faible latence, réouverture si le casque change.
- `src/audio/audio_system.cpp` : module « son » (`WFS_SYSTEM` 300) ; le son s'éteint quand le jeu ne donne plus de
  nouvelles (pause, arrière-plan) — pas d'accroche nécessaire dans `src/platform/`.
- Preuve pour la QA (banc PC) : `moteur-courbe.png` (forme d'onde, régime, rapport, vitesse, pédale), `moteur.wav`,
  journal du banc (une ligne par seconde).
- **Lignes d'accroche** : `CMakeLists.txt` (+ `aaudio`, + cible `test_audio`), `.github/workflows/build.yml` (+ `test_audio`),
  `src/platform/desktop_main.cpp` (+ 2 lignes : appel du banc), `tests/test_audio.cpp` (nouveau test).
**Vérification** : `test_audio` OK (rapports 1 → 5, plus grand saut 0,06 : pas de craquement, crête 0,23, silence en pause) ;
construction `c719e79` verte (APK arm64 + x86_64 avec AAudio, banc d'essai). Correction du scénario du banc (volant) en
cours de construction.
**Limites** : la pédale est déduite de la conduite (le module ne lit que `GameView`) : un coup de gaz à l'arrêt sans avancer
ne fait pas monter le régime ; son réel sur téléphone (latence, volume) non vérifiable en CI.

### 2026-11-02 — WFS-10 : PR #7 ouverte (Claude, session trafic)
Branche synchronisée avec `main` (aucun conflit). PR : https://github.com/wlfaleh/waforspeed_mobile/pull/7 — QA par `qa-lead` après la construction `d65cd1e`.

### 2026-11-02 — Fin WFS-10 Sons de moteur (Claude, session trafic)
**QA** : VALIDÉ par `qa-lead` (`qa/rapports/WFS-10.md`, banc PC, `d65cd1e`), PR #7 fusionnée.
**Fait** : son du moteur porté de `sfx.js` (5 rapports, régime, charge, deux voix sur le vrai enregistrement, filtres et
lissage du Web Audio), sortie AAudio 48 kHz faible latence, coupure en pause, preuve banc PC `moteur-courbe.png`.
**Fichiers** : `src/audio/**`, `tests/test_audio.cpp` ; accroches : `CMakeLists.txt` (aaudio, test_audio), `build.yml`,
`src/platform/desktop_main.cpp`.
**Remarques QA** : licence des sons — elle existe, `assets/licences/sons-moteur.txt` (pas dans `assets/audio/`) ;
`moteur.wav` n'est pas publié (la publication ne prend que les PNG) ; coup de gaz à l'arrêt sans avancer : le régime ne
monte pas (pédale déduite de la conduite) — à reprendre si `GameView` expose la pédale (ticket E0).
**À vérifier en fin d'épic E6** (simulateur / téléphone) : son réel, latence AAudio, coupure en arrière-plan.
WFS-28 (bruitages) n'attend plus WFS-10 : à passer en Prêt par le PO.

### 2026-11-02 — Suite de la QA de WFS-08 : WFS-32 et WFS-33 créés (PO)
Remarques non bloquantes du [rapport QA WFS-08](qa/rapports/WFS-08.md) :
- Remarque 5 (points blancs en trame sur les carrosseries, antérieurs à WFS-08) → **WFS-32** (E4, priorité 5.5, ⏳ Prêt) :
  défaut visible sur toutes les captures, petit, placé avant WFS-07 (aucun ticket E4 actif).
- Remarques 1 et 4 (un seul véhicule sur `trafic-cam0`, peu de trafic sur `course-*`) → **WFS-33** (E3, priorité 7.5,
  ⏳ Prêt, après WFS-12) ; la remarque 2 (arrière de la voiture percutée sur le trottoir) y est fusionnée comme
  vérification : `traffic.cpp` borne déjà l'écart à ±1,12 comme `crashTraffic` du jeu web, le dépassement vient sans
  doute de la demi-longueur de la voiture en travers — à confirmer face au jeu web.
- Remarque 3 (quasi-contacts invisibles) : pas de ticket, couverte par WFS-12 et WFS-27.
Aucune attribution.
Fin de ticket WFS-10 : **WFS-28** (bruitages, E6) a toutes ses dépendances ✅ (WFS-10, WFS-08) → ⏳ Prêt.

### 2026-11-02 — Début WFS-32 Carrosseries : points blancs en trame (Claude, session trafic)
**Attribution (PO)** : ticket ⏳ Prêt le plus prioritaire (5.5) dont l'épic n'a aucun ticket actif (E4 : aucun 🔧/🔍) ;
la session trafic n'a plus de ticket (WFS-10 ✅). Dépendances : aucune (rendu PBR WFS-03 ✅).
**Épic** : E4 Rendu et performances. **Zone de code autorisée** : `src/render/**` (sauf `ui_*`), `shaders/scene*`
(et `shaders/common.glsl` si la correction y est) ; rien dans `src/game/`, `src/world/`, `src/ui/`.
**Branche** : `feature/waforspeed-32`.
**Critères** : cause identifiée et notée au Journal ; plus de points blancs isolés sur `vitrine-golf-0`, `vitrine-p911-1`,
`course-cam0-1`, `trafic-choc` ; vernis toujours brillant, aucune régression ; coût GPU ±0,5 ms.
**Plan court** : reproduire sur les captures du banc PC ; chercher l'aliasing spéculaire du vernis dans `scene.frag`
(normales normalisées, rugosité minimale / filtrage spéculaire de type Toksvig, garde NaN / borne HDR avant tonemapping) ;
vérifier la résolution MSAA ; nouvelle construction puis QA `qa-lead` sur le banc PC.

### 2026-11-02 — WFS-32 : cause trouvée, correction poussée (Claude, session trafic)
**Cause** (capture `vitrine-golf-0`, construction 83 : plaques de points blancs **espacés de 3 pixels**, en 5 × 5, sur la
carrosserie près du feu arrière, le long des arêtes et sur la route sous la voiture) : ce n'est pas le vernis lui-même mais
le **halo de la passe finale** (`shaders/post.frag`), qui additionnait 5 × 5 lectures **espacées de 3 pixels**. Un seul pixel
HDR très brillant (reflet du soleil sur un petit triangle verni, en incidence rasante) allume alors exactement 25 pixels
isolés en grille. Vérifié par le calcul : ancien halo → 25 pixels isolés à pleine intensité ; nouveau → 64 pixels
contigus à 1/4.
**Correction** : `post.frag` — 4 × 4 lectures bilinéaires à cheval sur 2 × 2 pixels, espacées de 2 pixels (carré de 8 × 8
sans trou, 16 lectures au lieu de 25 : un peu moins coûteux), plafonnées à 24 ; `scene.frag` — couleur plafonnée à 64,
NaN / infini remplacés par 0. Vernis inchangé (reflets toujours nets).
**À vérifier** : captures `vitrine-golf-0`, `vitrine-p911-1`, `course-cam0-1`, `trafic-choc` de la construction `4a20e7c`.

### 2026-11-02 — ⚠️ MESSAGE pour toutes les sessions : banc d'essai qui plante au hasard (Claude, session trafic)
**Symptôme** : `wfs_bench` s'arrête juste après « Système : AudioSystem » avec « corrupted size vs. prev_size while
consolidating » (construction de `main` `76265ee`, et `feature/waforspeed-32` `4a20e7c`) ; le même code est passé
d'autres fois : c'est aléatoire.
**Cause** (ThreadSanitizer, banc complet lancé dans le conteneur avec lavapipe) : **course dans `blob()` de
`src/world/ville.cpp`** — ses tableaux statiques étaient remplis par « if (unit.empty()) » depuis plusieurs threads à la
fois (construction parallèle des morceaux de ville, WFS-06). Ni le trafic ni le son.
**Correction** (hors zone de WFS-32, quelques lignes, faute de ticket E2 en cours) : forme calculée une seule fois par une
variable statique locale. Banc complet sous ThreadSanitizer : plus aucune alerte dans notre code (restent 3 alertes
internes au pilote lavapipe). Elle arrive dans `main` avec la PR de WFS-32 ; en attendant, une construction qui plante
juste après « Système : AudioSystem » n'est pas un défaut de votre ticket.
**WFS-32** : cause des points blancs en grille = halo de la passe finale (voir entrée précédente) ; ajout d'un
anticrénelage des reflets. Restent de fines lignes pointillées sur les jointures (triangles plus fins qu'un pixel,
crénelage de géométrie : TAA de WFS-17).

### 2026-11-02 — Fin WFS-32 Carrosseries : points blancs en trame (Claude, session trafic)
**QA** : VALIDÉ par `qa-lead` (`qa/rapports/WFS-32.md`, banc PC, `84b489b` — la construction de référence est `84b489b`,
pas `4a20e7c` cité plus haut). PR #8 fusionnée.
**Fait** : halo de la passe finale sans trou (cause des trames de points), anticrénelage des reflets du vernis, couleur
plafonnée / NaN supprimés ; course dans `blob()` de la ville corrigée (banc d'essai qui plantait au hasard sur `main`).
Points brillants isolés (comptage qa-lead, avant → après) : Golf 66 → 25 (dont en trame 45 → 2), 911 43 → 8 (22 → 0),
course-cam0-1 24 → 8 (14 → 0), trafic-choc 15 → 5.
**Fichiers** : `shaders/post.frag`, `shaders/scene.frag` ; hors zone (annoncé) : `src/world/ville.cpp`.
**Restes (non bloquants, pour le PO)** (reportés le 2026-11-03 : WFS-17 et WFS-42) : lignes pointillées sur les jointures de carrosserie (crénelage de géométrie,
déjà présent avant) → à ajouter aux critères de WFS-17 (TAA) ; trame de petits points clairs sur la **lunette arrière de la
Golf** (vitrage, identique avant/après : texture du modèle ou transparence ?) ; points sur les rayons des jantes (911) ;
coût GPU non mesuré (halo 25 → 16 lectures + 2 dérivées) : à confirmer sur téléphone en fin d'épic E4.

### 2026-11-02 — Règles de gestion par US (demande de l'utilisateur)
- Nouveau dossier `regles-de-gestion/` : un fichier `WFS-XX.md` par US, écrit par le PO (règles RG-xx, éléments à
  réutiliser du jeu web `wlfaleh/waaps` : code `js/`, modèles 3D `models/`, sons `audio/`, critères, hors périmètre).
- Un ticket n'est « Prêt » que si son fichier de règles existe. Le PO rédige les fichiers des tickets ouverts.

### 2026-11-02 — Règles de gestion rédigées par le PO
Fichiers créés dans `regles-de-gestion/` (un par US ouverte, valeurs reprises de `wlfaleh/waaps`, `waforspeed/js/`) :
`WFS-07`, `WFS-09`, `WFS-11`, `WFS-12`, `WFS-13`, `WFS-14`, `WFS-15`, `WFS-16`, `WFS-17`, `WFS-18`, `WFS-19`, `WFS-20`,
`WFS-21`, `WFS-22`, `WFS-23`, `WFS-24`, `WFS-25`, `WFS-26` (en cours : règles pour la revue), `WFS-27`, `WFS-28`,
`WFS-29`, `WFS-33`. WFS-08 est déjà ✅ (rapport QA) : pas de fichier. Tableau et statuts inchangés.
Constat : `LOD_CAR_M` vaut 50 m dans le code (le skill disait 70 m) ; le critère « jointures de carrosserie » du rapport
WFS-32 est reporté dans WFS-17 (TAA).

### 2026-11-02 — WFS-34 créé (PO) : arbres réalistes du jeu web
Retour de l'utilisateur : les arbres de la ville native (WFS-06 ✅) sont simplifiés alors que le jeu web a déjà les siens
(`waaps/waforspeed/js/world/life.js` ~250-690). Ticket de correction **WFS-34** (E2, priorité 4.5, ⏳ Prêt, dépend de WFS-06 ✅) :
port des générateurs en C++ et atlas de feuillage exporté tel quel en PNG via le Chromium de test (pas de redessin).
Règles : `regles-de-gestion/WFS-34.md`. E2 est libre (aucun ticket actif).

### 2026-11-02 — Début WFS-34 Arbres réalistes portés du jeu web (attribué par le PO, pendant la QA de WFS-26, à la demande de l'utilisateur)
Exception voulue par l'utilisateur à la règle « un ticket par agent » : la session moteur-natif prend WFS-34 pendant que WFS-26 est en revue (🔍).
**Zone de code autorisée** : `src/world/**`, `assets/world/**`, `shaders/world_*` (épic E2). Tout autre changement du noyau passe par E0 (ou une ligne d'accroche annoncée au Journal).
**Responsable** : Claude (session moteur-natif), branche `feature/waforspeed-34`. Règles : `regles-de-gestion/WFS-34.md`.
Critères : voir la section « Critères d'acceptation des tickets ouverts » (WFS-34).

### 2026-11-02 — WFS-26 validé par qa-lead (rapport `qa/rapports/WFS-26.md`)
Captures du commit 19c62c6 (`texte-essai-1280`, `texte-essai-2400`, `course-cam0-2`) : accents, chiffres, `1:23.4`, `12 345`, `1,2 km` lisibles, net sur les deux résolutions, trois tailles, alignements gauche et centre, aucun texte parasite en course.
Remarques non bloquantes pour la suite : apostrophe droite (`'`) au lieu de `’` (RG-02) ; alignement droit à montrer sur la planche (à traiter en WFS-27).

### 2026-11-02 — WFS-26 fusionné ; WFS-34 construit, en attente de la CI
**Fait**
- WFS-26 : verdict QA `VALIDÉ`, branche synchronisée avec `main`, PR #9 fusionnée. **E5 libérée.**
- WFS-34 (`feature/waforspeed-34`, poussée) : `src/world/vegetation.{h,cpp}` (port de vegLimb / vegCore / vegCard / vegBroadleaf,
  même ordre des tirages), platanes dans `ville.cpp` (graine `a*13 + côté`), morceaux allégés au-delà de 200 m,
  atlas `assets/world/feuillage.png` exporté du jeu web par `tools/atlas_feuillage.cjs`.
- **Accroche noyau annoncée (≤ 10 lignes)** : `MAT_VTINT` (renderer.h) et `shaders/scene.frag` : la longueur de la normale porte
  la luminosité du sommet (le format de sommet n'a pas de couleur), pas d'inversion de normale en face arrière, couverture
  alpha conservée dans les mipmaps du feuillage. Aucun effet sur les autres matériaux.
**Suite** : captures de la CI → agent qa-lead → fusion si `VALIDÉ`.

### 2026-11-02 — Début WFS-07 Ombres dynamiques du soleil (attribué par le PO, pendant l'attente CI/QA de WFS-34, à la demande de l'utilisateur)
Même exception voulue par l'utilisateur à la règle « un ticket par agent » que pendant la QA de WFS-26.
Ticket choisi : meilleur `⏳ Prêt` d'une épic libre (E2 occupée par WFS-34 ; E4 sans ticket actif).
**Zone de code autorisée** : `src/render/**` (sauf `ui_*`), `shaders/scene*`, `sky*`, `post*`, `shadow*` (épic E4).
Pas de modification de `src/world/**` (WFS-34). Tout autre changement du noyau passe par E0 (ou ligne d'accroche annoncée).
**Responsable** : Claude (session moteur-natif), branche `feature/waforspeed-07`. Règles : `regles-de-gestion/WFS-07.md`.
Plan court : passe de profondeur 2048 (1024 mobile) suivant le joueur (+20 m devant), calage sur les texels, PCF rayon 3,
bias -0,0003 / normalBias 0,035 ; ombre de contact conservée la nuit ; coût mesuré au journal du banc.
**PO** : WFS-26 ✅ → **WFS-11** (E5, garage) passe en `⏳ Prêt`.

### 2026-11-02 — WFS-07 construit, en attente de la CI
**Fait** (`feature/waforspeed-07`, poussée) : passe d'ombre du soleil avant la scène (objets opaques mis en file), carte
2048 (1024 Android), boîte du jeu web (40 m, 260 m, 20 m devant, calage texels), 9 lectures comparées, ombre de contact
0,55 / 0,62 / 0,7, coût GPU au journal ; captures `ombres-a`, `ombres-b` (1 m d'écart), `ombres-coupees`.
**Accroches hors zone annoncées** : `src/game/game.cpp` (2 lignes : soleil `[-0.45, 0.75, -0.5]` du jeu web, point suivi = voiture ;
intensité 3,2 gardée, réglée pour notre PBR), `CMakeLists.txt` (2 shaders), `src/platform/desktop_main.cpp` (2 lignes : captures).
Conflit possible avec WFS-34 sur `shaders/scene.frag` (lignes différentes) : réglé à la synchronisation.
**Suite** : CI → qa-lead → fusion si `VALIDÉ`.

### 2026-11-02 — WFS-34 refusé par la QA, corrigé
**Retours QA** (`qa/rapports/WFS-34.md`) : blocs sombres à facettes (cœur du houppier) et feuilles délavées (reflet du ciel).
**Fait** (`feature/waforspeed-34`, 68ffdaf) : feuillage éclairé sans reflet comme le Lambert du jeu web, cœur lissé et éclairci,
nombre d'appels de dessin de la ville au journal du banc. **Suite** : CI → nouvelle QA.

### 2026-11-02 — Début WFS-12 Règles de course (attribué par le PO)
Ticket choisi : meilleur `⏳ Prêt` d'une épic libre (E2 occupée par WFS-34, E4 par WFS-07 ; E3 sans ticket actif, WFS-08 ✅).
**Zone de code autorisée** : `src/game/**` sauf `game.*` (nouveau système de course, ex. `src/game/course.{h,cpp}`), `tests/**` (épic E3).
Pas de HUD (WFS-27, E5) ; décompte « 3 / GO » via le texte de WFS-26 seulement par une accroche annoncée, sinon au journal du banc.
Tout autre changement du noyau passe par E0 (ou ligne d'accroche ≤ 10 lignes annoncée ; attention, WFS-07 touche aussi `game.cpp`).
**Responsable** : Claude (agent développeur, session 01UY1Vv1), branche `feature/waforspeed-12`. Règles : `regles-de-gestion/WFS-12.md`.
Critères : course automatique du banc identique au JS (temps, checkpoints, score, tolérance nulle sur les entiers) ;
capture « 3 » puis « GO » ; test unitaire checkpoint 1 → +11 s, checkpoint 10 → +7 s, quasi-contact ×3 → 750 points.
Plan court : porter `resetRace` / `update` / `hitCar` / `playerAccident` / `crash` du jeu web dans un système `Course`
branché sur les collisions existantes de WFS-08 ; constantes de `config.js` et `carPerf` ; tests de référence dans `tests/`.

### 2026-11-03 — WFS-12 construit, en attente de la CI
**Fait** (`feature/waforspeed-12`, poussée) : `src/game/course.{h,cpp}` (port de la fin de `update()`, `playerAccident`, `bump`,
`crash`, `resetRace`, `score`, `kmh`) et système `CourseSystem` (ordre 210) dans `src/game/course_system.{h,cpp}` : décompte
3-2-1-GO (voiture immobile, texte de WFS-26 depuis le système), chrono 30 s, checkpoints tous les 600 m (+6 voitures si < 110),
combos de quasi-contacts, combo perdu au-delà de 0,08, accident (ralenti ×0,35 pendant 0,5 s), fin de partie avec écran
(score, distance, vitesse max, quasi-contacts, checkpoints) et bouton « Rejouer » (même graine de trafic).
Trafic (`traffic.*`, même épic) : événements des chocs dans l'ordre (`HitEvents.seq`), `Traffic::watch` pendant le décompte.
**Vérification** : `tests/test_course.cpp` contre `tests/course_web_ref.txt` (produit par `node tests/course_web.cjs`, qui exécute
`update.js` / `state.js` du jeu web) : 2 courses automatiques complètes (ville/Golf 7 checkpoints 24 000 pts ; circuit/M4
11 checkpoints 37 480 pts), **22 lignes, 0 écart** (pas, temps restant à 4 décimales, score, quasi-contacts) ; règles unitaires OK ;
`test_traffic` toujours OK. Compilation vérifiée ici (`-fsyntax-only`) ; banc Vulkan et APK : CI.
**Accroches hors zone annoncées (8 lignes)** : `src/game/game.cpp` (2 : include + `dt = courseScaleDt(...)`, ralenti),
`src/platform/desktop_main.cpp` (3 : include, `courseBenchRules(false)` pour garder les scènes des autres tickets sans décompte
ni fin, `benchCourseScene`), `CMakeLists.txt` (2 : `test_course`), `build.yml` (1 : lancement de `test_course`).
**Remarque pour le PO** : le critère « checkpoint 10 → +7 s » ne correspond pas à la règle RG-02 : `max(5, 11 − 9×0,4)` = 7,4 s ;
+7 s est le checkpoint 11. Le test vérifie les deux valeurs exactes du jeu web.
**Limites** : collisions avec le décor (`crash`) portées et testées mais pas encore déclenchées (aucun décor percutable natif) ;
graine aléatoire par course avec le menu (WFS-11) ; HUD en continu : WFS-27.
**Suite** : CI → qa-lead → fusion si `VALIDÉ`.

### 2026-11-03 — WFS-07 refusé (sol sans ombre), WFS-34 refusé 2e fois (cœurs) : corrigés
- WFS-34 (deb85f2) : cœur du houppier réduit (×0,7) et teinté comme le feuillage à l'ombre (moyenne de l'atlas × teinte × 0,45).
- WFS-07 (6743fbc) : cause trouvée, antérieure au ticket : les triangles du sol sont enroulés à l'envers, le shader
  retournait leur normale vers le bas → route, trottoirs et bas-côtés ne recevaient jamais le soleil direct (ni l'ombre).
  Corrigé dans `scene.frag` (pas de retournement pour ROAD/WALK/GROUND). La route sera plus claire au soleil : attendu.
**Suite** : CI → nouvelles QA.

### 2026-11-03 — PO : menu et chargement priorisés en tête du backlog
**Besoin (utilisateur)** : prioriser le menu, l'implémenter petit à petit en petites US, avec le chargement de l'application avant tout.
**Étude** : le jeu web n'a pas d'écran de chargement d'application (seulement « Chargement du modèle 3D… » dans le garage) ; on
réutilise son style (logo `#logo`, `.eyebrow`, `.screen`, dégradé `.cp-bar`, fondu `screen-in` 300 ms, message d'échec de `boot()`).
En natif, `Game::init` charge tout d'un bloc (écran noir) ; aucun état menu / course : il faut un point d'extension E0.
**Décisions**
- Nouvelle épic **E8 « Écrans et menus »**, zone `src/ui/ecrans/**` (retirée de E5), pour avancer le menu **en parallèle** du
  garage et du HUD (E5).
- US créées (règles dans `regles-de-gestion/`) : WFS-35 chargement (0.1, E8), WFS-36 écrans de l'application (0.2, E0, catalyseur),
  WFS-37 menu affichage (0.3, E8), WFS-38 menu tactile (0.4, E8), WFS-39 pause (0.6, E8), WFS-40 fiche et peintures (0.7, E5),
  WFS-41 terrain (0.8, E8).
- **WFS-11 redécoupé** : ne garde que la scène 3D du garage et le changement de voiture (priorité 0.5, dépend maintenant de WFS-36,
  donc ⏳ En attente) ; fiche et peintures → WFS-40 ; menu → WFS-37 / WFS-38. WFS-13 dépend toujours de WFS-11.
- Parallélisme : WFS-36 (E0) est ⏳ Prêt pour un autre agent pendant WFS-35 (E8).

### 2026-11-03 — Début WFS-35 Écran de chargement (attribué par le PO, réservé)
Ticket choisi : meilleure priorité du backlog, épic E8 neuve donc libre ; dépendance WFS-26 ✅.
**Responsable** : Claude (agent développeur, session 01UY1Vv1), branche `feature/waforspeed-35`. Règles : `regles-de-gestion/WFS-35.md`.
**Règle « un ticket par agent »** : WFS-12 a son code fini et attend la CI puis la QA. WFS-35 lui est **réservé** et ne démarre
qu'une fois WFS-12 en 🔍 En revue (CI verte, PR ouverte, qa-lead lancé). Si la QA de WFS-12 rend REFUSÉ, ses corrections passent
avant WFS-35. Même exception que pendant la QA de WFS-26 et de WFS-07.
**Zone de code autorisée** : `src/ui/ecrans/chargement.{h,cpp}` (E8). Accroches annoncées ≤ 10 lignes : progression facultative de
`Game::init` (`src/game/game.{h,cpp}`, attention WFS-07 / WFS-12 y touchent aussi), chargement public de la police
(`src/ui/text.*`), appel de l'écran dans `src/platform/android_main.cpp` et `desktop_main.cpp` (captures `chargement-*`).
Critères : voir le tableau ci-dessus (captures `chargement-0/1/2`, 2400x1080, `chargement-fondu`, `chargement-erreur`, journal des durées).
Plan court : écran dessiné avant tout chargement lourd, 4 étapes pondérées (10 / 20 / 70 / 100 %), fondu 300 ms, message d'échec.

### 2026-11-03 — WFS-12 en revue (PR #10) ; début de WFS-35
**WFS-12** : PR #10 ouverte, première CI verte (banc, APK, publication ; journal du banc : course de référence identique au jeu web,
captures `course-decompte-3`, `course-decompte-go`, `course-fin`). Deux corrections poussées depuis : score et distance de l'écran
de fin figés à la fin (ils continuaient de monter pendant que la voiture roulait seule), couleurs de l'écran de fin en codes sRGB
(`#0A0B10`, `#14161F`, comme `ui::CYAN`). **Suite** : CI → qa-lead → fusion si `VALIDÉ`.
**WFS-35 démarré** (`feature/waforspeed-35`, poussée) : `src/ui/ecrans/chargement.{h,cpp}` — écran dessiné juste après
l'initialisation Vulkan (police puis 0 %), logo « WAFOR / SPEED » (FOR en contour de 2 px), sur-titre cyan, barre dégradée
cyan → orange (16 tranches), étapes 10 / 20 / 70 / 100 %, fondu de 300 ms (système `FonduChargement`, ordre 490), message
d'échec rouge, durée de chaque étape au journal ; banc : `chargement-0/1/2`, `-fondu`, `-erreur` (fichiers introuvables).
**Accroches annoncées (13 lignes, 3 de plus que les 10 prévues)** :
- `game.h` (1 : paramètre `progression` de `Game::init`) ;
- `game.cpp` (4 : signature + 3 étapes) ;
- `text.h/.cpp` (2 : `ui::chargerPolice`) ;
- `android_main.cpp` (4 : include, instant Vulkan, `debut`, `init` avec progression / `erreur`, `fin`) ;
- `desktop_main.cpp` (3 : include, instant Vulkan, `benchChargement`).
**Limite** : `chargement-1` montre l'étape « Chargement du modèle 3D… » à son début (20 %) : le chargeur de modèles (noyau)
ne donne pas d'avancement pendant le décodage ; un avancement plus fin demanderait un ticket E0.

### 2026-11-03 — WFS-07 fusionné (PR #11)
- QA 2e passage `VALIDÉ` (`qa/rapports/WFS-07.md`) : ombres des immeubles, voitures et lampadaires sur la chaussée et les
  trottoirs, bords stables, pas d'acné. **E4 libérée.** WFS-09 (nuit) et WFS-15 (qualité) débloqués côté dépendance.
- À mesurer en fin d'épic E4 (simulateur / téléphone) : coût de la passe d'ombre ≤ 3 ms avec la carte 1024.
- WFS-34 : 4e correction (cœur en feuillage découpé), en attente de CI puis QA.

### 2026-11-03 — PO : restes de WFS-32 reportés, WFS-09 et WFS-15 prêts
- Lignes pointillées des jointures de carrosserie → critère ajouté à **WFS-17** (déjà dans `regles-de-gestion/WFS-17.md`).
- Trame de points sur la lunette arrière de la Golf et points sur les rayons des jantes (911) → nouveau ticket **WFS-42** (E4,
  priorité 8, ⏳ Prêt, règles `regles-de-gestion/WFS-42.md`) ; le coût GPU du halo reste à mesurer en fin d'épic E4.
- WFS-07 ✅ : **WFS-09** (nuit) et **WFS-15** (qualité) passent en ⏳ Prêt. E4 est libre : WFS-42 puis WFS-09 sont attribuables.

### 2026-11-03 — Début WFS-36 Point d'extension : écrans de l'application (attribué par le PO à Claude, session trafic)
Ticket choisi : meilleur ⏳ Prêt du backlog (priorité 0.2), épic E0 sans ticket actif, aucune dépendance ; l'agent n'a plus de
ticket (WFS-08, WFS-10, WFS-32 ✅). Catalyseur : débloque WFS-37 (E8) et WFS-11 (E5). Branche `feature/waforspeed-36`.
**Zone de code autorisée (E0)** : `src/core/**` (nouveau `ecran.h`, `system.h` : `onEcran`), `src/game/game.{h,cpp}` (`GameView::ecran`,
`allerA`, application entre deux images, conduite et commandes tactiles coupées hors `Course`, `Pause` sans `update`),
`src/render/renderer.h` si besoin, `CMakeLists.txt` ; doc du point d'extension dans `.claude/skills/waforspeed-natif`.
Accroche annoncée ≤ 10 lignes : capture `ecran-menu-vide` dans `src/platform/desktop_main.cpp` (banc). Attention : WFS-35
(E8, accroches dans `game.*` et `desktop_main.cpp`) et WFS-12 (en revue) touchent aussi `game.*` : se synchroniser avec `main` souvent.
**Critères** : voir le tableau (captures du banc identiques, écran `Course` par défaut ; tests de trafic et de course à 0 écart ;
capture `ecran-menu-vide` sans commandes tactiles, voiture immobile ; doc dans le skill). Règles : `regles-de-gestion/WFS-36.md`.
**Plan court** : `enum class Ecran` dans `src/core/ecran.h` ; `GameView::ecran` + `allerA(Ecran)` (demande mise en attente,
appliquée en début d'image, `onEcran(ancien, nouveau)` à tous les systèmes) ; hors `Course` commandes remises à zéro et
`drawControls` sauté ; `Pause` saute les `update` ; écran initial `Course` ; capture du banc forcée ; doc ; QA par `qa-lead`.

### 2026-11-03 — WFS-36 : code écrit, PR #13 ouverte (Claude, session trafic)
**Fait** : `src/core/ecran.h` (`Ecran`, `nomEcran`) ; `src/core/system.h` (`GameView::ecran`, `GameView::allerA`,
`System::onEcran(ancien, nouveau, vue)`) ; `Game` (`allerA`, `ecran`, changement appliqué au début de `update` / `render`,
commandes lâchées, hors `Course` : aucune commande, touchers non lus, commandes non dessinées, voiture immobile ;
`Pause` : simulation figée ; départ `Course`, remis à chaque `init`) ; accroche banc d'essai (5 lignes) : capture
`ecran-menu-vide` ; doc dans le skill `waforspeed-natif`.
**Vérification (conteneur, lavapipe)** : banc complet de `main` et de la branche : **26 captures identiques au pixel près** ;
`ecran-menu-vide` : « Écran : course → menu », 0,0 km/h, scène sans commandes tactiles ; tests piste, systèmes, trafic
(0 écart), son OK. PR : https://github.com/wlfaleh/waforspeed_mobile/pull/13
**Limites** : pas encore de test de course (WFS-12 en revue) ; RG-05 (course qui repart en passant à `Course`) via
`onEcran` dans `CourseSystem` au moment de WFS-38.

### 2026-11-03 — Fin WFS-36 Écrans de l'application (Claude, session trafic)
**QA** : VALIDÉ par `qa-lead` (`qa/rapports/WFS-36.md`, banc PC, `e3d7ea4`) — 27 captures sur 27 identiques au pixel près à
celles de `main`, `ecran-menu-vide` conforme. PR #13 fusionnée.
**Fait** : `Ecran` (`src/core/ecran.h`), `GameView::ecran` / `allerA`, `System::onEcran`, règles du noyau (hors `Course` :
aucune commande, touchers non lus, commandes non dessinées, voiture immobile ; `Pause` : simulation figée ; départ `Course`).
**Fichiers** : `src/core/{ecran,system}.h`, `src/game/game.{h,cpp}`, `src/platform/desktop_main.cpp` (accroche 5 lignes),
`.claude/skills/waforspeed-natif/{SKILL.md,references/architecture.md}`.
**Débloque** : WFS-37 (menu, E8) et WFS-11 (garage, E5) — à passer en Prêt par le PO. RG-05 (course qui repart en passant
à `Course`) : `CourseSystem` via `onEcran` (WFS-38).

### 2026-11-03 — WFS-34 fusionné (PR #14)
- QA 4e passage `VALIDÉ` (`qa/rapports/WFS-34.md`) : platanes du jeu web, cœur du houppier en feuillage découpé,
  pas de feuillage délavé, ville à 160 appels de dessin. **E2 libérée.** Plus aucun ticket en cours pour cette session.
- Restes non bloquants (pour le PO) : branches en « baleines de parapluie » (comme le jeu web), cœur un peu visible au bord
  d'un houppier (trafic-cam0) ; ombres d'arbres peu visibles sur les captures (rues à l'ombre des immeubles).

### 2026-11-03 — Nouvelle règle QA : anomalies bloquantes ou mineures (demande de l'utilisateur)
Le QA ne doit plus bloquer un ticket pour de petits problèmes. **Le QA décide de la gravité** de chaque anomalie :
**bloquante** → `REFUSÉ`, le développeur corrige jusqu'à ce qu'il n'y en ait plus ; **mineures seulement** →
`VALIDÉ AVEC RÉSERVES`, fusion autorisée, anomalies inscrites au **tableau des anomalies** (ANO-XX) ; le **PO** les priorise
et crée des US de correction dédiées. Mis à jour : `CLAUDE.md`, `.claude/agents/qa-lead.md`, `.claude/skills/qa-lead/SKILL.md`,
`.claude/agents/po-scrum-master.md`, `.claude/skills/waforspeed-natif/SKILL.md`, ce fichier (règle 5, rôles, statuts,
définition de « Fini », nouveau tableau des anomalies).

### 2026-11-03 — Règle QA précisée : deux tours de correction pour les anomalies mineures (demande de l'utilisateur)
Anomalies **mineures** seulement → verdict `À CORRIGER` : le QA donne la main au développeur **deux fois au plus**.
Si elles restent après le 2e tour, le QA les crée au tableau des anomalies et accepte avec `PASSABLE AVEC ANOMALIES MINEURES`
(fusion autorisée) ; le PO crée ensuite les US de correction. Anomalie **bloquante** → `REFUSÉ`, sans limite de tours.
Remplace le verdict « VALIDÉ AVEC RÉSERVES » de l'entrée précédente.

### 2026-11-03 — PO : restes de WFS-34, WFS-11 prêt, début WFS-42 (attribué à Claude, session moteur-natif)
- **Restes de WFS-34** → ANO-01 et ANO-02 planifiées dans **WFS-43** (E2, priorité 17.5, ⏳ Prêt, `regles-de-gestion/WFS-43.md`) ;
  ANO-03 (baleines de parapluie) abandonnée : identique au jeu web.
- **WFS-36 ✅** : **WFS-11** (garage, E5) passe en ⏳ Prêt (WFS-26 et WFS-36 ✅). WFS-37 attend encore WFS-35.
- **Fin d'épic** : aucune nouvelle épic terminée (E0 entièrement ✅ mais sans changement visible : pas de vérification simulateur,
  décision du 2026-11-02 maintenue).
- **Début WFS-42** (E4) : meilleur ⏳ Prêt d'une épic libre (WFS-33 attend : E3 occupée par WFS-12 en revue ; E8 occupée par WFS-35).
  Responsable : Claude (session moteur-natif), branche `feature/waforspeed-42`, règles `regles-de-gestion/WFS-42.md`.
  **Zone de code autorisée** : `src/render/**` (sauf `ui_*`), `shaders/scene*` ; cause dans le modèle → noter, ticket E5 via le PO.
  **Critères** : cause notée au Journal avec capture ; `vitrine-golf-0` sans trame sur la lunette arrière ; `vitrine-p911-1` sans
  points sur les rayons ; vitres teintées, jantes métalliques ; coût GPU ±0,5 ms.
  **Plan court** : agrandir les deux captures, isoler la cause (matériau vitrage / spéculaire sous-pixel des rayons), appliquer
  l'anticrénelage spéculaire de WFS-32 aux vitres et métaux de `scene.frag`, banc, QA par `qa-lead`.

### 2026-11-03 — WFS-42 construit, en attente de la CI
**Cause trouvée (RG-01)** : géométrie plus fine qu'un pixel, non préfiltrée. Golf : pièce 17 (fils de la lunette arrière,
triangles de 2 mm, transparents, métal 1 / rugosité 0) → pointillés brillants (`vitrine-golf-0`). 911 : matériau 4 (jantes,
21 778 triangles de 1,9 mm, métal) → chaque triangle accroche le soleil (`vitrine-p911-1`).
**Fait** (`feature/waforspeed-42`) : finesse de chaque matériau mesurée au chargement (journal), opacité x couverture du
pixel pour les pièces transparentes fines, reflet du soleil élargi pour les pièces opaques fines. Modèles inchangés (RG-02).
**Suite** : CI → qa-lead.

### 2026-11-03 — QA WFS-12 : À CORRIGER (tour 1/2)
**qa-lead** (`qa/rapports/WFS-12.md`), commit `12662d9`, run 41605341761, Release `apercu-waforspeed-12`.
- Critères remplis : course de référence identique au jeu web (22 lignes, 0 écart ; référence regénérée par le QA depuis
  `wlfaleh/waaps` : identique), décompte « 3 » / « GO » visible, écran de fin complet, tests RG-01 à RG-07 OK.
  Pas de régression : vitrine, course, ombres, texte identiques au pixel à `main` ; trafic déplacé par les règles de course (attendu).
- Anomalies **mineures** : ANO-04 couleurs du décompte inversées (web : chiffres blancs à halo cyan, « GO » orange) ;
  ANO-05 écran de fin (bouton droit au lieu de parallélogramme, sur-titre gris au lieu de cyan, chiffres non italiques).
- **Pour le PO** : le critère « checkpoint 10 → +7 s » contredit RG-02 et le jeu web (+7,4 s ; +7 s au checkpoint 11) :
  critère à corriger, pas le code.
**Suite** : le développeur corrige ANO-04 / ANO-05, CI, 2e passage QA. Pas de fusion avant.

### 2026-11-03 — WFS-42 refusé (1er passage) puis corrigé
**Retour QA** (`qa/rapports/WFS-42.md`) : trame de la lunette et points des jantes inchangés ; coût de la passe de scène non mesuré.
**Vraie cause** : le ciel reflété ajoute le disque du soleil (60 x l'intensité) selon la seule rugosité du matériau ;
sur des centaines de petits points / facettes en miroir, certains renvoient le soleil → pics HDR plafonnés à 64
(baisser l'opacité ne suffisait pas : 64 x 0,1 reste saturé). **Fait** : pour les pièces fines, ciel et disque du soleil
reflétés suivent la rugosité effective ; seuil de finesse 4 mm ; coût de la passe de scène au journal.

### 2026-11-03 — QA WFS-35 : À CORRIGER (tour 1/2)
**qa-lead** (`qa/rapports/WFS-35.md`), commit `cfd7088`, run 41605344614, Release `apercu-waforspeed-35`.
- Critères remplis : `chargement-0/1/2` (couleurs mesurées au pixel conformes, barre 0 / 20 / 100 %, mise en page RG-05),
  `chargement-fondu` (ville visible à 50%), `chargement-erreur` (message rouge, pas de plantage), journal (durée par étape et
  total ; première image 0,046 à 0,071 s après Vulkan). 2400x1080 vérifié sur une construction du QA (non publié par la CI).
  Pas de régression : les 28 autres captures sont identiques au pixel près à `apercu` (`main` 3278fa4).
- Anomalies **mineures** : ANO-06 superpositions visibles pendant le fondu (traits dans la barre, « FOR » qui paraît plein) ;
  ANO-07 « 3D » qui se lit « 30 » à 720 p ; ANO-08 échec attribué à « Construction de la ville » au lieu du modèle 3D (barre à 70 %) ;
  ANO-09 captures `chargement-*` en 2400x1080 non publiées par la CI.
**Suite** : le développeur corrige ANO-06 à ANO-09, CI, 2e passage QA. Pas de fusion avant.

### 2026-11-03 — WFS-12 et WFS-35 : corrections du tour 1/2 de QA poussées
**WFS-12** (`f228e46`) — ANO-04 : décompte blanc #F2F4F8 avec halo cyan, « GO » orange avec halo orange, sans ombre sombre ;
ANO-05 : bouton Rejouer en parallélogramme (biais 14 px du `.btn`), « TEMPS ÉCOULÉ » cyan espacé 0,22 em, score orange avec halo.
Italique impossible dans le texte natif (`src/ui`, hors zone E3) : laissé au jugement du QA.
**WFS-35** — ANO-06 : tranches de la barre bord à bord au pixel, « FOR » en `--muted` et sans halo pendant le fondu ;
ANO-07 : libellé à 19 px (720 p) ; ANO-08 : `Game::init` s'arrête dès que la voiture manque (si écran de chargement), l'échec
reste sur « Chargement du modèle 3D… » ; ANO-09 : `build.yml` publie `chargement-*-2400.png`.
**Accroches WFS-35** : +2 lignes (`game.cpp` 1, `build.yml` 1), soit 15 au total (annoncé).
**Suite** : CI → 2e passage QA (tour 2/2) des deux tickets.

### 2026-11-03 — PO : WFS-36 terminé ; début WFS-11 Garage 3D (attribué par le PO à Claude, session trafic)
**Déblocages** : WFS-36 ✅ (PR #13). **WFS-11** (E5) était déjà ⏳ Prêt ; **WFS-37** (E8) reste ⏳ En attente de WFS-35
(🔧 corrections QA, tour 2/2).
**Choix** : meilleur ⏳ Prêt dont l'épic est libre (WFS-11, priorité 0.5 ; E0, E3, E4, E8 occupées ou sans Prêt devant) ;
l'agent n'a plus de ticket. Branche `feature/waforspeed-11`. Règles : `regles-de-gestion/WFS-11.md`.
**Zone de code autorisée (E5)** : `src/ui/**` sauf `src/ui/ecrans/` (nouveau `src/ui/garage.*` : système enfichable WFS-30
réagissant à `onEcran(Ecran::Garage)` de WFS-36), `shaders/ui*`, `assets/cars/**`. Pas de changement du noyau ni de
`src/render/**` : si l'API du renderer ne suffit pas (scène dédiée, caméra du garage), le signaler au PO pour un ticket E0.
Accroche annoncée ≤ 10 lignes : captures `garage-*` dans `src/platform/desktop_main.cpp` (banc).
**Réutiliser** (`wlfaleh/waaps`, `waforspeed/`) : `js/world/garage.js` (`buildGarage`, `setGarageCar`, `renderGarage` : mêmes
dimensions, lumières, caméra, vitesses), `js/cars/catalog.js` (`CARS`), modèles `.wfm` déjà convertis (Golf 8, M4, 911,
Gallardo), texte WFS-26. Aucune voiture générée par le code.
**Critères** : captures `garage-*` (plateau rayon 3,6 m, anneau néon `glow`, 3 projecteurs, 3 angles) ; `garage-golf` puis
`garage-m4` (nom et index « 01 / 04 » → « 02 / 04 ») ; lisible en 1280x720 et 2400x1080, accents corrects ; autres captures
du banc inchangées (écran `Course` par défaut).
**Plan court** : scène du garage (plateau, sol, anneau, lumières, brouillard) → voiture tournante 0,35 rad/s → glisser /
glissement rapide et flèches → nom, index, « Retour » → captures du banc → PR, CI, QA par `qa-lead`.

### 2026-11-03 — PO : WFS-11 bloqué ; nouveaux tickets WFS-44 (E0) et WFS-45 (E4) ; début WFS-44 (attribué à Claude, session trafic)
**Blocage WFS-11** (signalé par la session trafic, comme prévu à l'attribution) : un système ne peut que dessiner en plus
dans la scène de `Game` ; il ne peut ni choisir la caméra (`Game::render` calcule seul poursuite / orbite et FOV 58-70°, RG-02
demande FOV 36, distance 1,9×L, regard y = −0,35), ni régler fond / ciel / brouillard / soleil (`FrameParams` en dur, ciel de
jour, brouillard 0,0009 ; RG-01 demande `#07070c` et brouillard 14-40 m), ni masquer route, ville, trafic et voiture du joueur ;
et le renderer n'a qu'un soleil + ambiance (RG-02 : hémisphère + 3 projecteurs). WFS-11 → ⛔ Bloqué, **responsable libéré**
(un ticket par agent) ; dépendances ajoutées : WFS-44, WFS-45. La session trafic le reprendra en priorité une fois débloqué.
**Nouveaux tickets** : WFS-44 (E0, 0.45) scène dédiée par écran ; WFS-45 (E4, 0.46, ⏳ Prêt, attribuable dès la fin de WFS-42)
spots + hémisphère + brouillard linéaire, réutilisable par WFS-09 (phares). Les deux sont indépendants : en parallèle.

**Début WFS-44** — Épic **E0**, libre (WFS-30 et WFS-36 ✅). Responsable : Claude (session trafic), 2026-11-03.
Branche `feature/waforspeed-44`, partie de `main`. Règles : `regles-de-gestion/WFS-44.md`.
**Zone de code autorisée** : `src/core/**`, `src/game/game.*`, `src/render/renderer.h` (API seulement), `CMakeLists.txt` ;
accroches annoncées ≤ 10 lignes : arrêt du dessin si `sceneDediee` dans la ville (`src/world/`) et le trafic
(`src/game/traffic_system.cpp`), système de test + capture `scene-dediee-vide` dans `src/platform/desktop_main.cpp`.
Ne pas toucher aux shaders ni au reste de `src/render/**` (WFS-45, E4).
**Critères** : captures du banc identiques par défaut, tests à 0 écart ; `scene-dediee-vide` (fond uni `#07070c`, rien de la
course, cube repère en FOV 36°) ; doc dans le skill `waforspeed-natif`.
**Plan court** : `SceneDediee` + `System::scene` + `GameView::sceneDediee` → `Game::render` les applique avant `beginFrame` →
accroches ville / trafic → système de test du banc → captures → PR, CI, QA par `qa-lead`.

### 2026-11-03 — WFS-44 : code écrit, PR #15 ouverte (Claude, session trafic)
**Fait** : `System::scene(fp, s, vue)`, `SceneDediee { route, joueur, commandes, cible }`, `GameView::sceneDediee` ;
`Game::render` donne la scène au premier système qui la prend (caméra, fond, brouillard, soleil, ambiance, ombres) et ne
dessine plus route / voiture du joueur / commandes sauf demande ; système de test `src/core/scene_essai.*` (inactif par
défaut) ; doc dans le skill `waforspeed-natif`.
**Lignes d'accroche** : `src/world/ville.cpp` (1 ligne), `src/game/traffic_system.cpp` (1 ligne) : pas de dessin pendant une
scène dédiée ; `src/platform/desktop_main.cpp` (2 lignes) : capture `scene-dediee-vide`.
**Vérification (conteneur, lavapipe)** : banc de `main` et de la branche : 26 captures identiques au pixel près ;
`scene-dediee-vide` : cube seul, fond (7, 7, 13) pour `#07070c` ; tests piste, systèmes, trafic (0 écart), son OK.

### 2026-11-03 — QA WFS-12 : PASSABLE AVEC ANOMALIES MINEURES (2e passage, tour 2/2)
**qa-lead** (`qa/rapports/WFS-12.md`), commit `f228e46`, run 41609393349, Release `apercu-waforspeed-12` mise à jour.
- Corrigé : décompte blanc à halo cyan et « GO » orange à halo orange, sans ombre décalée ; écran de fin avec bouton en
  parallélogramme, sur-titre cyan, score orange à halo. Course de référence toujours à 0 écart ; autres captures identiques.
- Reste **ANO-04** (mineure, 🆕 au tableau des anomalies) : italique absent (décompte, score, stats), à faire dans `src/ui/text.*`
  (hors zone E3, donc impossible dans un 2e tour de WFS-12). **Fusion autorisée.** Au PO : US de correction pour ANO-04, et
  critère « checkpoint 10 → +7 s » à corriger en +7,4 s.

### 2026-11-03 — QA WFS-35 : PASSABLE AVEC ANOMALIES MINEURES (2e passage, tour 2/2)
**qa-lead** (`qa/rapports/WFS-35.md`), commit `dff7464`, run 41609812174, Release `apercu-waforspeed-35` mise à jour.
- Corrigé : ANO-06 (barre sans traits, « FOR » gris uni et SPEED sans halo pendant le fondu), ANO-07 (« 3D » lisible à 19 px),
  ANO-08 (échec sur « Chargement du modèle 3D… », barre à 20 %), ANO-09 (`chargement-*-2400.png` publiées, conformes).
  Première image 0,045 à 0,058 s après Vulkan ; les 28 autres captures identiques au pixel près à `main`.
- Reste **ANO-10** (mineure, 🆕 au tableau des anomalies) : `chargement-fondu-2400` prise pendant la planche de test du texte,
  le panneau « DÉMO DU TEXTE » chevauche l'écran (banc seulement, le joueur ne le voit pas). **Fusion autorisée.**
  Au PO : US de correction pour ANO-10.

### 2026-11-03 — Fin WFS-12 Règles de course (PR #10 fusionnée, d4bab4c)
QA : PASSABLE AVEC ANOMALIES MINEURES au tour 2/2 ; ANO-04 (italique du décompte, du score et des stats, à faire dans `src/ui/text.*`)
au tableau des anomalies, pour le PO. Branche synchronisée avec `main` juste avant la fusion (ddcb0af : seuls des fichiers de
règles avaient changé ; compilation et `test_course` OK, 0 écart). Épic E3 libre.
**PO** : passer en `⏳ Prêt` les tickets qui dépendaient de WFS-12 (WFS-27, WFS-24, WFS-21, WFS-38 si WFS-37 est fini) ; créer l'US de
correction d'ANO-04 ; corriger le critère « checkpoint 10 → +7 s » en « checkpoint 10 → +7,4 s (checkpoint 11 → +7 s) ».

### 2026-11-03 — Journal et anomalies séparés de TACHES.md (demande de l'utilisateur)
Le Journal est maintenant dans `journal.md` (nouvelles entrées à la fin) et le tableau des anomalies dans `anno.md`.
`TACHES.md` ne garde que les rôles, les règles, les épics, le tableau des tickets et les critères. Contenu déplacé tel quel.
Consignes mises à jour : `CLAUDE.md`, `.claude/agents/qa-lead.md`, `.claude/agents/po-scrum-master.md`,
`.claude/skills/qa-lead/SKILL.md`, `.claude/skills/waforspeed-natif/SKILL.md` et `references/construction-et-tests.md`.
**Pour tous les agents** : écrire le Journal dans `journal.md` et les anomalies dans `anno.md`, plus dans `TACHES.md`.

### 2026-11-03 — Début WFS-33 Banc d'essai : scènes de trafic plus probantes (Claude, session moteur-natif)
**Attribué par** `po-scrum-master`. Épic E3 (libre : aucun ticket E3 en cours ou en revue). Branche `feature/waforspeed-33`.
**Exception voulue par l'utilisateur** à la règle « un ticket par agent » : la session moteur-natif garde WFS-42 (E4, 🔧,
diagnostic au banc en attente de CI, ~20 min) et prend WFS-33 pendant cette attente. Même exception que pendant WFS-34 / WFS-07.
WFS-45 (prio 0.46, E4) non attribuable : E4 occupée par WFS-42.
**Zone autorisée** : `src/game/traffic_system.cpp` (scènes du banc seulement) ; `tests/**` en lecture. `traffic.cpp` intouché.
**Règles** : `regles-de-gestion/WFS-33.md` (RG-01 à RG-04).
**Critères** : `trafic-cam0` 2-3 voitures groupées, une par file ; ≥ 3 `course-*` sur 6 avec trafic visible ; `trafic-choc`
écart latéral ≤ 1,12 au journal + constat trottoir ; test de fidélité `0 écart(s)`.
**Plan** : placer un peloton devant le joueur et densifier le trafic proche pour les scènes du banc ; journaliser l'écart
latéral du choc ; vérifier `crashTraffic` du web pour le trottoir.

### 2026-11-03 — WFS-33 construit, en attente de la CI (Claude, session moteur-natif)
**Fait** (`feature/waforspeed-33`, zone `src/game/traffic_system.cpp` seulement) : peloton de 3 voitures une par file pour
`trafic-cam0` ; au banc (rendu hors écran) une voiture du trafic gardée dans le champ pendant la course ; écart latéral de
la voiture percutée au journal du banc.
**Constat RG-03** : le jeu web borne aussi l'écart latéral d'une voiture en glissade à ±1,12 demi-largeur de route
(`stepTraffic` de `js/game/traffic.js` : `clamp(car.offset + c.vx * h, -1.12, 1.12)`), soit 8,4 m du centre pour une
chaussée de 7,5 m : l'arrière sur le trottoir pendant une glissade existe donc aussi dans le jeu web (comportement identique).
**WFS-42** (toujours en cours) : diagnostic au banc en cours (pièce opaque de l'intérieur de la Golf).
**Suite** : CI → qa-lead.

### 2026-11-03 — Décision du PO : MVP du 2026-11-03 (demande de l'utilisateur : « un MVP aujourd'hui »)
**But** : le plus petit jeu complet sur téléphone : chargement → menu principal → « Course solo » → décompte 3-2-1-GO →
course solo avec trafic, chrono, checkpoints, HUD lisible (vitesse, temps, score, nitro) et notifications → écran de fin →
« Rejouer » / « Menu », en paysage plein écran Android.
**Déjà là** : conduite tactile, caméra, ville, arbres, ombres, trafic + collisions, sons du moteur, règles de course (décompte,
chrono, checkpoints, score, nitro, écran de fin + Rejouer), texte, écrans (WFS-36) et scène dédiée (WFS-44).
**Tickets du MVP, dans l'ordre** (marqués [MVP] en tête du tableau) :
1. WFS-35 chargement (E8, 🔍, fusion PR #12) ;
2. WFS-27 HUD **réduit** à du texte (E5, ≈ 2-3 h) : le compteur en arc sort en **WFS-46**, les notifications en **WFS-47** ;
3. WFS-37 menu, affichage (E8, Prêt dès la fusion de WFS-35) ;
4. WFS-29 Android plein écran, icône, nom (E7, Prêt, parallèle) ;
5. WFS-38 menu tactile, départ, fin → Rejouer / Menu (E8) ;
6. **WFS-47** notifications (checkpoint, quasi-contact, accident) (E5, nouveau, ≈ 2 h) ;
7. **WFS-50** vérification simulateur du MVP (E7, nouveau, parcours complet, rapport `simulateur-MVP.md`), puis « Livraison MVP ».
Chemin critique : E8 (35 → 37 → 38) en série ; E5 (27 → 47) et E7 (29) en parallèle.
**Hors MVP** : garage (WFS-11/40/13), terrain (WFS-41), pause (WFS-39), duel (WFS-21), sauvegarde (WFS-24), compteur en arc
(WFS-46), italique (WFS-48), nuit / lumières / météo / post-traitement (E4, E2), bruitages (WFS-28), piétons (WFS-19).
**Anomalies planifiées** : ANO-04 → **WFS-48** (E5, italique dans `src/ui/text.*`) ; ANO-10 → **WFS-49** (E7, captures
`chargement-*-2400` dans un lancement du banc sans `WFS_TEXTE_ESSAI`, `.github/workflows/build.yml`). `anno.md` : 📋 Planifiée.
**Mises à jour** : WFS-44 ✅ (PR #15 fusionnée, E0 libre) ; WFS-11 ⛔ → ⏳ En attente (ne dépend plus que de WFS-45) ;
WFS-24 et WFS-21 → ⏳ Prêt (WFS-12 ✅) ; WFS-38 reste en attente de WFS-37 ; critère de WFS-12 corrigé :
« checkpoint 10 → +7,4 s (checkpoint 11 → +7 s) » (`TACHES.md` et `regles-de-gestion/WFS-12.md`).
**Parallélisme (épics attribuables maintenant)** : E7 WFS-29 [MVP] ; E3 WFS-24 dès la fin de WFS-33 (en cours) ; E2 WFS-14 / WFS-43 / WFS-19 ;
E6 WFS-28 ; E4 WFS-45 dès la fin de WFS-42 ; E8 WFS-37 dès la fusion de PR #12. E0 : rien à faire.
Règles de gestion écrites : `WFS-27` (réécrite, réduite), `WFS-46`, `WFS-47`, `WFS-48`, `WFS-49`, `WFS-50`.

**Début WFS-27** — Épic **E5**, libre (WFS-11 en attente, aucun ticket E5 actif). Responsable : Claude (agent développeur,
session 01UY1Vv1), 2026-11-03 (WFS-35 n'attend plus que la fusion de PR #12, code fini, QA passable). Branche
`feature/waforspeed-27`, partie de `main`. Règles : `regles-de-gestion/WFS-27.md`.
**Zone de code autorisée** : `src/ui/**` sauf `src/ui/ecrans/` (nouveau `src/ui/hud.*`), `shaders/ui*`, `assets/fonts/**` ;
lecture seule de `src/game/course_system.h` / `course.h` / `player.h` ; accroche annoncée ≤ 10 lignes dans `src/game/game.cpp`
(retrait des deux barres provisoires de `Game::drawUi`) ; captures `hud-*` dans `src/platform/desktop_main.cpp` (≤ 10 lignes).
**Critères** : `course-*` + `hud-2400` lisibles ; `hud-temps-bas`, `hud-nitro`, `hud-combo` ; anciennes barres retirées ;
décompte et fin inchangés ; banc `0 écart(s)`.
**Plan court** : système `Hud` (lit `activeCourse()` et `GameView::player`, seulement en `Ecran::Course` et course non finie) →
temps + checkpoint (haut gauche), score + combo (haut droite), vitesse + nitro (bas centre) → accroche `game.cpp` → captures
du banc → PR, CI, QA par `qa-lead`.

### 2026-11-03 — WFS-27 HUD de course construit (MVP), en attente de la CI
**Fait** (`feature/waforspeed-27`, 29139a4) : `src/ui/hud.{h,cpp}`, système `Hud` (ordre 420), port de `updateHud` et du style
`.hud-top` / `.time-value(.low)` / `.cp-bar` / `.score-value` / `.combo` / `.nitro-bar` du jeu web. Affiché seulement en
`Ecran::Course`, avec une course active, hors écran de fin et hors scène dédiée. Haut gauche : « TEMPS » à 1 décimale (rouge sous
8 s) + « CHECKPOINT n » et sa barre cyan → orange ; haut droite (au-dessus de 22 % de H) : « SCORE » au format français + « Combo ×N » ;
bas centre (entre volant et frein) : |km/h| (« R · KM/H » en marche arrière, rouge > 300) + « NITRO » en 10 cases vertes #B6FF00
(halo en nitro active). Fonds vitrés rgba(20,22,31,.72), gouttières 16 / 14 px, tailles en fraction de la hauteur.
**Accroches annoncées** : `game.cpp` (5 lignes retirées : anciennes barres de vitesse et de nitro), `desktop_main.cpp`
(3 lignes : include, `benchHud`, mode `WFS_BANC=hud` pour la capture 2400x1080 sans planche de texte), `build.yml` (1 ligne : `hud-2400`).
Captures : `hud-course`, `hud-temps-bas` (temps forcé à 6 s), `hud-nitro` (touchers gaz + nitro), `hud-combo` (deux quasi-contacts
déclenchés par le banc), `hud-2400`.
**Suite** : CI → qa-lead → fusion. **WFS-35** : resynchronisé avec main (WFS-44, 825dfc6), CI en cours avant fusion de la PR #12.

### 2026-11-03 — PO : fin WFS-44 prise en compte, début WFS-29 (Claude, session trafic)
**Débloqué par WFS-44** : rien de nouveau en Prêt. WFS-11 reste ⏳ En attente : il dépend encore de WFS-45 (projecteurs, E4,
⏳ Prêt mais non attribuable tant que WFS-42 occupe E4). Anomalies : ANO-04 → WFS-48 (E5 occupée par WFS-27), ANO-10 → WFS-49
(attend la fusion de WFS-35) ; aucune nouvelle anomalie à planifier.
**Début WFS-29** — Épic **E7**, libre (aucun ticket E7 actif). Ticket Prêt le plus prioritaire attribuable (0.35, MVP).
Responsable : Claude (session trafic), 2026-11-03. Branche `feature/waforspeed-29`, partie de `main`.
Règles : `regles-de-gestion/WFS-29.md`.
**Zone de code autorisée** : `android/**` (manifeste, `res/` : icône adaptative, thème, chaînes ; activité Java/Kotlin),
`src/platform/**` (partie Android seulement si l'appel immersif passe par le natif). Rien dans `src/game`, `src/render`, `src/ui`.
**Critères** : `android:label="WaForSpeed"` ; icône adaptative (avant-plan + fond `#07070c`, orange `#FF6A00` / cyan `#00E5FF`) ;
thème plein écran, paysage verrouillé, écran allumé, `layoutInDisplayCutoutMode=shortEdges` ; immersif collant au démarrage et à
`onWindowFocusChanged` ; APK construit en CI ; captures du banc PC inchangées (`0 écart(s)`). Vérification à l'écran : WFS-50.
**Plan court** : manifeste + `res/values` (nom, thème) → `mipmap-anydpi-v26` (icône adaptative vectorielle) → immersif dans
l'activité → CI (APK + banc) → QA `qa-lead` (revue du manifeste et du code, banc inchangé) → fusion.

### 2026-11-03 — WFS-42 : cause trouvée par diagnostic au banc (Claude, session moteur-natif)
**Méthode** : captures de vitrine en masquant une à une les pièces (diagnostic temporaire, retiré ensuite).
**Cause (RG-01)** : la trame de la lunette de la Golf ne vient ni de la vitre ni des couches transparentes, mais de la
**pièce 18** : baguette chrome **opaque** (métal 1, rugosité 0) faite de fils de 1,3 mm x 21 mm, vue à travers la vitre ;
plus fins qu'un pixel, les fils se rasterisent en tirets brillants (`diag42-golf-sans-18` : trame disparue).
911 : la jante (matériau 4, triangles de 1,9 mm) ne scintille plus avec les reflets élargis du 2e passage.
**Correction** (`feature/waforspeed-42`, 6798421) : pièces « fils » (triangles < 4 mm, allongement médian > 8, hors peinture
et vitres) dessinées en couverture partielle avant les vitres ; modèles inchangés (RG-02). **Suite** : CI → qa-lead.

### 2026-11-03 — Fin WFS-35 Écran de chargement (PR #12 fusionnée, 500c5b5)
QA : PASSABLE AVEC ANOMALIES MINEURES au tour 2/2 ; ANO-10 → WFS-49 (PO). Branche resynchronisée avec main (WFS-12 puis WFS-44)
avant la fusion, CI verte à chaque fois (dernière : run 41617276755). Épic E8 libre.
**PO** : WFS-37 (menu principal, E8, MVP) et WFS-49 sont débloqués (dépendance WFS-35 finie).
**WFS-27** : CI verte (run 41617208849), QA lancée.

### 2026-11-03 — PO : WFS-37 et WFS-49 Prêts, début WFS-37 (agent développeur, session 01UY1Vv1)
**Fin de ticket WFS-35** prise en compte (✅, PR #12, 500c5b5). **Débloqués** : WFS-37 (WFS-35 ✅, WFS-36 ✅) et WFS-49 (WFS-35 ✅)
→ ⏳ Prêt. WFS-49 reste non attribuable tant que WFS-29 occupe E7. **WFS-27** → 🔍 En revue (PR #16, CI verte, QA en cours).
**Début WFS-37** — Épic **E8**, libre (WFS-35 fini, aucun ticket E8 actif). Ticket MVP Prêt le plus prioritaire (0.3).
Responsable : Claude (agent développeur, session 01UY1Vv1), 2026-11-03 ; son WFS-27 est en revue (même règle que pour WFS-35) ;
**les corrections demandées par la QA de WFS-27 passent avant WFS-37**. Branche `feature/waforspeed-37`, partie de `main`.
Règles : `regles-de-gestion/WFS-37.md` (complétées : RG-07 à RG-10, zone et accroches).
**Zone de code autorisée** : `src/ui/ecrans/**` (nouveau `menu.{h,cpp}`, mise en commun du logo dans `chargement.{h,cpp}`) ;
accroches annoncées ≤ 10 lignes : enregistrement du système, captures `menu-*` dans `src/platform/desktop_main.cpp`,
1 ligne `menu-1` dans `.github/workflows/build.yml`.
**Critères** : `menu-0` / `menu-1` (2400x1080, sans planche de texte) : ville assombrie, logo, 3 choix, record « — »,
« Golf 8 GTI » ; `menu-anim` ; ni commandes ni HUD sur le menu ; autres captures inchangées, banc `0 écart(s)`.
**Plan court** : sortir le bloc logo de `chargement.cpp` → système `Menu` (voile 8-16 bandes, 3 choix, pied, aide tactile,
`slide-in`) → captures du banc → PR, CI, QA par `qa-lead`.

### 2026-11-03 — WFS-29 : code écrit, PR #17 ouverte (Claude, session trafic)
**Fait** : `JeuActivity` (sous-classe de `NativeActivity`, jeu toujours natif) : mode immersif collant
(`WindowInsetsController` ≥ Android 11, `SYSTEM_UI_FLAG_IMMERSIVE_STICKY` avant) à `onCreate`, `onResume`,
`onWindowFocusChanged` ; écran toujours allumé ; encoche `shortEdges`. Icône adaptative (fond `#07070c`, « W » orange,
lignes cyan, monochrome) dans la zone sûre (32,8 / 33 dp). Manifeste : `hasCode`, `.JeuActivity`, icône.
`tools/qa/emulateur.sh` lance `.JeuActivity` (sinon le simulateur ne démarrerait plus l'appli).
**Vérification** : CI `45f3d8a` verte (APK avec le Java) ; APK inspecté : `classes.dex` avec `JeuActivity`, ressources
`ic_launcher*`, `ic_avant_plan`, nom « WaForSpeed ». Aucun fichier du moteur modifié (captures du banc inchangées).
**Pour le PO** : RG-02 « le HUD respecte les marges de l'encoche » n'est pas dans la zone E7 (HUD : E5 / E3) : à porter dans
un ticket du HUD. Barres masquées et icône : à vérifier au simulateur (WFS-50, fin d'épic E7).


### 2026-11-03 — WFS-33 fusionné (PR #18) (Claude, session moteur-natif)
QA `VALIDÉ` du premier coup (`qa/rapports/WFS-33.md`) : peloton de 3 voitures, trafic visible sur 3 captures de course
sur 6, écart latéral du choc +0,905 (≤ 1,12), 0 écart(s). **E3 libérée** (WFS-24 devient attribuable).
Observation non bloquante : en caméra 0, la voiture replacée à 90 m est petite à l'écran.

### 2026-11-03 — QA WFS-27 HUD de course : À CORRIGER, tour 1/2 (qa-lead)
Commit `29139a4`, run 41617208849, Release `apercu-waforspeed-27`, comparée pixel par pixel à `apercu-waforspeed-44` (même base).
Règles RG-01 à RG-09 remplies : temps (rouge à 5,5 s), checkpoint + barre, score au-dessus du bouton Nitro, « Combo ×2 », vitesse entre
volant et frein, nitro (halo en nitro active), anciennes barres retirées, `course-fin` sans HUD, décompte inchangé, `hud-2400` propre,
banc à 0 écart(s), rendu 3D inchangé hors panneaux du HUD. **Deux anomalies mineures** : ANO-11 (scènes de banc sans règles : temps
« 1000000000,0 » qui déborde du panneau, HUD affiché dans les vitrines) et ANO-12 (libellés 11 px flous à 720 p). Pas de fusion ;
le développeur corrige puis nouvelles captures. Rapport : `qa/rapports/WFS-27.md`.

### 2026-11-03 — PO : WFS-14 attribué à Claude (session trafic) ; WFS-14 redécoupé ; RG-02 de WFS-29 → WFS-51 / WFS-52 (po-scrum-master)
**Décision « un ticket par agent »** : WFS-29 (E7) est `VALIDÉ` par qa-lead et n'attend plus que la fusion (CI de la
resynchronisation) : il ne demande plus de travail, la session trafic peut donc prendre un nouveau ticket. E7 reste occupée
jusqu'à la fusion (WFS-49 non attribuable d'ici là).
**Choix** : prêts mieux classés écartés car leur épic est active — WFS-45 (E4 : WFS-42), WFS-49 (E7 : WFS-29), WFS-48
(E5 : WFS-27). Premier prêt d'une épic libre : **WFS-14 (E2)**, trop gros pour une journée → **redécoupé** : WFS-14 = socle
multi-cartes + Corniche (plage) ; **WFS-53** Campagne, **WFS-54** Grand Prix (en attente de WFS-14).
**Remarque RG-02 de WFS-29** (HUD sous l'encoche, hors E7) : **WFS-51** (E0, prio 0.43, point d'extension `view.marges`,
lecture des insets Android dans `JeuActivity`, commandes tactiles décalées, banc `WFS_ENCOCHE`) puis **WFS-52** (E5,
prio 0.435, HUD dans la zone sûre). WFS-51 devient `Prêt` dès la fusion de WFS-29.

### 2026-11-03 — Début WFS-14 : socle multi-cartes et Corniche (plage) (Claude, session trafic)
**Épic** : E2. **Zone de code autorisée** : `src/world/**`, `assets/world/**`, `shaders/world_*` ; test de fidélité du tracé
dans `tests/**` (ligne d'accroche E3 annoncée ici) ; branche `feature/waforspeed-14`. Règles : `regles-de-gestion/WFS-14.md`.
**Critères** : `carte-plage-jour` / `carte-plage-nuit` (océan, palmiers, parasols, sable, ambiance) ; tracé des 3 nouvelles
cartes fidèle au JS (0 écart) ; ≤ 300 appels de dessin ; ville par défaut, autres captures identiques.
**Plan court** : porter `MAPS` / `envFor` / `ZONES` (maps.js, decor.js) → décor plage (`vegPalm` via `vegetation.*`,
parasols, plan d'eau `#1e7fa8`) → `WFS_CARTE=plage` au banc + captures → PR, CI, QA par `qa-lead`.

### 2026-11-03 — Fin WFS-29 Android : plein écran immersif, icône, nom (Claude, session trafic)
**QA** : VALIDÉ par `qa-lead` (`qa/rapports/WFS-29.md`, `45f3d8a`), PR #17 fusionnée. Branche resynchronisée avec `main`
(WFS-35) avant la fusion : APK reconstruit avec succès (run 41621305547). **Épic E7 libre** : WFS-49 et WFS-51 attribuables (PO).
À vérifier au simulateur (WFS-50) : barres masquées, icône et nom sur le lanceur, démarrage de `.JeuActivity`.

### 2026-11-03 — WFS-14 : code écrit, PR ouverte (Claude, session trafic)
**Fait** : `src/world/plage.cpp` (bas-côtés `GROUND.beach` par zone, océan, palmiers, parasols, transats, ambiance
`envFor` jour / nuit) ; palmier `veg::palmier` (port de `vegPalm`) ; `src/world/zones.*` (`ZONES`, `ZONE_LEN`, nom exposé) ;
`tests/test_cartes.cpp` + référence du jeu web (tracé plage, campagne, circuit).
**Lignes d'accroche** : `src/core/system.h` (+4 : `System::ambiance`, `System::remplaceSol`), `src/game/game.cpp` (+3/−1 :
leurs appels, sol générique omis si remplacé) ; `src/platform/desktop_main.cpp` (captures `carte-plage-*`, `WFS_CARTE`) ;
`CMakeLists.txt` (+1) et `.github/workflows/build.yml` (test_cartes).
**Vérification (conteneur, lavapipe)** : banc de `main` et de la branche : 35 captures identiques au pixel près ;
`carte-plage-jour` / `carte-plage-nuit` conformes à l'œil ; 108 appels de dessin pour la plage ; test_cartes OK.
**⚠️ Pour le PO (épic E3)** : `easeInOut` de `src/game/track.cpp` utilise `PI` en float (jeu web : `Math.PI` double) →
altitudes de campagne / circuit décalées de 3 µm au plus. Corrigé et mesuré (0 écart à 1e-12), mais cela change
légèrement 27 captures existantes (ville comprise) : retiré de WFS-14 ; à faire dans un ticket E3 qui renouvelle les
références (test_cartes passera alors sa tolérance de 1e-3 à 1e-9).
**Limites** : villas, tours de sauveteurs, bateaux, lampadaires de plage, baigneurs non portés ; nuit sans éclairage
(WFS-09).


### 2026-11-03 — PO : WFS-55 créé (écart de PI) ; WFS-51 Prêt puis attribué à Claude (session trafic) (po-scrum-master)
**WFS-55** (E3, prio 12.05, en attente de WFS-14 qui apporte `test_cartes`) : `easeIn/Out/InOut` de `src/game/track.cpp`
en double (`std::numbers::pi`), `test_cartes` à 1e-9, captures de référence renouvelées et listées ; fusion annoncée au
Journal pour que les tickets en cours se resynchronisent. Priorité basse : invisible pour le joueur, mais nécessaire avant
WFS-53 / WFS-54 pour un test de fidélité strict. Règles : `regles-de-gestion/WFS-55.md`.
**WFS-51** passe `Prêt` (WFS-29 ✅). **Décision « un ticket par agent »** : WFS-14 est 🔍 En revue (code fini, attente CI et
qa-lead) ; la session trafic peut prendre un ticket, mais **les corrections de WFS-14 passent en premier** si qa-lead rend
`À CORRIGER` ou `REFUSÉ`. E2 reste occupée par WFS-14.
**Choix** : prêts mieux classés écartés car leur épic est active — WFS-45 (E4 : WFS-42), WFS-48 (E5 : WFS-27). Premier
prêt d'une épic libre : **WFS-51 (E0, 0.43)**, devant WFS-49 (E7, 0.47), qui reste attribuable à un autre agent
(il ne touche que `.github/workflows/build.yml`).

### 2026-11-03 — Début WFS-51 : marges de sécurité de l'écran (encoche) (Claude, session trafic)
**Épic** : E0. **Zone de code autorisée** : `src/core/**`, `src/game/game.*` ; exceptionnellement `src/platform/android_main.cpp`,
`android/**/JeuActivity.java` et, pour `WFS_ENCOCHE`, `src/platform/desktop_main.cpp` (pas `.github/**` : WFS-49).
Branche `feature/waforspeed-51`. Règles : `regles-de-gestion/WFS-51.md`.
**Critères** : `encoche-course` (2400x1080, `WFS_ENCOCHE=90`) : bande de 90 px marquée, volant et gaz hors de la bande ;
autres captures identiques, banc `0 écart(s)` ; insets Android + JNI relus en revue.
**Plan court** : `GameView::marges` (0 par défaut) → décalage des commandes et zones de toucher dans `Game` → lecture des
insets dans `JeuActivity` + une fonction JNI → `WFS_ENCOCHE` au banc + capture → PR, CI, qa-lead.

### 2026-11-03 — WFS-51 en recette (Claude, session trafic)
- PR #20 : `GameView::marges`, `Game::setMarges`, commandes tactiles et zones de toucher décalées ; CI verte (commit 66c13bc, synchronisé avec `main` en 778085f).
- Accroches hors E0, prévues par `regles-de-gestion/WFS-51.md` : `src/platform/android_main.cpp` (fonction JNI `nativeMarges`), `JeuActivity.java` (lecture des insets d'encoche), `src/platform/desktop_main.cpp` (`WFS_ENCOCHE`, capture `encoche-course.png`). Sans `WFS_ENCOCHE`, captures identiques à `main`.
- QA demandée à `qa-lead`.

### 2026-11-03 — main ne compile plus depuis la fusion de WFS-27 (Claude, session trafic)
- La fusion de la PR #16 (WFS-27) a laissé des marqueurs de conflit (`<<<<<<<` / `>>>>>>>`) dans `.github/workflows/build.yml` et `src/platform/desktop_main.cpp` : CI de `main` en échec (c68c460, c17627d).
- Réparé dans `feature/waforspeed-51` (PR #20, validée par le QA) en gardant les deux côtés : WFS-35 (écran de chargement) et WFS-27 (banc HUD seul). Tests et banc local OK ; `main` sera réparé à la fusion de #20.
- Agents : avant toute fusion, vérifier `git grep -n "^
### 2026-11-03 — WFS-51 fusionné ; main réparé ; WFS-14 (Claude, session trafic)
- PR #20 (WFS-51, VALIDÉ) fusionnée en 24f08d3 : `main` compile de nouveau (marqueurs de conflit de WFS-27 retirés, CI verte sur 04ec9f5).
- La PR #19 (WFS-14) a été fusionnée par l'utilisateur alors qu'elle était « À CORRIGER tour 1/2 ». La correction d'ANO-13 (stipes des palmiers trop sombres) part de `main` sur `feature/waforspeed-14`, nouvelle PR, puis QA tour 2/2.

### 2026-11-03 — WFS-14 : correction ANO-13 en PR #21 (Claude, session trafic)
- Stipes des palmiers : albédo ×1,5 et part propre brune #7d6a52 (0,8 de jour, 0,05 la nuit), seulement dans `src/world/plage.cpp`. Seules les 2 captures de la plage changent. QA tour 2/2 demandée après la CI.

### 2026-11-03 — Début WFS-49 : captures du chargement en 2400x1080 sans planche de texte (Claude, session trafic) — attribué par le PO
**Épic** : E7. **Zone de code autorisée** : `.github/workflows/build.yml` uniquement (aucun `src/**`). Branche `feature/waforspeed-49`.
Règles : `regles-de-gestion/WFS-49.md`.
**Critères** : `chargement-0/1/2/fondu/erreur-2400` sans panneau « DÉMO DU TEXTE » ; `texte-essai-1280/2400` et captures 1280x720 inchangées ; + 1 lancement du banc au plus.
**Plan court** : lancement séparé `WFS_TAILLE=2400x1080 ./build/wfs_bench chargement assets/` sans `WFS_TEXTE_ESSAI`, la boucle de copie (l. 41) lit ce dossier → PR, CI, qa-lead.
**Choix du PO** : épics actives écartées — E2 (WFS-14), E4 (WFS-42), E5 (WFS-27), E8 (WFS-37) ; E0 n'a plus de ticket prêt. Premier prêt d'une épic libre : WFS-49 (E7, 0.47), devant WFS-28 (E6, 15) et WFS-24 (E3, 21).
WFS-14 est en revue (PR #21, QA tour 2/2) : la session finit d'abord WFS-14, WFS-49 est préparé en parallèle (ticket de 30 min, zone sans recouvrement).
**WFS-51 ✅** : WFS-52 n'attend plus que WFS-27 (toujours « À CORRIGER tour 1/2 » au tableau, PR #16 pourtant fusionnée : à clore par son responsable après QA tour 2/2).

### 2026-11-03 — WFS-49 en revue, PR #22 (Claude, session trafic)
- `build.yml` : `chargement-*-2400` pris dans un lancement séparé du banc en 2400x1080 sans `WFS_TEXTE_ESSAI`. QA après la CI.

### 2026-11-03 — QA WFS-27 HUD de course : VALIDÉ, tour 2/2 (qa-lead)
Captures de la Release `apercu` (`main` 6e672a9, run 41639563396, contient 24f08d3) ; `feature/waforspeed-27` (4f1a32d) = main.
ANO-11 corrigée : plus de HUD ni de temps « 1000000000,0 » dans `course-cam*`, `trafic-*`, `ombres-*`, `texte-essai-*` ; les 12
`vitrine-*` sont identiques au pixel près à la référence WFS-44. ANO-12 corrigée : libellés lisibles à 720 p (`hud-course`, `hud-nitro`).
`hud-*`, `course-decompte-3` et `course-fin` toujours conformes, banc à 0 écart(s). Aucune anomalie restante, rien dans `anno.md`.
**Procédure** : la PR #16 avait été fusionnée (c68c460) avant ce verdict, avec des marqueurs de conflit réparés par la PR #20 (WFS-51).
Remarque pour WFS-51 : `encoche-course` est prise sans HUD (scène sans règles de course). Rapport : `qa/rapports/WFS-27.md`.
### 2026-11-03 — WFS-27 corrigé (tour 1/2) et WFS-37 construit (Claude, session 01UY1Vv1)
**WFS-27** : corrections ANO-11 (pas de HUD quand le temps est illimité : vitrines et scènes du banc sans règles) et ANO-12
(libellés à 13 px minimum, calés au pixel) poussées en 1846220. **Incident** : ce commit de synchronisation contenait des marqueurs
de conflit (`desktop_main.cpp`, `build.yml`) et la PR #16 a été fusionnée par quelqu'un d'autre avant le tour 2/2 de QA ; réparé par
la PR #20 (WFS-51). Leçon notée : toujours vérifier l'absence de marqueurs avant chaque commit de synchronisation. La branche
(4f1a32d) est identique à `main`. **Suite** : QA tour 2/2 sur `main`, puis clôture.
**WFS-37** (`feature/waforspeed-37`, e252578) : `src/ui/ecrans/menu.{h,cpp}` (système Menu, ordre 450, seulement en `Ecran::Menu`) :
voile `.screen` en 16 bandes, fondu 300 ms, slide-in (−36 px, 40 ms de décalage, `--ease`), 3 choix `.mitem` aux textes exacts
(« Course solo » orange), pied `.menu-foot` (RECORD « — » / meilleur score de la session, TA VOITURE « GOLF 8 GTI »), aide tactile.
Logo et sur-titre mis en commun avec le chargement (`miseEnPage`, `dessinerBlocLogo` dans `chargement.{h,cpp}`, dessin inchangé).
**Accroches annoncées** : `desktop_main.cpp` (3 lignes : include, `benchMenu`, mode `WFS_BANC=menu`), `build.yml` (1 ligne : `menu-1`).
Captures : `menu-anim`, `menu-0`, `menu-1` (2400x1080). **Suite** : CI → qa-lead.

### 2026-11-03 — Fin WFS-27 HUD de course (QA VALIDÉ au tour 2/2)
PR #16 fusionnée (c68c460, avant le verdict, puis réparée par la PR #20) ; QA du tour 2/2 VALIDÉ sur `main` (ae41abd). Épic E5 libre.
Débloqués : WFS-47 (notifications, MVP), WFS-46 (compteur en arc), WFS-52. Ligne `résolution de conflit (Claude, session 01UY1Vv1) : retirée.

### 2026-11-03 — WFS-14 terminé (Claude, session trafic)
- QA tour 2/2 : VALIDÉ (ANO-13 corrigée, stipes bruns de jour, sombres la nuit ; 48/50 captures identiques). PR #21 fusionnée (e4bc431). E2 libre ; WFS-53, WFS-54 et WFS-55 débloqués.

### 2026-11-03 — WFS-49 terminé (Claude, session trafic)
- QA : VALIDÉ au 1er passage (ANO-10 corrigée, seul `chargement-fondu-2400` change). PR #22 fusionnée (37facc6). E7 libre.

### 2026-11-03 — Début WFS-47 Notifications de course (Claude, session trafic) — attribué par le PO
- Ticket : WFS-47 [MVP] · Épic E5 · règles `regles-de-gestion/WFS-47.md` · branche `feature/waforspeed-47`.
- Zone autorisée : `src/ui/**` (sauf `src/ui/ecrans/`), `shaders/ui*` ; nouveau fichier conseillé `src/ui/toasts.*`. Aucune modification de `src/game/` (évènements déduits des compteurs de `Course`).
- Critères : `toast-checkpoint` (« CHECKPOINT 1 » « +11,0 s » sous le score) ; `toast-pile` (3 bandeaux cyan / orange / rouge, rien sur les commandes) ; banc `0 écart(s)`, autres captures inchangées.
- Plan : porter `js/ui/toasts.js` (pile de 3, 1,9 s, entrée 240 ms / sortie 260 ms) ; détection par différence d'image (`level`, `nearCount`, `combo`, `accidents`) ; remise à zéro au Rejouer ; 2 scènes de banc.
- PO : WFS-27 ✅ débloque WFS-52 et WFS-46 (E5, ⏳ Prêt) ; WFS-14 ✅ débloque WFS-53, WFS-54 (E2) et WFS-55 (E3), passés ⏳ Prêt. Ligne de critères de WFS-27 corrigée (Terminé).

### 2026-11-03 — WFS-37 Menu principal fusionné sur ordre de l'utilisateur (PR #23, 42d4f61)
L'utilisateur a ordonné la fusion sans attendre le banc d'essai complet ni la QA (« fusionne, c'est un ordre ») : compilation,
test de la piste et APK verts (run 41644926035), captures du banc encore en cours. Statut ✅, **QA faite après la fusion** sur `main`.
**Nouvelle règle (demande de l'utilisateur)** : fusion rapide possible sur ordre de l'utilisateur (CI verte et rien de bloquant
sur les captures), QA après fusion ; bloquant → ticket de correction prioritaire, mineur → `anno.md`. Mis à jour : `TACHES.md`
(règle 5), `CLAUDE.md`, `.claude/agents/qa-lead.md`. Épic E8 libre : **WFS-38** (menu tactile, départ, Rejouer / Menu) débloqué.

### 2026-11-03 — WFS-38 prêt (PO)
- WFS-37 ✅ (PR #23, 42d4f61) et WFS-12 ✅ : **WFS-38** (menu tactile, départ de course, fin → Rejouer / Menu, E8) passe ⏳ Prêt, sans responsable.
- **WFS-38 prêt, à attribuer en priorité au prochain agent développeur qui demande un ticket (consigne de l'utilisateur) ; la session 01UY1Vv1 est en pause** (ne pas la lui attribuer). Mention « prochain demandeur » dans la colonne Responsable de `TACHES.md`.
- Règles `regles-de-gestion/WFS-38.md` complétées : état pressé aligné sur le web (`.mitem:hover` : décalage 10 px, barre cyan, et non orange / 2 px), zone de code E8 et 3 accroches ≤ 10 lignes (`course_system.cpp` : bouton « Menu » + remise à zéro en entrant en course ; `android_main.cpp` : ouverture sur le menu ; `desktop_main.cpp` : scène de banc), RG-04 « Bientôt » (Showroom, Duel), RG-08 banc, captures `menu-bientot` et `fin-retour-menu` ajoutées.

### 2026-11-03 — WFS-42 fusionné (PR #24) (Claude, session moteur-natif)
QA 1er passage `REFUSÉ`, corrigé (cause : pièce 18 de la Golf, fils chromés opaques plus fins qu'un pixel). Le 2e passage
QA a été interrompu ; **fusion décidée par l'utilisateur** après contrôle des captures : lunette de la Golf sans trame,
jantes de la 911 sans points brillants, aucun blocage. Mineur : feux arrière de la Golf un peu plus rouges.
Hors périmètre, inchangé : pointillés le long des jointures de carrosserie (WFS-17). **E4 libérée** (WFS-45 conseillé ensuite).

### 2026-11-03 — WFS-47 en revue, PR #25 (Claude, session trafic)
- Nouveau système `src/ui/toasts.*` (ordre 425) : évènements déduits des compteurs de `Course` (aucune modification de `src/game/`). Accroche de 1 ligne dans `benchHud` (`src/ui/hud.cpp`, même épic) pour les captures `toast-checkpoint` et `toast-pile`.
- Pile placée à gauche du bouton Nitro : alignés sur le bord droit, 3 bandeaux le recouvraient à 720 p (RG-05). `hud-combo` montre désormais les bandeaux de ses 2 quasi-contacts. QA après la CI.

### 2026-11-03 — WFS-47 terminé (Claude, session trafic)
- QA : VALIDÉ au 1er passage (toast-checkpoint, toast-pile, hud-combo conformes ; pile à gauche du Nitro acceptée, à revoir avec WFS-52 si besoin). PR #25 fusionnée (5362761). E5 libre.

### 2026-11-03 — WFS-38 attribué à Claude (session trafic) (PO) — Début
- Ticket **WFS-38** (menu tactile : appuis, départ de course, fin → Rejouer / Menu), épic **E8**, priorité 0.4, branche `feature/waforspeed-38`. Dépendances WFS-37 ✅, WFS-12 ✅. Attribué au prochain demandeur, selon la consigne de l'utilisateur.
- Zone autorisée : `src/ui/ecrans/**` + 3 accroches ≤ 10 lignes annoncées au Journal (`src/game/course_system.cpp` : bouton « Menu » et remise à zéro en entrant en course ; `src/platform/android_main.cpp` : ouverture sur le menu ; `src/platform/desktop_main.cpp` : scène de banc). `src/ui/hud.*` n'est plus réservé (WFS-47 ✅) mais reste hors zone.
- Critères : captures `menu-appui`, `menu-depart-3`, `menu-depart-go`, `menu-bientot`, `fin-menu`, `fin-retour-menu` ; autres captures inchangées, banc `0 écart(s)`. Règles : `regles-de-gestion/WFS-38.md`.
- Plan : zones touchables + état pressé dans `menu.cpp` → départ en course (reset) → bouton « Menu » de fin → accroche Android → scène `benchMenuTactile` → QA `qa-lead`.

### 2026-11-03 — WFS-55 attribué à Claude (session facile) (PO) — Début
- Ticket **WFS-55** (correction : PI double dans `easeIn/easeOut/easeInOut`), épic **E3**, priorité 12.05, branche `feature/waforspeed-55`. Dépendance WFS-14 ✅. Demande de l'utilisateur : ticket facile.
- Zone autorisée : `src/game/track.cpp`, `tests/test_cartes.cpp`, captures de référence renouvelées. Noyau (`src/core`) inchangé. Règles : `regles-de-gestion/WFS-55.md`.
- Plan : formules en double (`std::numbers::pi`) → tolérance 1e-9 → renouveler les captures qui changent (≈ 27) et les lister → annoncer la fusion au Journal.

### 2026-11-03 — WFS-38 en revue, PR #27 (Claude, session trafic)
- Menu tactile dans `src/ui/ecrans/menu.*` (état pressé, relâcher, glissé annulé, « Bientôt », Duel grisé, record pris à la fin de course).
- **Accroches annoncées** (prévues par `regles-de-gestion/WFS-38.md`) :
  - `src/game/course_system.cpp` (E3) : `onEcran` Menu → Course = même chemin que « Rejouer » ; course figée derrière le menu ; décompte et écran de fin seulement en `Ecran::Course` ; bouton « Menu » à droite de « Rejouer » (paire centrée) et son toucher. Environ 10 lignes, dont 4 lignes existantes modifiées.
  - `src/platform/android_main.cpp` (E7) : `allerA(Ecran::Menu)` après le chargement (1 ligne).
  - `src/platform/desktop_main.cpp` (E7) : `benchMenuTactile` (1 ligne, et dans le raccourci `WFS_BANC=menu`).
- Changements de captures attendus : `course-fin` (bouton « Menu »), `menu-0` / `menu-anim` (Duel grisé, plus de « 3 » sur le menu). QA après la CI.

### 2026-11-03 15:52:10 — WFS-55 terminé, PR #26 fusionnée (Claude, session facile)
- `easeInOut` (`src/game/track.cpp`) calcule avec `std::numbers::pi` (double) comme `Math.PI` du jeu web ; `test_cartes` à 1e-9 (écart max 3,6e-12).
- Fusion rapide sur ordre de l'utilisateur : compilation, tests et APK verts ; banc d'essai non terminé (plus de 55 min) ; captures non vues → recette de nuit (`qa/Check.md`).
- **Les tickets en cours doivent se resynchroniser avec `main`** : environ 27 captures de référence changent de quelques pixels (aucune différence visible attendue).

### 2026-11-03 — PO : point MVP, création de WFS-56, début WFS-56 (Claude, session 01UY1Vv1) — attribué par le PO
- **Point MVP** : ✅ WFS-35, WFS-27, WFS-37 (fusion rapide, QA après fusion pas encore faite), WFS-29, WFS-47 (PR #25 fusionnée, VALIDÉ). 🔍 WFS-38 (PR #27, session trafic) : E8 occupée. ⏳ WFS-50 attend WFS-38. Aucune anomalie bloquante signalée après fusion (`anno.md` : mineures seulement ; recette de nuit `qa/Check.md` : WFS-42, WFS-55 non encore passées). WFS-37 manquait à `qa/Check.md` : ligne ajoutée par le PO.
- Aucune US MVP attribuable (WFS-38 en revue, WFS-50 en attente) : création de **WFS-56** (étape 1 de WFS-50, E7 libre), ajoutée aux dépendances de WFS-50 et à l'ordre du MVP.
- Ticket **WFS-56** · épic **E7** · priorité 0.441 · branche `feature/waforspeed-56` · règles `regles-de-gestion/WFS-56.md`.
- Zone autorisée : `tools/qa/**`, `.github/workflows/simulateur.yml`. Aucune accroche : ni `src/`, ni `android/` (lecture seule de `src/ui/ecrans/menu.cpp` et `drawOver` de `src/game/course_system.cpp` pour les coordonnées). `android_main.cpp` / `desktop_main.cpp` sont touchés par la PR #27 : ne pas y toucher.
- Critères : `scenario=mvp` → 10 captures nommées (chargement, menu, appui Course solo, décompte, GO, course + HUD, fin, Rejouer, 2e fin, retour menu), appuis notés dans `deroulement.txt`, `RÉSULTAT TECHNIQUE : OK` ; `scenario=conduite` inchangé.
- Plan : paramètre `scenario` dans `simulateur.yml` → fonction `tap` + scénario `mvp` dans `emulateur.sh` (coordonnées calculées depuis la mise en page, attentes 30 s de chrono + marge) → lancement du workflow sur la branche → QA `qa-lead`.

### 2026-11-03 — WFS-56 construit, essai simulateur lancé (Claude, session 01UY1Vv1)
`feature/waforspeed-56` (088e0b3), PR #28 : `tools/qa/emulateur.sh` gagne un 3e argument `scenario` (`conduite` par défaut,
inchangé ; `mvp` : 10 captures 01-chargement … 10-retour-menu, appuis `adb input tap` aux centres calculés comme la mise en page
du jeu, « appui <nom> x,y » dans `deroulement.txt`, contrôle `VK_ERROR`) ; `simulateur.yml` : entrée `scenario`. Essai lancé avec
`scenario=mvp` (Release `simulateur-WFS-56-mvp`). Seules les captures 01 et 02 sont attendues correctes tant que WFS-38 n'est pas
fusionné. **Suite** : résultat du simulateur → qa-lead.

### 2026-11-03 — WFS-38 : correction d'ANO-01, QA tour 2/2 demandée (Claude, session trafic)
- L'accroche de `src/game/course_system.cpp` passe à **9 lignes touchées** (`git diff --numstat` : 9 ajoutées, 4 supprimées, soit 5 nouvelles et 4 modifiées). L'annonce précédente (« environ 10, dont 4 modifiées ») était fausse : il y en avait 15.
- Pour y arriver : un seul garde en tête de `drawUi` ; « Rejouer » reste à sa place (RG-06 : inchangé) et « Menu » est placé à sa droite. `fin-menu` et `course-fin` montrent donc la paire décalée à droite, et non plus centrée.

### 2026-11-03 — QA WFS-38, tour 2/2 : PASSABLE AVEC ANOMALIES MINEURES (qa-lead)
- `60cde90` (run 41680051281) : ANO-01 corrigée. L'accroche de `course_system.cpp` touche 9 lignes, le journal est exact. Aucune régression : 59 captures sont identiques au pixel au tour 1, et `fin-menu` / `course-fin` ne diffèrent que par le bouton « Menu ». Banc `0 écart(s)`.
- Il reste une mineure, **ANO-14** : la paire « Rejouer » / « Menu » est décentrée de 108 px. Elle est ajoutée à `anno.md` (🆕 Nouvelle) pour que le PO la priorise. La PR #27 était déjà fusionnée par l'utilisateur (`27e22c6`) ; le verdict l'autorise. Rapport : `qa/rapports/WFS-38.md`.

### 2026-11-03 — WFS-38 terminé (Claude, session trafic)
- QA tour 2/2 : PASSABLE AVEC ANOMALIES MINEURES (ANO-01 corrigée ; ANO-14 : paire « Rejouer » / « Menu » décentrée, à prioriser par le PO, correction en E3). PR #27 fusionnée par l'utilisateur (27e22c6), verdict compatible. E8 libre ; WFS-39 et WFS-41 débloqués.

### 2026-11-03 — PO : ANO-14 planifiée (WFS-57), WFS-39 et WFS-41 prêts, début WFS-57 (Claude, session trafic) — attribué par le PO
- **ANO-14** priorisée : écran le plus vu du MVP, ≈ 1 h, et le scénario simulateur (`tools/qa/emulateur.sh`, WFS-56) calcule déjà « Menu » dans une paire centrée (`bx = W/2 − (1,7·bw + 2·u)/2`) : son appui tombe aujourd'hui à côté du bouton. → US **WFS-57** (E3, priorité 0.405), ajoutée aux dépendances de WFS-50 et à l'ordre du MVP ; ANO-14 en 📋 Planifiée.
- WFS-38 ✅ : **WFS-39** (pause) et **WFS-41** (terrain) passent en ⏳ Prêt (E8 libre, WFS-39 d'abord).
- Ticket **WFS-57** · épic **E3** · branche `feature/waforspeed-57` · règles `regles-de-gestion/WFS-57.md`.
- Zone autorisée : `src/game/course_system.cpp` (`drawOver`, `onTouch`). Accroche annoncée ≤ 2 lignes : `src/ui/ecrans/menu.cpp` (`benchMenuTactile`, toucher « Menu »). Interdit : `tools/qa/**` (formule déjà bonne).
- Critères : `fin-menu` / `course-fin` paire centrée (milieu x = 640 ± 2 en 1280x720), `fin-retour-menu` au menu, autres captures identiques, banc `0 écart(s)`.
- Plan : un départ `bx` commun pour les deux rectangles (dessin = zones de toucher) → accroche du banc → banc PC → `qa-lead`.

### 2026-11-03 — WFS-57 en revue, PR #29 (Claude, session trafic)
- `course_system.cpp` : paire « Rejouer » / « Menu » centrée (milieu mesuré à 639,5 en 1280x720) ; zones de toucher = rectangles dessinés.
- **Accroche E8 annoncée** (2 lignes, prévue par les règles) : `src/ui/ecrans/menu.cpp`, toucher « Menu » de `benchMenuTactile` sur la nouvelle place. `tools/qa/**` non touché. QA après la CI.

### 2026-11-03 — QA WFS-57 : VALIDÉ (qa-lead)
- Banc PC, commit `cc76b01` (PR #29) : paire « Rejouer » / « Menu » centrée (milieu 639,5, marges 187 / 187 px), `fin-retour-menu` au menu (record 390),
  58 autres captures identiques à `apercu-waforspeed-38`, banc `0 écart(s)`. Aucune anomalie : **fusion autorisée**. ANO-14 passée en ✅ Corrigée.
  Rapport : `qa/rapports/WFS-57.md`.

### 2026-11-03 — WFS-57 terminé (Claude, session trafic)
- QA : VALIDÉ au 1er passage, ANO-14 corrigée. PR #29 fusionnée (b08d7d2). E3 libre.

### 2026-11-03 — Début WFS-52 HUD dans les marges de sécurité (Claude, session trafic)
- **Ticket** : WFS-52 · épic E5 · branche `feature/waforspeed-52` · règles `regles-de-gestion/WFS-52.md`.
- **Zone autorisée** : `src/ui/**` sauf `src/ui/ecrans/` (HUD de WFS-27, notifications de WFS-47), `shaders/ui*` ; scène de banc `encoche-hud` via l'option `WFS_ENCOCHE` de WFS-51 (si une ligne d'accroche est nécessaire dans `tools/qa/**`, ≤ 10 lignes annoncées ici).
- **Critères** : `encoche-hud` (2400x1080, `WFS_ENCOCHE=90`) sans texte ni panneau dans la bande ; sans encoche, `course-*` / `hud-*` identiques, banc `0 écart(s)` ; notifications (WFS-47, fusionné) soumises à la même règle (RG-03).
- **Plan** : lire `view.marges` (WFS-51) dans le placement du HUD et des notifications, décaler par la marge, ajouter la capture, QA `qa-lead`.
- PO : lignes de détail WFS-57 et WFS-37 remises à jour (✅ Terminé), le tableau l'indiquait déjà.

### 2026-11-03 — WFS-52 en revue, PR #30 (Claude, session trafic)
- `src/ui/hud.cpp` : panneaux du HUD à la marge de `view.marges` + gouttières de WFS-27 ; sans marge, rendu inchangé. Capture `encoche-hud` dans `benchHud`.
- **Accroche annoncée** : `.github/workflows/build.yml`, 1 ligne modifiée (copie de `hud/encoche-hud.png` en `captures/encoche-hud-2400.png`, dans le lancement HUD 2400x1080). QA après la CI.

### 2026-11-03 — Point des PR fusionnées, tableau remis à jour (Claude, session trafic, à la demande de l'utilisateur)
- Revue des 20 dernières PR fermées (#10 à #29), toutes fusionnées, comparées à `TACHES.md` : 19 tickets étaient déjà à jour.
- **WFS-56** (E7, session 01UY1Vv1) : PR #28 fusionnée par l'utilisateur à 17:54 (94087c7), ligne restée « En cours » → passée à **✅ Terminé (QA après fusion)**, PR #28 notée. Aucun rapport `qa/rapports/WFS-56.md` pour l'instant.
- QA encore à faire après fusion : **WFS-37** (PR #23) et **WFS-56** (PR #28) ; pas de rapport QA non plus pour WFS-55 (PR #26, « recette de nuit »).
- Seule PR ouverte : **#30 (WFS-52, en revue)**, CI en cours.
- Pour le PO : toutes les dépendances de **WFS-50** (vérification simulateur du MVP) sont maintenant fusionnées (WFS-35, 27, 37, 38, 29, 47, 56, 57).

### 2026-11-03 — PO : WFS-50 prêt puis début WFS-50 Vérification simulateur du MVP (Claude, session trafic) — attribué par le PO
- Toutes les dépendances de **WFS-50** sont ✅ (WFS-35, 27, 37, 38, 29, 47, 56, 57) : ⏳ En attente → ⏳ Prêt, puis attribué (meilleure priorité hors E5, E7 libre). Ligne de détail WFS-52 remise en « 🔍 En revue (PR #30) ».
- Règle « un ticket par agent » : dérogation assumée par le PO, WFS-52 n'est plus qu'en revue (attente QA) et les zones ne se recouvrent pas (E5 `src/ui/**` / E7 `tools/qa/**`, `.github/**`).
- **QA après fusion de WFS-37, WFS-56 (et recette de WFS-55)** : le parcours simulateur de WFS-50 sert de vérification à l'écran de WFS-37 (menu) et de WFS-56 (scénario `mvp`) ; `qa-lead` le citera dans `simulateur-MVP.md`. Un défaut trouvé = ticket de correction créé par le PO, pas de correctif dans WFS-50.
- **Ticket** : WFS-50 · épic E7 · branche `feature/waforspeed-50` · règles `regles-de-gestion/WFS-50.md`.
- **Zone autorisée** : `tools/qa/**`, `.github/**` (E7) seulement, et uniquement si le scénario `mvp` de WFS-56 doit être ajusté ; aucun changement de `src/` ni `android/`.
- **Critères** : `simulateur.yml` lancé sur `main` avec `epic=MVP` et `scenario=mvp` ; captures de chaque étape (chargement, menu, Course solo, décompte, course + HUD, fin, Rejouer, Menu) ; paysage plein écran, aucun plantage ni ANR, logcat sans erreur Vulkan ; rapport `qa/rapports/simulateur-MVP.md` VALIDÉ par `qa-lead`.
- **Plan** : lancer le workflow sur `main` → récupérer captures et `deroulement.txt` → `qa-lead` → si VALIDÉ, le PO note « Livraison MVP » au Journal avec le lien de l'APK (`apercu`).

### 2026-11-03 — WFS-50 : simulateur lancé sur main (Claude, session trafic)
- `simulateur.yml` lancé sur `main` (b8b89fc), avec `epic=MVP` et `scenario=mvp` (run 41702978455). L'APK est celle de `main` après fusion de tous les tickets du MVP.
- Aucun changement de code pour l'instant. La branche `feature/waforspeed-50` ne sera créée que si le scénario `mvp` doit être ajusté (`tools/qa/**`).
- `qa-lead` rendra `qa/rapports/simulateur-MVP.md` sur les captures du run.

### 2026-11-03 — PO : WFS-50 REFUSÉ, corrections WFS-58 et WFS-59 créées ; début WFS-58 (Claude, session trafic) — attribué par le PO
- Rapport `qa/rapports/simulateur-MVP.md` (4bb1670) : **REFUSÉ**, bloquantes SIM-MVP-1 (0,3 ips sur SwiftShader, dt plafonné à 0,05 s → temps du jeu ≈ 1 s par minute), SIM-MVP-2 (captures 01-05 hors étape), SIM-MVP-3 (faux « OK » du scénario), mineure SIM-MVP-4 (barres système sur `qa-01`). Pas de « Livraison MVP ».
- Découpage (RG-04 de WFS-50), deux tickets **parallèles**, zones séparées :
  - **WFS-58** (E7, prio 0.437, `tools/qa/**`, `.github/**`) : scénario `mvp` piloté par les lignes du logcat avec délai max, capture après présentation d'images, `ÉCHEC` au lieu de faux OK, 1re capture après la 1re image du jeu (SIM-MVP-4), écran d'émulateur réduit (`wm size`), lancement avec `--ez qa_simu true`. Règles : `regles-de-gestion/WFS-58.md`.
  - **WFS-59** (E0, prio 0.438) : mode GPU logiciel (détection SwiftShader existante de `vk.cpp` ou extra `qa_simu`) : dt max 0,5 s (pas fixes 1/120 s gardés), rendu 25 %, sans ombres ; rien ne change sur GPU matériel ni au banc. Accroches ≤ 10 lignes autorisées dans `src/render/vk.cpp` (E4 libre) et `src/platform/android_main.cpp` (pas de fichier commun avec WFS-58). Règles : `regles-de-gestion/WFS-59.md`. Prêt pour un autre agent.
  - Arbitrage PO : pas de course raccourcie (on garderait des règles différentes du jeu web) ; distance de vue hors périmètre tant que l'objectif `QA ips` ≥ 1 est atteint.
- **WFS-50** : 🔧 → ⛔ Bloqué, dépend de WFS-58 et WFS-59, réservation libérée ; à relancer sur `main` (`epic=MVP`, `scenario=mvp`) après leurs fusions, puis nouveau passage `qa-lead`. La QA à l'écran de WFS-37 et de WFS-56 reste due par ce parcours.
- **Début WFS-58** · épic E7 · responsable Claude (session trafic) · branche `feature/waforspeed-58` · zone `tools/qa/**`, `.github/**` (aucun `src/`, `android/`).
- **Critères** : chaque étape attend son événement du logcat ; `01-chargement` = barre du jeu sans barres système, `02-menu` = menu ; jamais `RÉSULTAT TECHNIQUE : OK` avec une étape manquante ; `scenario=conduite` inchangé.
- **Plan** : fonction `attendre` (logcat borné) → réécrire les étapes du scénario `mvp` autour → `wm size` + extra `qa_simu` → lancer `simulateur.yml` sur la branche → `qa-lead`.

### 2026-11-03 — WFS-58 construit, essai simulateur lancé (Claude, session trafic)
- `tools/qa/emulateur.sh`, scénario `mvp` : chaque étape attend sa ligne du journal du jeu avec un délai max. Ensuite, 2 lignes `QA ips=` avant la capture et une marque FIGÉE si deux captures sont identiques. Si une étape manque, le scénario s'arrête en échec franc. L'écran est réduit à 540x1200 et l'appli est lancée avec `--ez qa_simu true`. `simulateur.yml` : l'étape du scénario passe à 35 min.
- **Pour le PO** : ni le décompte ni le « GO » ne sont journalisés dans `src/` (`course_system.cpp` n'écrit que `nouvelle course`, `checkpoint` et `temps écoulé`), et l'affichage de l'écran de fin non plus. En attendant, le scénario détecte GO par la première ligne `QA ips=… vitesse=[1-9]` avec le gaz tenu, et l'écran de fin par `course : temps écoulé` suivi de 2 lignes `QA ips`. Une ligne `course : GO` serait plus sûre ; c'est une piste pour WFS-59 ou un ticket E3.
- Essai : `simulateur.yml` sur `feature/waforspeed-58` (`epic=WFS-58`, `scenario=mvp`, run 41705761615). Tant que WFS-59 n'est pas fusionné, un ÉCHEC franc est le résultat attendu.

### 2026-11-03 19:47:29 — Règle : heure complète dans chaque entrée du Journal (Claude, agent 3, à la demande de l'utilisateur)
- La règle de `CLAUDE.md` (date et heure UTC à la seconde, `### AAAA-MM-JJ hh:mm:ss — titre (auteur)`, via `date -u "+%Y-%m-%d %H:%M:%S"`) n'était pas appliquée : les entrées récentes n'avaient que la date.
- Rappel ajouté là où les agents la lisent : en-tête de `journal.md`, `TACHES.md` (section Journal), skill `waforspeed-natif` (point 8), agents `po-scrum-master` et `qa-lead`.
- Les anciennes entrées ne sont pas modifiées (heure inconnue, on ne la devine pas). La règle s'applique à toutes les nouvelles entrées.

### 2026-11-03 19:50:27 — PO : début WFS-59 Point d'extension mode GPU logiciel (agent 3) — attribué par le PO
- **Choix** : WFS-59 est le ticket ⏳ Prêt de meilleure priorité (0.438, MVP) dont l'épic est libre : E0 n'a aucun ticket actif ; E5 (WFS-52 en revue) et E7 (WFS-58 en cours) sont occupées par la session trafic. Dépendances : aucune. Il débloque WFS-50 (vérification simulateur du MVP) avec WFS-58.
- **Ticket** : WFS-59 · épic E0 · branche `feature/waforspeed-59` · règles `regles-de-gestion/WFS-59.md`.
- **Zone autorisée** : `src/core/**`, `src/game/game.*`, `src/render/renderer.h` (API), `CMakeLists.txt` ; accroches ≤ 10 lignes annoncées au Journal dans `src/render/vk.cpp` (détection SwiftShader / llvmpipe, rendu 25 %, sans ombres) et `src/platform/android_main.cpp` (extra `qa_simu`). Aucun fichier de `tools/qa/**` ni `.github/**` (WFS-58).
- **Critères** : banc PC identique à `main` (0 écart, mode inactif) ; simulateur `scenario=mvp` : ligne `mode GPU logiciel : actif (dt max 0,5 s, rendu 25 %, sans ombres)` dans le logcat, `QA ips` ≥ 1, décompte puis course qui avance (chrono < 30,0, vitesse > 0).
- **Plan** : booléen `gpuLogiciel()` dans `renderer.h` → accroche `vk.cpp` (renderScale 0,25 + forceNoShadows) → accroche `android_main.cpp` (`qa_simu`) → plafond dt 0,5 s dans `Game::update` (pas de 1/120 s, ≤ 60 pas par image) → banc PC + simulateur sur la branche → contrôle rapide, fusion, ajout à `qa/Check.md`.
- Piste notée par WFS-58 (hors périmètre ici sauf accord) : une ligne de logcat `course : GO` faciliterait le scénario ; elle relève d'E3 (`src/game/course_system.cpp`).

### 2026-11-03 19:53:50 — Début WFS-45 Lumières : spots, hémisphère, brouillard linéaire (Claude, agent 3, session 011cpqZan) — attribué par le PO
- **Choix** : WFS-59 (E0, 0.438) était déjà attribué à 19:50:27 à « agent 3 » d'une autre session (011cpqZdegh) : E0 occupée, pas de double attribution. E5 (WFS-52 en revue) et E7 (WFS-58) occupées. WFS-45 est le ⏳ Prêt suivant (0.46), épic E4 libre, sans dépendance ; catalyseur de WFS-11 (garage) et WFS-09 (nuit).
- **Ticket** : WFS-45 · épic E4 · branche `feature/waforspeed-45` · règles `regles-de-gestion/WFS-45.md`.
- **Zone autorisée** : `src/render/**` (sauf `ui_*`), `shaders/scene*`, `sky*`, `post*`, `shadow*` ; nouveau banc `src/render/bench_spots.*` (CMake : glob, rien à changer). Accroches ≤ 10 lignes annoncées au Journal : champs de `FrameParams` dans `src/render/renderer.h` (RG-07) et enregistrement de la capture `spots-sol` dans `src/platform/desktop_main.cpp`.
- **Coordination WFS-59** (E0, en cours) : WFS-59 accroche `renderer.h` (booléen GPU logiciel) et `src/render/vk.cpp` (rendu 25 %, sans ombres). WFS-45 ne touche pas à ce code ; synchroniser souvent avec `main`, conflit de fusion à résoudre en gardant les deux.
- **Critères** : captures existantes identiques à `main` (0 spot par défaut) ; capture `spots-sol` (valeurs de `js/world/garage.js` → `buildGarage`) ; passe de scène ≤ +0,3 ms avec 3 spots.
- **Plan** : FrameParams (4 spots, hémisphère, fogNear/fogFar) → `shaders/scene.frag` (formules three r128) → banc `spots-sol` → contrôle rapide, fusion, ajout à `qa/Check.md`.

### 2026-11-03 19:56:48 — WFS-59 : mode GPU logiciel codé, PR #31 (agent 3)
- **Fait** : `src/core/gpu_logiciel.h` (drapeau du mode, plafond de dt 0,05 s → 0,5 s en mode, 60 pas de 1/120 s au plus par image) et `src/core/gpu_logiciel_android.cpp` (extra d'intent `qa_simu`, `--ez` ou `--es`). `Game::update` utilise `plafondDt` ; `Game::init` coupe les ombres et écrit la ligne de logcat `mode GPU logiciel : actif (dt max 0,5 s, rendu 25 %, sans ombres)`. `Renderer::gpuLogiciel()` dans `renderer.h`.
- **Accroches annoncées (≤ 10 lignes)** : `src/render/vk.cpp` l. 85-92 (SwiftShader/llvmpipe → mode ; en mode : sans MSAA, rendu 25 % au lieu de 50%) — zone E4 (WFS-45 en cours) : conflit éventuel à résoudre en gardant les deux ; `src/platform/android_main.cpp` (intent lu au début d'`android_main`, avant Vulkan).
- **Vérification** : `test_systems` OK en local (hors mode inchangé ; 360 images à 0,3 i/s = 180 s de jeu). Compilation `-fsyntax-only` OK ; JNI et APK vérifiés par la CI. Banc PC : mode inactif (hors écran).
- **Limites** : le simulateur ne passe `qa_simu=true` que si le scénario de WFS-58 l'ajoute à `am start` ; sans cela, la détection SwiftShader suffit.

### 2026-11-03 19:56:51 — Renommage : la session 011cpqZan est « agent 4 » (Claude, agent 4, à la demande de l'utilisateur)
- L'utilisateur précise que cette session est **agent 4**. « Agent 3 » est la session qui tient WFS-59 (E0).
- Responsable de **WFS-45** (E4) corrigé dans `TACHES.md` : « Claude (agent 4, session 011cpqZan) ».
- Les entrées précédentes de cette session au Journal (19:47:29, règle sur l'heure ; 19:53:50, début de WFS-45), signées « agent 3 », sont de l'agent 4. Elles ne sont pas réécrites.

### 2026-11-03 20:00:50 — QA WFS-52 : VALIDÉ (qa-lead)
- Construction `d2b19dc` (PR #30, run 41699480375 vert), Release `apercu-waforspeed-52` (63 captures) regardée.
- `encoche-hud-2400` : bande de 90 px vide de HUD (panneau Temps à x ≈ 114, vitesse centrée entre les marges). Autres captures comparées à `apercu-waforspeed-57` : bruit de rendu ≤ 1/255 seulement (le même qu'entre deux `main`), banc `0 écart(s)`.
- RG-01 à RG-03 respectées ; l'accroche d'1 ligne de `build.yml` (annoncée) est acceptée à la place de `tools/qa/**`.
- Aucune anomalie : **fusion autorisée**. Rapport : `qa/rapports/WFS-52.md`.

### 2026-11-03 20:03:17 — PO : début WFS-48 Texte en italique (agent 5) — attribué par le PO ; fin de WFS-52
- **Fin de WFS-52** (cérémonie PO) : QA VALIDÉ (`qa/rapports/WFS-52.md`) et PR #30 fusionnée (`318ac8b`) → ✅ Terminé. L'épic E5 est libre.
- **Choix** : épics occupées : E0 (WFS-59 en revue), E4 (WFS-45), E7 (WFS-58). WFS-48 (E5, 0.48) est le ⏳ Prêt de meilleure priorité dans une épic libre ; dépendance WFS-26 ✅. (Une première attribution de WFS-39 a été préparée puis annulée avant envoi quand la fusion de WFS-52 a libéré E5 ; WFS-39 reste ⏳ Prêt, règles affinées : zone, `Ecran::Pause` déjà dans le noyau.)
- **Ticket** : WFS-48 · épic E5 · branche `feature/waforspeed-48` · règles `regles-de-gestion/WFS-48.md` · corrige ANO-04.
- **Zone autorisée** : `src/ui/**` (sauf `src/ui/ecrans/`), `shaders/ui*`. Accroche E3 ≤ 10 lignes à annoncer au Journal dans `src/game/course_system.cpp` (décompte et écran de fin en italique).
- **Critères** : `course-decompte-3`, `course-decompte-go`, `course-fin`, `course-*` : chiffres inclinés de 12°, nets, sans débordement ; `texte-essai-*` : ligne droite + ligne italique ; autres captures inchangées.
- **Plan** : cisaillement des quads de glyphe dans `src/ui/text.*` (option, droit par défaut, largeur inchangée) → HUD → accroche décompte / fin → banc → contrôle rapide, fusion, ajout à `qa/Check.md`.
- **Ménage** : marqueur de conflit orphelin `
### 2026-11-03 20:07:56 — WFS-48 : italique codé, PR #33 en revue (Claude, agent 5)
- **Fait** : `ui::text(..., italique)` : glyphes cisaillés de 12° autour du milieu des capitales, droit par défaut, `ui::width` inchangée. Italique activé là où le web l'a (`css/style.css`) : HUD (`.time-value`, `.score-value`, `.combo`, `.speedo-read`), titre des bandeaux (`.toast b` = le titre, pas la valeur), décompte, score de fin (`.score-big`), valeurs des stats (`.stats dd`). Planche `texte-essai-*` : ligne droite + ligne italique.
- **Accroches annoncées** : noyau `src/render/renderer.h/.cpp` (4 lignes) : `Renderer::uiGlyph(..., float biais = 0)` décale le bord haut du quad (sans conflit avec la PR #31 de WFS-59) ; E3 `src/game/course_system.cpp` (7 lignes) : décompte, score et stats de fin en italique.
- **Fichiers** : `src/ui/text.h`, `src/ui/text.cpp`, `src/ui/hud.cpp`, `src/ui/toasts.cpp`, `src/render/renderer.h`, `src/render/renderer.cpp`, `src/game/course_system.cpp`.
- **Vérification** : compilation `-fsyntax-only -Wall` des 5 `.cpp` touchés sans erreur ; captures et APK : en attente de la CI.


### 2026-11-03 — WFS-52 fusionné ; WFS-58 : essais simulateur 1 à 3 (Claude, session trafic)
- **WFS-52** : VALIDÉ par qa-lead, PR #30 fusionnée (318ac8b).
- **WFS-58, essai 1** : faux échec juste après `am start` (le processus n'existait pas encore). Corrigé : l'appli n'est déclarée arrêtée qu'après 10 s de suite sans processus.
- **WFS-58, essai 2** (run 41707107542) : le scénario suit bien le jeu. Chargement, menu, « Course solo » et décompte sont atteints, avec leurs captures. L'écran réduit (1200x540) fait passer le jeu de 0,3 à **1,2 image/s**. Échec franc à l'étape « go » : Android affichait la bulle « Tap to restart this app for a better view », déclenchée par le changement de densité, sur l'accélérateur, et le gaz n'arrivait pas. Corrigé : la densité ne change plus et le gaz est appuyé en bas du bouton (0,84 W ; 0,92 H).
- **WFS-58, essai 3** lancé. Sans WFS-59 (PR #31), la course de 30 s dure encore environ 8 min réelles à 1,2 image/s (plafond de 0,05 s par image), un ÉCHEC franc à l'étape « fin » reste donc possible. Le scénario envoie déjà `--ez qa_simu true` pour WFS-59.

### 2026-11-03 20:09:37 — Règle : autonomie des agents, pas de confirmation demandée à l'utilisateur (Claude, agent 4, à la demande de l'utilisateur)
- Les agents suivent le processus sans demander à l'utilisateur « je lance ? » ou « je continue ? » : ils agissent puis rendent compte.
- L'utilisateur n'est sollicité que pour un vrai blocage (décision produit non tranchée par les règles ni par le PO, accès manquant, action irréversible hors processus), avec une seule question précise et une recommandation. Les questions de règles et de priorité vont au PO.
- Règle ajoutée dans `CLAUDE.md` (en tête des consignes), `TACHES.md` (règle 10), skill `waforspeed-natif`, agents `po-scrum-master` et `qa-lead`.
- **Ménage** : marqueur de conflit orphelin `### 2026-11-03 — WFS-58 : correction du tour 2 (WFS58-5) ; à l'attention du PO (Claude, session trafic)
- **WFS58-5** (bloquante, tour 2) : `01-chargement` était prise avant l'affichage du texte. Elle attend maintenant la ligne `chargement : … écran complet (police)` puis 1 s (cc78b4d, branche synchronisée avec main). Essai `mvp` relancé, puis QA.
- **Pour le PO (WFS-64)** : selon le QA, le gel juste après le chargement apparaît aussi **sans WFS-59**, dans l'essai `conduite` à ffb5474 (« Pixel Launcher isn't responding », journal arrêté sur la recréation de la swapchain, captures 02 à 07 identiques). La cause n'est donc peut-être pas WFS-59 : il faut étudier aussi la piste d'un émulateur saturé ou d'une fenêtre système qui prend le focus. Le scénario `conduite` (inchangé par WFS-58, RG-06) conclut OK sur des captures figées : faux OK à reprendre dans un ticket E7.

### 2026-11-03 20:08:40 — PO : création de WFS-60, WFS-61, WFS-62 (roues animées) (po-scrum-master)
- **Besoin** (utilisateur) : animer les roues quand la voiture roule ou s'arrête. Aucune US n'existait ; priorité **basse** (hors MVP, 17.6 à 17.8).
- **Constat** : le natif ne sépare pas les roues (drapeau « roue » au niveau du matériau seulement) ; le jeu web a déjà `splitWheels` et `animWheels` (Ackermann, rotation = distance / rayon).
- **Découpage** : WFS-60 (E5, roues séparées dans le `.wfm`, ⏳ Prêt) → WFS-61 (E0, accroche de rendu par roue) → WFS-62 (E3, animation). Règles : `regles-de-gestion/WFS-60.md` à `WFS-62.md`.

### 2026-11-03 — Point des PR et WFS-58 en revue, PR #32 (Claude, session trafic)
- PR fusionnées depuis le dernier point : **#30 (WFS-52)**, déjà marquée Terminé. PR ouvertes : #31 (WFS-59, agent 3), #32 (WFS-58), #33 (WFS-48, agent 5). Le tableau est à jour pour toutes.
- **WFS-58** : PR #32 (ouverte par l'utilisateur, titre et description complétés) → 🔍 En revue. L'essai simulateur 3 est en cours. La QA viendra après, et un ÉCHEC franc reste attendu tant que WFS-59 n'est pas fusionné.

### 2026-11-03 — Début WFS-39 : pause en course et retour au menu (po-scrum-master → Claude, session trafic)
- **Ticket** : WFS-39 · épic E8 · priorité 0.6 · branche `feature/waforspeed-39` · règles `regles-de-gestion/WFS-39.md`.
- **Choix** : meilleure priorité prête d'une épic libre. WFS-46 (0.49) écarté : E5 occupée par WFS-48 (en revue). E7, E0, E4 occupées (WFS-58, WFS-59, WFS-45). L'agent garde WFS-58 en revue en parallèle, à la demande de l'utilisateur.
- **Zone autorisée** : `src/ui/ecrans/**` ; accroches ≤ 10 lignes annoncées au Journal : `src/platform/android_main.cpp` (APP_CMD_PAUSE / APP_CMD_LOST_FOCUS → `Ecran::Pause`) et enregistrement des scènes `pause-*` du banc (comme WFS-38).
- **Critères** : `pause-bouton` (bouton Menu hors encoche), `pause-0` (3 boutons), `pause-reprise` (même chrono), `pause-quitter` (retour au menu).
- **Plan court** : utiliser `Ecran::Pause` et le gel de `Game::update` (WFS-36), marges WFS-51, boutons de l'écran de fin WFS-38 / WFS-57 ; porter `pauseGame` / `resumeGame` / `toMenu` de `js/ui/screens.js` (waaps).

### 2026-11-03 20:13:45 — Règle : synchroniser la branche avec main avant chaque CI (Claude, agent 4, à la demande de l'utilisateur)
- Avant chaque `git push` de branche, chaque PR et chaque lancement de workflow (`build.yml`, `simulateur.yml`…) : `git fetch origin && git merge origin/main`, conflits résolus en gardant les deux modifications, tests locaux relancés, puis seulement pousser.
- Une CI lancée sur une branche en retard sur `main` ne compte pas : ses captures et son verdict ne sont pas valables.
- Ajouté dans `CLAUDE.md`, `TACHES.md` (règle 4) et le skill `waforspeed-natif` (point 4).

### 2026-11-03 — WFS-39 en revue, PR #34 (Claude, session trafic)
- `src/ui/ecrans/pause.*` : bouton « Menu », écran de pause, « Reprendre », « Recommencer » (Pause → Menu → Course, le menu n'est pas dessiné) et « Quitter la course ». Chrono figé pendant la pause, vérifié au journal du banc.
- **Accroches annoncées** : `src/platform/android_main.cpp` (`APP_CMD_PAUSE` / `LOST_FOCUS` en course → Pause, 4 lignes) et `src/platform/desktop_main.cpp` (`benchPause` + include, 3 lignes). QA après la CI.
- WFS-58 (PR #32) : l'essai simulateur 3 atteint le GO, puis échoue franchement à l'étape « fin » (attendu sans WFS-59). La QA est lancée.
### 2026-11-03 20:17:58 — PO : tableau de TACHES.md trié par numéro de ticket (po-scrum-master)
- Demande de l'utilisateur : tableau affiché dans l'ordre des numéros WFS-XX. Fait ; la colonne « Prio » reste l'ordre de travail, la colonne « Épic » existait déjà. Aucun statut ni responsable modifié.

### 2026-11-03 20:20:43 — PO : priorités à la Jira (P1 Urgent à P4 Basse) dans TACHES.md (po-scrum-master)
- Demande de l'utilisateur. Colonne « Priorité » : P1 Urgent (bloque le MVP : WFS-50, 58, 59), P2 Haute (garage, pause, terrain, compteur, italique, lumières), P3 Moyenne (nuit, 20 voitures, performances, cartes, bruitages, piétons), P4 Basse (roues, arbres, météo, post-traitement, duel, sauvegarde, iOS, stores). À égalité : plus petit numéro de ticket. `po-scrum-master.md` mis à jour. Aucun statut ni responsable modifié.

### 2026-11-03 20:24:41 — WFS-45 en revue, PR #35 (Claude, agent 4)
- **Fait** : `FrameParams` gagne 4 projecteurs (three `SpotLight`), une lumière hémisphère et `fogNear` / `fogFar`. UBO, `common.glsl` et `scene.frag` étendus : cône lissé, portée, GGX du soleil, `HemisphereLight`, `Fog` linéaire. Banc `src/render/bench_spots.*` : éclairage du garage du jeu web (`garage.js`, `buildGarage`), capture `spots-sol` et mesure du coût sans projecteur puis avec 3.
- **Accroches annoncées** : `src/render/renderer.h` (5 lignes dans `FrameParams`, RG-07) et `src/platform/desktop_main.cpp` (2 lignes : include et appel de `benchSpots`).
- **Écart à RG-02, pour rester fidèle au jeu web** : le jeu web n'active pas `physicallyCorrectLights`. three r128 applique donc `pow(saturate(1 − d/portée), decay)` et une irradiance × PI. J'ai porté ce comportement plutôt que la formule physique de RG-02, qui donnerait un garage environ 10 fois plus sombre. À confirmer par le PO dans le fichier de règles.
- **Vérification** : compilation C++ et `test_systems` réussis en local, branche synchronisée avec `main` avant le push. Shaders, banc et APK : CI de la PR #35 en cours.

### 2026-11-03 — WFS-58 : corrections du tour 1 (REFUSÉ) ; WFS-39 comparé à main (Claude, session trafic)
- **WFS-58** (ffb5474, branche synchronisée avec main) :
  - **WFS58-1** : plus de `wm size`. Avec ou sans changement de densité, Android passait l'appli en mode de compatibilité et posait la bulle « Tap to restart this app » sur la moitié droite de l'écran. La vitesse sur GPU logiciel viendra du rendu à 25 % de WFS-59.
  - **WFS58-2** : chaque étape trace le motif attendu et la ligne du journal du jeu trouvée, avec l'heure. Attente de 2 s avant `01-chargement`.
  - **WFS58-3** : `03` est prise dès l'appui, `04` après des images.
  - **WFS58-4** : la taille d'écran est tracée en fin de scénario.
  - Essais relancés : `scenario=mvp` (Release `simulateur-WFS-58`) et `scenario=conduite` (Release `simulateur-WFS-58-conduite`).
- **WFS-39** (PR #34), banc local comparé à main : 11 captures de vraie course ne changent que dans la zone du bouton « Menu » (`course-decompte-*`, `hud-*`, `encoche-hud`, `menu-depart-*`, `toast-*`) ; 5 nouvelles captures `pause-*` ; aucune autre différence. Branche synchronisée avec main avant la CI (nouvelle règle 2384f67).

### 2026-11-03 20:41:47 — Fin WFS-59 : fusionné (PR #31), ajouté à qa/Check.md (agent 3)
- **Contrôle rapide** : CI verte sur `14056c9` (android, banc-essai avec tous les tests, publication `apercu-waforspeed-59`). Banc : GPU llvmpipe hors écran → mode inactif (« MSAA x4 », scène 2400x1080, ombres présentes) ; captures `course-cam0-0`, `hud-course`, `ombres-a`, `menu-0` regardées : rien de bloquant.
- **Limite** : comparaison au pixel avec `main` non faite (Release `apercu` en reconstruction au moment de la fusion) : à faire par la recette de nuit. Comportement sur le simulateur à mesurer avec WFS-50 (et le scénario de WFS-58).
- **Fusion** : `b2ede73`. WFS-50 ne dépend plus que de WFS-58.

### 2026-11-03 20:43:48 — Début WFS-28 attribué à agent 3 (session 011cpqZd) ; WFS-19 remis en attente, WFS-63 créé (po-scrum-master)
- **Épics occupées** : E4 (WFS-45), E5 (WFS-48), E7 (WFS-58), E8 (WFS-39). Libres : E0, E2, E3, E6.
- **WFS-19 (E2) n'est pas réellement prêt** : les personnages du jeu web sont des maillages à squelette, le renderer natif n'en dessine pas (rien dans `renderer.h`), et le ticket dépasse une journée. Il passe en ⏳ En attente de **WFS-63** (E0, P3, ⏳ Prêt, `regles-de-gestion/WFS-63.md`) : point d'extension « maillages animés par squelette », catalyseur des piétons et des conducteurs. E0 retrouve ainsi un ticket prêt.
- **Attribution** : **WFS-28 — Bruitages** (E6, P3 Moyenne), branche `feature/waforspeed-28`. Zone autorisée : `src/audio/**`, `assets/audio/**` ; accroche ≤ 10 lignes dans `src/game/course.*` seulement si un compteur manque (RG-09, à annoncer ici).
- **Règles** : `regles-de-gestion/WFS-28.md` complété (RG-08 à RG-11 : événements lus par front montant sur `activeCourse()` et `GameView::player`, sons synthétisés au démarrage par bruit filtré comme `js/audio/sfx.js`, mixés dans le flux AAudio de WFS-10, ligne `SFX <nom>` au journal du banc).
- **Critères** : chaque événement déclenche son son une seule fois (journal du banc) ; pas de craquement ; coupure en pause ; captures inchangées, `0 écart(s)`.
- **Plan court** : 1) `noiseHit` / `beep` natifs (tables pré-calculées) ; 2) détection des événements ; 3) mixage et coupure en pause ; 4) journal du banc.

### 2026-11-03 20:49:47 — WFS-28 : bruitages codés, PR #36 (agent 3, session 011cpqZd)
- **Fait** : `src/audio/sfx_synth.h` (noiseHit / beep de `js/audio/sfx.js`, fondus de 5 ms, boucles raccordées), `src/audio/sfx.h/.cpp` (sons calculés au démarrage, file sans verrou, 16 voix, coupure en pause, setMuted ; un son par changement d'état de la course : décompte, GO, checkpoint, fin, accident, choc, frottement, quasi-contact ; crissements et souffle de nitro ; « SFX <nom> » au journal). Mélange dans le rappel AAudio de `audio_system.cpp` seulement (`moteur.wav` du banc inchangé).
- **Accroche annoncée (4 lignes, zone E3)** : `src/game/course.h` (compteurs `bumps`, `scrapes`, `lastImpact`) et `src/game/course.cpp` (`reset`, `bump`, `applyHits`). Aucune règle de course changée.
- **Vérification** : `test_audio` OK en local (6 bruitages : bouts à zéro, raccords de boucle, crête) ; essai du mélangeur : silence après pause. CI en cours.
- **Limites** : contact et arrivée en duel préparés, pas encore joués (pas de duel) ; vibration non faite (hors `src/audio`).

### 2026-11-03 — WFS-58 tour 2 ; WFS-39 resynchronisé après WFS-59 (Claude, session trafic)
- **WFS-58** :
  - Essai `mvp` sans `wm size` (run 41714023261) : plus de bulle Android. Le scénario atteint chargement, menu, « Course solo », décompte et GO (195 s), chaque étape étant tracée avec la ligne du journal du jeu. Il échoue franchement à l'étape « fin », comme attendu : il était parti 3 min avant la fusion de WFS-59.
  - Scénario `conduite` : OK, 7 captures.
  - Un essai intermédiaire n'a rien produit : l'émulateur de la CI n'a pas démarré (panne de CI).
  - QA tour 2 en cours. Nouvel essai `mvp` lancé sur la branche resynchronisée avec WFS-59 (3d447b1, Release `simulateur-WFS-58-avec-59`) pour voir le parcours complet.
- **WFS-39** : branche resynchronisée avec main après la fusion de WFS-59 (b4fc8f6), comme l'exige la nouvelle règle : la CI précédente ne compte plus. Tests et banc `menu` en local : captures `pause-*` identiques au pixel. CI relancée.

### 2026-11-03 21:01:08 — WFS-64 créé : simulateur figé par le mode GPU logiciel de WFS-59 ; repli du rendu à 50% (po-scrum-master)
- **Signalé par** Claude (session trafic), preuve du scénario `mvp` de WFS-58 : Releases `simulateur-WFS-58` (run 41714023261, sans WFS-59 : scène 1200x540, « Écran : course → menu », `QA ips=0.3`, GO atteint) et `simulateur-WFS-58-avec-59` (run 41716390682 : mode actif, scène 600x270, puis plus aucune ligne au logcat, ni erreur ni plantage ; échec à l'étape « menu » après 180 s).
- **Décision du PO** : défaut **bloquant pour le MVP**, ticket **WFS-64** (E0, P1 Urgent, ⏳ Prêt, `regles-de-gestion/WFS-64.md`). Repli immédiat : en mode GPU logiciel le rendu revient à **50%** (valeur qui tournait), dt max 0,5 s et ombres coupées gardés ; deux lignes de diagnostic (fin de la 1re mise à jour, 1re image après recréation de la swapchain) pour trancher « rendu ou 60 pas » ; si figé avant la fin de la mise à jour, `PAS_MAX` à 20. Réparer le 25 % est hors périmètre (ticket E4 seulement si besoin).
- **Pourquoi E0** : la constante est dans `src/core/gpu_logiciel.h` ; accroches ≤ 10 lignes annoncées dans `vk.cpp`, `renderer.cpp`, `game.cpp`. E0 libre (WFS-63 Prêt en P3 passe après).
- **Effets** : WFS-59 reste fusionné, marqué « fige le simulateur, corrigé par WFS-64 ». **WFS-50** dépend désormais de WFS-58 et **WFS-64** (⛔ Bloqué). Rien ne change sur un vrai GPU ni sur le banc PC.
### 2026-11-03 21:00:40 — PO : le MVP n'attend plus le simulateur (po-scrum-master)
- **Décision de l'utilisateur** : il teste lui-même l'APK de `main` sur téléphone. WFS-50 (vérification simulateur) suspendu, P4 ; WFS-58 (PR #32) sans urgence, P4 ; WFS-59 déjà fusionné. Le MVP est jouable dès maintenant sur `main` (WFS-35, 27, 37, 29, 38, 47, 57 terminés). `CLAUDE.md` mis à jour. Les anomalies du rapport `simulateur-MVP` (lenteur de l'émulateur) ne sont plus bloquantes.

### 2026-11-03 21:02:56 — PO : regroupement en gros tickets (po-scrum-master)
- **Décision de l'utilisateur** : fini les micro-tickets (≈ 30 min de build chacun). Regroupés (le ticket qui absorbe lit aussi les règles des absorbés, une seule CI) : WFS-09 ← 16 ; WFS-15 ← 20, 17 ; WFS-53 ← 54, 43 ; WFS-19 ← 18 ; WFS-11 ← 40, 13, 46 ; WFS-24 ← 21 ; WFS-22 ← 23, 25 ; WFS-61 ← 62. Tickets en cours ou en revue non touchés. Règle ajoutée à `po-scrum-master.md` : tickets de 1 à 3 jours, jamais de micro-ticket.

### 2026-11-03 21:03:05 — Début WFS-63 : point d'extension skinning, attribué à Claude (session trafic) (po-scrum-master)
- **Demande** : WFS-64. **Refusée par le PO** : depuis la décision utilisateur de 21:00:40 (le MVP n'attend plus le simulateur), WFS-64 est P4 Basse ; le ticket ⏳ Prêt le plus prioritaire d'E0 (libre) est **WFS-63 (P3 Moyenne)**, attribué à la place. WFS-64 reste ⏳ Prêt.
- **Épic** E0 ; **zone autorisée** : `src/render/renderer.h` (API), `src/render/**` pour le pipeline skinning, `shaders/` (variantes de `scene.vert` / `shadow.vert`), `tools/` (convertisseur), `assets/` (personnages convertis), CMake si besoin ; doc dans le skill `waforspeed-natif`.
- **Critères** : captures `squelette-essai` (homme en `Idle_Loop`), `squelette-marche-0/1` (poses `Walk_Loop` différentes), captures existantes identiques, banc `0 écart(s)`.
- **Plan** : API maillage à os (RG-01) → shader (RG-02) → conversion `models/people/*.json` de `wlfaleh/waaps` (RG-03, aucun personnage recréé) → lecture d'animation CPU (RG-04) → mesure (RG-05). Règles : `regles-de-gestion/WFS-63.md`.
- Note : ses tickets WFS-58 (PR #32) et WFS-39 (PR #34) sont en revue sans code en cours ; pas de conflit d'épic (E7, E8).
### 2026-11-03 21:04:53 — PO : 4 nouveaux gros sujets (WFS-65 à 68) (po-scrum-master)
- Écart avec le jeu web comblé : **WFS-65** effets de conduite (E4), **WFS-66** caméras et cockpit (E3), **WFS-67** trafic vivant (E3, après WFS-60), **WFS-68** réglages (E8, après WFS-39). Règles : `regles-de-gestion/WFS-65.md` à `WFS-68.md`. Tickets de 1 à 3 jours, une seule CI chacun. Aucun ticket en cours ou terminé touché.

### 2026-11-03 21:05:42 — PO : les 6 sujets importants (map, caméra, personnages, menus, duel, garage) (po-scrum-master)
- **Demande de l'utilisateur** : 6 sujets distincts, en P2 Haute. **Map design 3D** : WFS-53 (Campagne, Grand Prix, arbres). **Caméra** : WFS-66. **Personnages** : WFS-19 (piétons et conducteurs, après WFS-63 squelettes ; la pluie redevient WFS-18, séparée). **Menus** : WFS-41 (terrain, moment, réglages ; absorbe WFS-68). **Mode duel** : WFS-21 redevenu un ticket à part (séparé de WFS-24 sauvegarde). **Garage** : WFS-11. Aucun ticket en cours ou terminé touché.

### 2026-11-03 21:06:35 — PO : WFS-69 créé, CI qui annule les builds (po-scrum-master)
- Constat (runs 390 à 419) : chaque commit sur `main`, même documentation, lance un build de ~30 min qui annule celui en cours ; push + PR lancent deux builds identiques. Les commits du PO y ont contribué. **WFS-69** (E7, P1 Urgent) : pas de build pour la documentation, un seul build par commit, jamais d'annulation sur `main`. Retours des agents 3, 4 et 5 demandés (attente et idées).

### 2026-11-03 21:07:16 — PO : retours des agents 3, 4, 5 intégrés à WFS-69 (po-scrum-master)
- Mesures concordantes : banc d'essai 37 à 52 min (Captures du rendu en série sur lavapipe) ; Android < 2 min ; doublon push + PR ; commits de documentation qui annulent `main`. **WFS-69** élargi (RG-06 à RG-10) : banc en matrice de jobs parallèles par groupe de scènes (cible ≈ 10-15 min), `WFS_CAPTURES`, moins d'images inutiles, un seul banc par commit, aucune annulation sur `main`.

### 2026-11-03 21:00:20 — QA WFS-58 tour 2 : REFUSÉ (QA lead)
- Captures regardées : `simulateur-WFS-58` (a51bbc8), `simulateur-WFS-58-avec-59` (3d447b1) et `simulateur-WFS-58-conduite` (ffb5474).
- Les anomalies du tour 1 sont corrigées : plus de bulle Android, déroulement tracé (motif, ligne, heure), 03 et 04 séparées, écran tracé en fin de scénario. L'échec reste franc sur les deux essais `mvp`.
- **Nouvelle anomalie bloquante, WFS58-5** : `01-chargement` est presque noire (capture prise 2 s après « Cibles de rendu », avant la police). Il faut attendre `chargement : première image … écran complet (police)`. [Rapport](qa/rapports/WFS-58.md).
- Pour le PO, hors WFS-58 : le gel du jeu avec la fenêtre « Pixel Launcher isn't responding » apparaît aussi sans WFS-59 (essai `conduite` à ffb5474). Le scénario `conduite` conclut OK sur des captures figées.
### 2026-11-03 21:08:15 — PO : on vise le fonctionnel, pas la perfection (po-scrum-master)
- **Décision de l'utilisateur** : voir des choses qui fonctionnent. Principe 0 ajouté à `po-scrum-master.md` (fini = ça marche et se voit sur téléphone, pas de finitions ni de fidélité au pixel). WFS-69 ordonné : d'abord le minimum (pas de build pour la doc, un banc par commit, `main` jamais annulé), le banc parallèle ensuite.

### 2026-11-03 21:11:56 — Fusion des 5 PR ouvertes sur ordre de l'utilisateur, pipeline lancée sur main (Claude, agent 4)
- **Ordre de l'utilisateur** : fusionner toutes les PR en cours, mettre à jour le suivi, puis lancer la pipeline sur `main`. Fusion rapide : la QA se fera la nuit, après fusion.
- **Fusionnées dans cet ordre, en local puis poussées** : WFS-58 (#32), WFS-48 (#33), WFS-39 (#34), WFS-45 (#35), WFS-28 (#36).
- **Un seul conflit** : `src/platform/desktop_main.cpp`, où `benchPause` (WFS-39) et `benchSpots` (WFS-45) s'ajoutaient au même endroit. Les deux appels sont gardés.
- **Vérification avant l'envoi** : syntaxe C++ de tous les `src/**/*.cpp` (hors Android) sans erreur ; `test_audio`, `test_course`, `test_systems` OK ; `bash -n tools/qa/emulateur.sh` OK. Les shaders, le banc et l'APK seront vérifiés par la pipeline de `main`.
- **Point d'attention** : WFS-58 est fusionné alors que son dernier passage QA était **REFUSÉ** au tour 2 (WFS58-5, `01-chargement` presque noire). Il est remis à la QA de nuit ; s'il reste bloquant, le PO crée un ticket de correction prioritaire.
- **Les CI des PR n'étaient pas terminées** au moment de la fusion. La pipeline de `main` sert de contrôle.
- **Suivi** : `TACHES.md` passe WFS-58, 48, 39, 45 et 28 à ✅ Fusionné (QA de nuit à faire) ; les 5 tickets sont ajoutés à `qa/Check.md`.

### 2026-11-03 21:13:18 — Fin WFS-28 : fusionné sur ordre de l'utilisateur (PR #36) ; marqueur de conflit retiré du journal (agent 3, session 011cpqZd)
- WFS-28 fusionné par l'utilisateur à 21:12:25, CI de la PR non terminée (Android ✅). Suivi déjà fait par l'agent 4 (`TACHES.md` ✅, `qa/Check.md`). Contrôle : la pipeline de `main` doit montrer les lignes `SFX bip-decompte`, `SFX bip-go`, `SFX quasi-contact`, `SFX accident`, `SFX bip-fin`… dans le journal du banc, et `test_audio` vert.
- `journal.md` : une ligne orpheline `### 2026-11-04 00:52:11 — WFS-74 : chargement au changement de carte, course figée — PR #50 en revue (Claude, agent 5)
- **Fait** : `Game::changerCarte` (zone E0) : écran de chargement branché par la plateforme via `Game::demandes()` (`debutChargement`, `progression`, `finChargement`), étapes réelles de `init`, voiture et peinture gardées, durée au journal ; même carte et même moment = pas de reconstruction. La course n'avance que sur l'écran Course (menu, garage, terrain : figée) ; Terrain → Course = course neuve.
- **Accroches annoncées** : `src/game/course_system.cpp` (E3, 5 lignes : `onEcran`, test d'écran dans `collide`) ; `src/platform/android_main.cpp` (6 lignes : branchement du chargement, message rouge si échec) ; `src/platform/desktop_main.cpp` (3 lignes) ; nouveau `src/game/bench_changement_carte.*` (banc).
- **Vérification** : compilation sans erreur, `test_systems` et `test_course` OK en local ; Android et captures : CI de la PR #50. Fusion après CI verte (noyau et Android non testables ici : confiance ≤ 50 %).

### 2026-11-03 21:14:33 — Début WFS-69 : CI banc en jobs parallèles, fin des builds inutiles (PO / Scrum Master)
- Ticket : WFS-69 · épic E7 · P1 Urgent · attribué à agent 3 (session 011cpqZd) · branche `feature/waforspeed-69` · règles `regles-de-gestion/WFS-69.md`.
- Zone autorisée : `.github/**`, `tools/qa/**` (E7). Toucher au banc (`WFS_BANC` groupes) côté code de jeu = ligne d'accroche ≤ 10 lignes annoncée au Journal, sinon ticket E0.
- Critères : RG-01 à RG-05 d'abord (pas de build pour la doc, un build par commit, `main` jamais annulé), puis RG-06 (banc en matrice de 4 groupes, cible 10-15 min) ; RG-07/08/10 facultatifs. 3 essais documentés au journal avec les runs.
- Plan court : paths-ignore + concurrency conditionnelle dans `build.yml` ; matrice par groupe de scènes ; publication des captures dans la même Release.

### 2026-11-03 21:16:00 — Début WFS-21 Mode duel en ligne (Claude, agent 4) — attribué par le PO
- Ticket : WFS-21 · Épic E3 (Gameplay et simulation) · P2 Haute · branche `feature/waforspeed-21` · règles `regles-de-gestion/WFS-21.md`.
- Choix du PO : WFS-69 (E7, P1) demandé en premier, mais déjà attribué à l'agent 3 (commit 44c41f3) ; ticket « Prêt » suivant par priorité puis numéro : WFS-21 (E3 libre, WFS-12 ✅).
- Zone de code autorisée : `src/game/` sauf `game.*`, `tests/**`. Noyau (`game.*`, renderer, CMake) : ligne d'accroche ≤ 10 lignes annoncée au Journal, sinon ticket E0. HUD/menus du salon : uniquement ce qui est propre au duel.
- Décision du PO sur RG-07 (transport) : interface de transport abstraite + transport local en mémoire (deux instances dans le même processus, et un adversaire simulé qui rejoue des messages de présence) pour le banc et les tests ; le transport réseau réel (sockets, permission INTERNET dans `android/**` = E7) n'est pas exigé ici et fera l'objet d'un ticket E7 si l'utilisateur le veut sur téléphone.
- Réutiliser : `js/net/duel.js` (`startDuelRace`, `sendRacePresence`, `updateGhosts`, `finishDuel`, `renderLobby`), `js/game/update.js` → `hitGhosts`, `js/ui/hud.js` → `renderBoard`, modèles `models/<voiture>/`.
- Plan : RG-02/03 (graine commune, messages de présence) → RG-04 fantômes bornés → RG-05 contacts → RG-06 classement + résultat → RG-01 salon minimal.
- Critères : test CPU même graine = trafic identique image par image ; test messages hostiles sans plantage ; capture voiture adverse avec son nom + classement.

### 2026-11-03 21:19:39 — WFS-69 : CI découpée, PR #37 (agent 3, session 011cpqZd)
- **Constat** (run de WFS-59) : appel principal de `wfs_bench` ≈ 5 min ; les lancements `texte` ×2 et `chargement` 2400 refaisaient tout le banc pour 1 à 3 captures : ≈ 21 min sur 29.
- **Fait** : `build.yml` (paths-ignore de la doc ; plus de déclencheur `pull_request` ; concurrency par commit et sans annulation sur `main` ; banc en matrice `partie1` / `partie2` / `texte` / `ecrans-2400` ; publication en série qui ne remplace jamais une Release `apercu` plus récente) ; `desktop_main.cpp` (`WFS_BANC=partie1|partie2|chargement|texte`, rien ne change sans la variable).
- **Essai 1 (commit de code sur branche + PR)** : un seul run, [n° 435](https://github.com/wlfaleh/waforspeed_mobile/actions/runs/41720665103), aucun run créé par l'ouverture de la PR #37.
- **Essai 2 (documentation seule)** : ce commit (`TACHES.md` + `journal.md` sur `main`) ne doit lancer aucun build une fois WFS-69 fusionné ; avant fusion, `main` garde l'ancien `build.yml`.

### 2026-11-03 21:24:50 — WFS-21 en revue, PR #38 (Claude, agent 4)
- **Fait** :
  - `src/game/duel.*` : port de `js/net/duel.js` (graine `hashStr`, présence toutes les 0,1 s, fantômes estimés, lissés et bornés, résultat), de `hitGhosts` et de `renderBoard`. Messages texte nettoyés. Interface `Transport` et transport en mémoire (RG-07, décision du PO).
  - `src/game/duel_system.*` : départ commun avec la graine du trafic, horloge commune, contacts, voiture adverse avec son nom, classement « POSITION n/N », résultat « Revanche » / « Menu ».
  - Mode duel de `Course` et `trafficRestartSeed`.
  - Banc : `duel-course` et `duel-resultat`, avec un adversaire simulé.
- **Accroches annoncées** : `CMakeLists.txt` (2 lignes, `test_duel`), `.github/workflows/build.yml` (`&& ./build/test_duel` sur la ligne des tests ; WFS-69 de l'agent 3 touche aussi ce fichier, garder les deux), `src/platform/desktop_main.cpp` (2 lignes).
- **Vérification** :
  - `test_duel` OK : graine = jeu web ; trafic identique image par image sur 1800 images, en mode exact et en mode parallèle ; 6 messages hostiles sans plantage ; contacts et classement corrects.
  - `test_course` et `test_systems` OK.
  - Branche synchronisée avec `main` avant le push.
  - Shaders, banc et APK : CI de la PR #38.
- **Limites** : pas de réseau réel (ticket E7 à créer par le PO), pas d'entrée « Duel » dans le menu (E8), HUD solo affiché pendant le duel.

### 2026-11-03 21:28:24 — Fin WFS-69 : fusionné (PR #37), CI de 29-52 min à 7 min (agent 3, session 011cpqZd)
- **Run 435** (commit de code sur branche + PR) : un seul build ; android 1 min 32 ; banc `texte` + tests 1 min 44 ; `partie2` 4 min 05 ; `ecrans-2400` 5 min 24 ; `partie1` 6 min (chemin le plus long) ; publication 52 s ; **total 7 min** (21:18:54 → 21:25:52) au lieu de 29 à 52 min. Release `apercu-waforspeed-69` : 70 fichiers.
- **Captures** : 59 captures comparées à une référence locale (ancien banc de `main`, lavapipe dans le conteneur) : écart max 1 niveau, 0 pixel > 16 (arrondi entre versions de lavapipe), donc identiques. Les 10 captures des nouveaux lancements `texte` / `chargement` regardées : correctes ; comparaison au pixel à venir (référence locale en cours).
- **Fusion** : `4f95695`. Ajouté à `qa/Check.md`.
- **Essai 2 (doc seule)** : ce commit (`TACHES.md`, `qa/Check.md`, `journal.md`) ne doit lancer aucun build. **Essai 3** : deux commits de code de suite sur `main` (la fusion `4f95695` puis le prochain ticket fusionné) doivent publier tous les deux. Résultats ajoutés ici dès qu'ils sont connus.
- Constat au passage : la Release `apercu` de `main` n'avait plus qu'un fichier (builds de `main` annulés les uns par les autres) ; la fusion de WFS-69 la republie.

### 2026-11-03 21:29:57 — Début WFS-11 : garage complet, attribué à agent 3 (PO)
- **Choix** : WFS-45 fusionné, toutes les dépendances de WFS-11 sont finies : ⏳ En attente → 🔧 En cours. C'est le ⏳ Prêt P2 Haute de plus petit numéro dont l'épic (E5) est libre. E3 est occupée par WFS-21 (PR #38), donc pas WFS-66. WFS-41 (E8) et WFS-53 (E2) restent attribuables en parallèle.
- **Ticket** : WFS-11, épic E5, branche `feature/waforspeed-11`. Règles : `regles-de-gestion/WFS-11.md` + WFS-40 (fiche, peintures, « Choisir »), WFS-13 (20 voitures, 16 peintures, `carPerf`), WFS-46 (compteur en arc).
- **Zone autorisée** : `src/ui/**` sauf `src/ui/ecrans/` (écran garage dans `src/ui/garage.*`), `shaders/ui*`, `assets/cars/**`, `assets/licences/**`, `tools/convert_car.mjs`.
- **Accroches** (≤ 10 lignes chacune, à annoncer au journal) : `src/ui/ecrans/menu.cpp` (E8 : « Course solo » → garage ; WFS-41 peut toucher ce fichier en parallèle, garder les deux modifications), `game.*` / `CMakeLists.txt` (brancher l'écran Garage), `desktop_main.cpp` (captures dans un groupe `WFS_BANC` existant, `partie1` ou `partie2`, depuis WFS-69).
- **Plan (version STABLE demain : du fonctionnel d'abord)** : (1) scène garage + changement de voiture sur les 4 voitures actuelles ; (2) fiche, 16 peintures, « Choisir » → course avec voiture et peinture ; (3) compteur en arc ; (4) conversion des 16 autres voitures (`-lod` pour le trafic), taille de l'APK au journal. Si le temps manque, fusionner (1) à (3) avec les voitures déjà converties et le noter : le garage marche, les voitures suivent.
- **Critères** : captures `garage-*` (plateau, néon, projecteurs), `garage-golf` / `garage-m4`, `garage-fiche`, `garage-peinture-0/1`, course avec la voiture choisie, `hud-arc-120` / `hud-arc-320`, `vitrine-<id>` des voitures ajoutées ; 1280x720 et 2400x1080.

### 2026-11-03 — WFS-63 en revue, PR #39 (Claude, session trafic)
- Skinning GPU : `SkinVertex` / `SkinnedMesh` / `drawSkinned` dans le renderer, shaders `scene_skin.vert` / `shadow_skin.vert` (même `scene.frag`), palette d'os en set 2 (offset dynamique). Maillages rigides inchangés.
- `tools/convert_people.mjs` → `assets/people/homme.wfp`, `femme.wfp` (modèles et animations du jeu web, CC0). Lecture d'animation dans `src/render/squelette.*`.
- Banc : `squelette-essai`, `squelette-marche-0` / `-1`, `squelette-8` (8 instances : environ 0,03 ms CPU). Les 60 captures existantes sont identiques à main.
- **Accroches annoncées** : `CMakeLists.txt` (2 shaders ajoutés à la liste) ; `src/platform/desktop_main.cpp` (include, `WFS_BANC=squelette`, appel dans `partie2`).
- **Incident (réparé)** : mon dossier de travail avait été basculé sur `main` local par un autre agent. Les 2 commits WFS-63 y ont été déplacés vers `feature/waforspeed-63`, sans aucun envoi sur `main` ; `main` local est remis sur `origin/main`. **Agents : travaillez dans un worktree ou un clone séparé, pas dans `/home/user/waforspeed_mobile`.**
- Les PR #32 (WFS-58) et #34 (WFS-39) ont été fusionnées par l'utilisateur (QA de nuit notée par le PO). Le dernier essai simulateur de WFS-58 (cc78b4d, correction de WFS58-5) est à joindre à cette QA.

### 2026-11-03 21:34:56 — Début WFS-41 : menus complets (terrain, moment, réglages), attribué à agent 5 (PO)
- **Choix** : agent 5 libre (WFS-48 fusionné, PR #33). Épics occupées : E5 (WFS-11), E3 (WFS-21, PR #38), E0 (WFS-63, PR #39). Parmi les ⏳ Prêt d'épics libres, P2 Haute : WFS-41 (E8) et WFS-53 (E2) ; plus petit numéro → WFS-41. WFS-53 (E2) reste attribuable en parallèle.
- **Ticket** : WFS-41 (absorbe WFS-68), épic E8, branche `feature/waforspeed-41`. Règles : `regles-de-gestion/WFS-41.md` + `regles-de-gestion/WFS-68.md`.
- **Zone autorisée** : `src/ui/ecrans/**` (nouveaux écrans `terrain.*`, `reglages.*`). **Accroches** (≤ 10 lignes chacune, annoncées au journal) : `src/game/game.*` / `CMakeLists.txt` (brancher les écrans, passer `mapId` et le moment à la course, appliquer les réglages), `src/platform/desktop_main.cpp` (captures dans un groupe `WFS_BANC` existant). `src/ui/ecrans/menu.cpp` est aussi touché par WFS-11 (agent 3 : « Course solo » → garage) : garder les deux modifications. Ne pas toucher `src/world/**` (E2) ni `src/render/**` (E4) : la qualité s'applique par l'API existante du renderer, sinon le noter comme limite.
- **Plan** : (1) écran terrain : carrousel 4 cartes de `maps.js`, Jour / Nuit, « Rouler » → course sur la carte choisie, « Retour » ; (2) écran réglages (caméra, qualité, son, commandes, vibrations) depuis le menu et la pause, mémorisé ; (3) captures, CI, fusion rapide, `qa/Check.md`.
- **Critères** : `terrain-0` (ville, Jour / Nuit), `terrain-1` (circuit), `terrain-rouler` (journal : carte = circuit), `reglages-menu`, `reglages-pause`, un choix de qualité change le rendu, banc `0 écart(s)` sur les captures existantes.
- **Rappel** : travailler dans un worktree ou un clone séparé (incident du journal WFS-63), pas dans `/home/user/waforspeed_mobile`.

### 2026-11-03 21:37:53 — Pipeline de main verte (4f95695) ; contrôle de spots-sol (WFS-45) (Claude, agent 4)
- **Pipeline** : `build.yml` sur `main` 4f95695 (5 PR fusionnées + WFS-69) **verte**, en 8 min 31 s (run 41722024755). Release `apercu` à jour.
- **WFS-45, `spots-sol`** :
  - Ce qui marche : projecteurs orange et cyan visibles sur les bords de la Golf, plateau bleuté par l'hémisphère, brouillard linéaire.
  - Ce qui manque : **pas de tache de lumière visible sur le grand sol**, alors que le critère d'acceptation en demande trois. Le sol a un métal de 0,7 et une couleur 0x101219, donc son diffus est quasi nul, et le reflet des spots tombe hors du cône pour cette caméra.
  - La voiture est vue de l'arrière, alors que le jeu web la montre de 3/4 avant : il faudrait un cap de 0,6 + π dans le repère natif.
  - Noté dans `qa/Check.md` pour la recette de nuit ; à reprendre avec WFS-11 (garage, agent 3), qui utilise ces projecteurs.

### 2026-11-03 21:38:55 — PO : écarts avec le jeu web transformés en tickets (po-scrum-master)
- **WFS-70** (E3, P2 Haute) ressenti de conduite : modes de direction, secousse / ralenti / vibration au choc, réduire les mouvements. **WFS-71** (E2, P3) ville vivante : commerces et terrasses, portique 3D, enseignes. Écran de fin (record, stats) et pause « Recommencer » ajoutés à **WFS-41**. Aucun ticket en cours ou terminé touché. Message agent 4 → agent 3 (projecteurs du garage) : pas pour le PO, aucune action.

### 2026-11-03 21:40:54 — PO : WFS-50 remis en Prêt, WFS-64 suspendu et revu en diagnostic (po-scrum-master)
- **Fait apporté par Claude (session trafic)** : le run `simulateur.yml` 41719456821 (`scenario=mvp`, `feature/waforspeed-58` au commit cc78b4d, c'est-à-dire le code de la PR #32 avec WFS-59, sans WFS-64 ; Release `simulateur-WFS-58`, 21:22) passe tout le parcours MVP. Le résultat est « RÉSULTAT TECHNIQUE : OK ». Étapes vues : chargement, menu, Course solo, décompte, GO, fin (124 s), Rejouer, 2e fin (140 s), Menu. Score de fin : 2 260. Il y a une seule marque FIGÉE, sur `03-appui-course-solo`, identique à `02-menu`. Elle est **attendue** : la capture est prise juste après l'appui (WFS58-3). Le gel après le chargement (run 41716390682, et un essai `conduite` sans WFS-59) ne s'est pas reproduit. Il paraît donc intermittent (émulateur ou focus), et WFS-59 n'en est pas la cause.
- **Décisions** :
  - **WFS-50** passe en ⏳ Prêt, P4 Basse. La priorité reste basse parce que l'utilisateur teste l'APK sur téléphone, mais E7 est libre et le coût est d'environ 1 h. WFS-64 sort de ses dépendances. Il faut le relancer sur `main` avec `epic=MVP` et `scenario=mvp`, puis le donner à `qa-lead`. Règles mises à jour dans `regles-de-gestion/WFS-50.md`. La capture 03 figée n'est pas une anomalie. Un gel qui réapparaît est noté au rapport et réactive WFS-64.
  - **WFS-64** est renommé « Diagnostic : gel intermittent juste après le chargement sur le simulateur » et passe en ⏸ Suspendu (P4). Le repli du rendu à 50% est abandonné, car on ne corrige pas à l'aveugle. Le ticket est réactivé seulement si un essai sur `main` se fige de nouveau. Règles réécrites dans `regles-de-gestion/WFS-64.md` : diagnostic par 2 lignes de logcat et l'état du focus.
  - **WFS-59** : la mention « fige le simulateur » est retirée du tableau.
  - **QA de nuit de WFS-58 et WFS-39** : elles peuvent s'appuyer sur la Release `simulateur-WFS-58` du run 41719456821.
- Rien n'est attribué à Claude (session trafic), qui est sur WFS-63 (PR #39, en QA).

### 2026-11-03 21:42:36 — QA WFS-63 : VALIDÉ (qa-lead)
- PR #39, commit `0eb5465`, run 41722597629 vert, Release `apercu-waforspeed-63`. `squelette-essai` (Idle_Loop), `squelette-marche-0/1` (deux poses de Walk_Loop différentes) et `squelette-8` sont corrects : proportions justes, éclairage et ombre comme les voitures. Coût RG-05 au journal : 8 instances, 0,030 ms CPU par image.
- Non-régression contre `apercu-waforspeed-69` (main) : 63 captures identiques ; 6 écrans d'interface avec un écart de 1/255 au plus (bruit du rendu, invisible). Vêtements non colorés acceptés (apparence prévue dans WFS-19). **Fusion autorisée.** Rapport : `qa/rapports/WFS-63.md`.

### 2026-11-03 — WFS-63 terminé (Claude, session trafic)
- QA : VALIDÉ au 1er passage, sans anomalie. PR #39 fusionnée (fc70591). Le point d'extension « personnages animés par squelette » est sur main : `drawSkinned`, `src/render/squelette.h`, `assets/people/*.wfp`, documenté dans le skill `waforspeed-natif`.
- **Pour le PO** : WFS-19 (piétons et conducteurs) n'attend plus WFS-63.

### 2026-11-03 21:48:57 — Début WFS-19 Piétons animés et conducteurs, attribué à Claude (session trafic) (po-scrum-master)
- **Ticket** : WFS-19, épic E2 (P2 Haute), branche `feature/waforspeed-19`. WFS-63 ✅ (PR #39, VALIDÉ) : dépendances toutes terminées. À priorité égale avec WFS-53 (P2, E2), le plus petit numéro l'emporte ; E3 occupée (WFS-21 en revue), donc WFS-66 et WFS-70 non attribuables.
- **Zone de code autorisée** : `src/world/**`, `assets/world/**`, `shaders/world_*` (et `assets/people/**` en lecture). Le renderer et `drawSkinned` ne changent pas (sinon ticket E0 ou ligne d'accroche ≤ 10 lignes annoncée ici).
- **Critères** : capture de ville avec piétons aux poses variées ; capture plage (baigneurs, promeneurs) ; conducteur visible face au volant ; deux captures à des instants différents avec des poses de marche différentes ; banc `0 écart(s)` hors scènes concernées.
- **Plan court** : porter `populatePeople` / `pedLook` / `updatePeopleReal` (people.js, life.js) en un système E2 qui place et habille les personnes près du joueur, puis `bakeBust` / `measureSeat` / `addDriver` (drivers.js) pour le conducteur, rendus par `drawSkinned`. Règles : `regles-de-gestion/WFS-19.md`.

### 2026-11-03 21:52:23 — Arbitrage WFS-19 : création de WFS-72 (E0), attribué à Claude (session trafic) (po-scrum-master)
- **Constat** (remonté par Claude, session trafic) : RG-04 de WFS-19 (apparence du jeu web) repose sur le shader `pedMaterial` (`js/world/people.js` l. 54-110) : attribut par sommet `_cloth` et couleurs / seuils par personne. `drawSkinned` (WFS-63) n'a ni l'un ni l'autre ; il faut toucher au renderer, au format `.wfp` et ajouter `scene_skin.frag` : hors zone E2 et bien plus que 10 lignes d'accroche.
- **Décision** : on crée un ticket E0 « point d'extension » préalable, **WFS-72 — Point d'extension : apparence des personnages** (P2 Haute, environ 2 h). Le portage de `pedMaterial` se fait tel quel, sans finitions. L'apparence réduite (peau seule) est **refusée** : des personnages torse nu ne tiennent pas RG-04 et feraient refaire une seconde passe sur les mêmes fichiers.
- **Début WFS-72** : épic E0, attribué à Claude (session trafic), branche `feature/waforspeed-72`. E0 et E4 n'ont aucun ticket actif.
  - **Zone de code autorisée** : `src/render/renderer.h`, la partie skinning de `renderer.cpp`, `src/render/squelette.*`, `bench_squelette.*`, `shaders/scene_skin.*` / `shadow_skin.*`, `tools/convert_people.mjs`, `assets/people/**`, `CMakeLists.txt` et le skill `waforspeed-natif`.
  - **Critères** : nouvelle capture `squelette-tenue` (homme et femme habillés, parties distinctes) ; captures existantes identiques, banc `0 écart(s)` ; doc du point d'extension.
  - **Plan court** : `_cloth` → convertisseur, `SkinVertex` et `.wfp` ; bloc d'apparence par instance dans `drawSkinned` ; `scene_skin.frag` porte `pedMaterial`.
  - **Règles** : `regles-de-gestion/WFS-72.md`.
- **WFS-19** passe en ⏸ En attente de WFS-72. Sa réservation est conservée à Claude (session trafic), et E2 reste tenue par WFS-19 : l'attente est courte et WFS-19 (P2) passe avant WFS-71 (P3). Le ticket reprend sur `feature/waforspeed-19` dès que WFS-72 est fusionné. `pedLook` (tirage des tenues) reste dans WFS-19.

### 2026-11-03 22:57:42 — WFS-21 fusionné sur ordre de l'utilisateur, CI release lancée sur main (Claude, session trafic)
- **Conflit résolu** en synchronisant `feature/waforspeed-21` avec main : `src/platform/desktop_main.cpp`, commentaire de la liste du groupe `partie2` (« duel » de WFS-21 et « personnages » de WFS-63, les deux gardés), commit 5e3a72e. Compilation et tous les tests en local OK, `test_duel` compris.
- CI de la branche verte (android, banc-essai partie1 / partie2 / texte / ecrans-2400, publication). Contrôle rapide de `duel-course` et `duel-resultat` : rien de bloquant. **PR #38 fusionnée** (41e7936). Ajout dans `qa/Check.md` pour la recette de nuit, avec un point à vérifier : sur `duel-resultat`, le panneau latéral dit « POSITION 1/2 » alors que le résultat dit « 2E PLACE ».
- CI release (`build.yml`, `workflow_dispatch`) lancée sur main : APK et captures dans la Release `apercu`.
- Remarque : la liste de `qa/Check.md` est en double (même tableau deux fois). WFS-21 est ajouté au premier tableau ; le doublon reste à nettoyer par son auteur ou par le PO.

### 2026-11-03 23:11:47 — WFS-72 en revue, PR #40 (Claude, session trafic)
- `_cloth` et repères du corps dans le `.wfp` (version 2, fichiers régénérés) ; `ApparencePersonnage` passée à `drawSkinned` ; `shaders/scene_skin.frag` = copie de `scene.frag` + `pedMaterial` du jeu web tel quel. Sans apparence : rendu de WFS-63, les captures `squelette-*` sont identiques au pixel.
- Nouvelle capture `squelette-tenue` (homme et femme habillés). La fusion suivra la CI verte et le contrôle des captures (règle en vigueur), avec un ajout dans `qa/Check.md`.
- Point d'attention : `scene_skin.frag` est une copie de `scene.frag`. Toute modification future de `scene.frag` doit y être reportée (commentaire en tête du fichier).

### 2026-11-03 23:20:16 — WFS-72 fusionné (Claude, session trafic)
- CI verte (android, 4 jobs du banc, publication) ; contrôle rapide de `squelette-tenue` (identique au banc local) : rien de bloquant. PR #40 fusionnée (4314bf7) ; ajouté dans `qa/Check.md` pour la recette de nuit.
- WFS-19 (piétons et conducteurs) n'attend plus WFS-72 : je le reprends (`feature/waforspeed-19`).

### 2026-11-03 23:21:10 — WFS-66 attribué à Claude (session trafic), en parallèle de WFS-19 (PO)
- **Ticket** : WFS-66 — Caméras et vue cockpit (3 vues, volant, champ de vision selon la vitesse), épic **E3**, priorité P2 Haute, aucune dépendance. Branche `feature/waforspeed-66`. Règles : `regles-de-gestion/WFS-66.md`.
- **Exception à « un ticket par agent »**, à la demande de l'utilisateur : Claude (session trafic) garde WFS-19 (E2) et prend WFS-66 dans une autre épic libre. E3 n'avait aucun ticket actif ; WFS-66 est le ticket prêt de meilleure priorité hors E2 (P2, plus petit numéro devant WFS-70).
- **Zone de code autorisée** : `src/game/` sauf `game.*` (nouveau fichier de caméra, par ex. `src/game/camera_*.{h,cpp}`, et cockpit dessiné par code), `tests/**`. Accroche ≤ 10 lignes dans `src/game/game.*` autorisée si annoncée au Journal (RG-05). Rien dans `src/render/**`, `src/ui/**` ni `src/world/**` (zones de E4, E5, E8, E2) ; le bouton de changement de vue en course passe par l'accroche, le réglage dans le menu reste à WFS-41.
- **Critères** : captures `camera-poursuite`, `camera-capot`, `camera-cockpit` (1280x720 et 2400x1080) ; volant braqué visible en virage ; banc `0 écart(s)`.
- **À réutiliser** : `waforspeed/js/world/scene.js` du jeu web (`CAM_MODES`, cockpit ≈ lignes 94-123, suivi et champ de vision ≈ ligne 168) ; valeurs : cockpit 72°, volant `-braquage × 2,3` lissé, aiguille 0-300 km/h sur 2,2 rad.
- **Plan court** : porter `CAM_MODES` et le suivi lissé dans un fichier caméra E3 ; cockpit (tableau de bord, montants, volant, aiguille) en maillages générés ; accroche de sélection de vue dans `game.*` ; scènes de banc des 3 vues.

### 2026-11-03 23:23:49 — Découpage de WFS-19 : piétons / conducteurs (PO)
- **Décision** : proposition de Claude (session trafic) acceptée. WFS-19 (E2) = **piétons seulement**, cartes ville et plage (`populatePeople` : trottoirs par zone, promenade, côté de parc tiré à la graine faute de `parkSide`). Règles mises à jour (`regles-de-gestion/WFS-19.md`, RG-05 retirée).
- **Nouveau ticket WFS-73 — Conducteurs dans les voitures**, épic **E0** (mesure du siège dans `model_loader` / `Model`, accroches joueur, trafic, duel ≤ 10 lignes chacune), P3 Moyenne, ⏳ En attente de WFS-19. Règles : `regles-de-gestion/WFS-73.md` (réutiliser `drivers.js` : `measureSeat`, `bakeBust`, `addDriver`).
- **Poses assis, allongés, baigneurs** : pas de nouveau ticket, rattachées à **WFS-71** (terrasses : RG-02 existante ; serviettes de plage et baigneurs : nouvelle RG-06), même zone E2.
- Pourquoi : la mesure du siège et les accroches sortent de la zone E2 ; les mettre à part garde WFS-19 livrable vite et visible.

### 2026-11-04 00:02:53 — WFS-19 fusionné : piétons animés (Claude, session trafic)
- Système `Pietons` (`src/world/pietons.*`) : `populatePeople` et `pedLook` du jeu web, 8 piétons affichés au plus (16 dessins à squelette par image), LOD 16 / 50 m. Rendu par `drawSkinned` et `ApparencePersonnage` (WFS-63 / WFS-72), sans toucher au renderer.
- **Accroche annoncée** : `src/platform/desktop_main.cpp` (include, `WFS_BANC=pietons`, `benchPietons` dans `partie2`, 3 lignes).
- CI verte ; captures `pietons-*` de la CI identiques au banc local. PR #42 fusionnée (710c9fc) ; ajouté dans `qa/Check.md`. Toutes les captures de ville changent (piétons visibles).
- Pour la suite : WFS-73 (conducteurs) n'attend plus WFS-19. Limite constatée : 16 instances à squelette par image (`SKIN_MAX_INSTANCES`), à relever par un ticket E0 si conducteurs et piétons doivent être affichés ensemble en plus grand nombre.

### 2026-11-04 00:05:25 — WFS-53 et WFS-09 attribués à Claude (session trafic), deux sujets en parallèle (PO)
- **WFS-53** — Cartes Campagne et Grand Prix + finitions des arbres, épic **E2**, P2 Haute (ticket prêt le plus prioritaire). Dépendances WFS-07, WFS-14, WFS-34 : toutes ✅. Branche `feature/waforspeed-53`. Règles : `regles-de-gestion/WFS-53.md` + `WFS-54.md` + `WFS-43.md` (périmètre élargi). Zone : `src/world/**`, `assets/world/**`, `shaders/world_*`.
  - Critères : captures `carte-campagne-jour` / `-nuit` et Grand Prix (collines, champs, haies, fermes, éoliennes animées ; circuit) ; finitions des arbres de WFS-43 ; ≤ 300 appels de dessin ; autres cartes inchangées.
  - À réutiliser : `js/world/decor.js` (`DECOR`, `ZONES`), `js/world/life.js` (`vegBroadleaf`, `vegPine`), `js/world/maps.js` (`envFor`) ; socle natif WFS-14 et `src/world/vegetation.*`.
- **WFS-09** — Nuit complète : phares, éclairage public, route mouillée et reflets, épic **E4**, P3 Moyenne (plus petit numéro parmi les P3 de E4, épic libre). Dépendance WFS-07 ✅. Branche `feature/waforspeed-09`. Règles : `regles-de-gestion/WFS-09.md` + `WFS-16.md`. Zone : `src/render/**` (sauf `ui_*`), `shaders/scene*`, `sky*`, `post*`, `shadow*` (`scene_skin.frag` à garder synchrone avec `scene.frag`).
  - Critères : nuit = cône de phares sur la chaussée, feux arrière / stop, halos des lampadaires, route mouillée avec reflets ; jour sans faisceau ; campagne de nuit sans lampadaire ; coût GPU au Journal.
  - À réutiliser : `js/cars/player.js` (`buildBeam`, `updateBeam`), `js/cars/models.js` (`BEAM`, feux par matériau), boucle des lampadaires de `js/world/scene.js`, `envFor`.
- **Pourquoi pas WFS-70 (E3, P2)** : E3 a encore WFS-66 actif (un ticket actif par épic). WFS-70 sera le prochain ticket de E3 dès la fusion de WFS-66.
- **Exception « un ticket par agent »** reconduite à la demande de l'utilisateur (deux sujets en parallèle, zones E2 et E4 séparées) ; WFS-66 doit être fusionné avant de commencer le second.

### 2026-11-04 00:15:49 — WFS-66 fusionné : caméras et vue cockpit (Claude, session trafic)
- `src/game/camera_vue.*` (système `camera`, port de `CAM_MODES`, `updateCamera` et `buildCockpit`) et `src/game/camera_banc.*` ; tests dans `tests/test_systems.cpp`.
- **Accroches annoncées** :
  - `src/game/game.cpp` : 3 lignes (include, `cameraVue`, voiture cachée en cockpit) ;
  - `src/platform/desktop_main.cpp` : 3 lignes (`WFS_BANC=camera`, `benchCameras` dans `partie2`) ;
  - `.github/workflows/build.yml` : 1 ligne, job `ecrans-2400`, captures `camera-*-2400`.
- Conflit de synchronisation avec WFS-19 dans `desktop_main.cpp` résolu, les deux scènes du banc sont gardées. CI verte (9506863), PR #41 fusionnée (3994283), ajouté à `qa/Check.md`.
- Suite : WFS-53 (E2, cartes Campagne et Grand Prix, finitions des arbres) et WFS-09 (E4, nuit complète) attribués par le PO. WFS-70 (E3) passe ensuite.

### 2026-11-04 00:17:41 — WFS-70 et WFS-50 attribués à Claude (session trafic), deux sujets en parallèle (PO)
- **WFS-70** — Ressenti de conduite : modes de direction, chocs (secousse, ralenti, vibration), réduire les mouvements, épic **E3**, P2 Haute (ticket prêt le plus prioritaire ; E3 libérée par la fusion de WFS-66). Aucune dépendance. Branche `feature/waforspeed-70`. Règles : `regles-de-gestion/WFS-70.md` (précisions RG-07 à RG-11 ajoutées).
  - Zone : `src/game/` sauf `game.*`, `tests/**` ; **exceptions accordées** : déplacement de la partie « volant » de `Game::onTouch` / `drawControls` vers `src/game/direction.cpp` (appels ≤ 10 lignes dans `game.cpp`) ; vibration Android dans un nouveau `src/platform/vibration_android.cpp` + permission `VIBRATE` + accroche ≤ 10 lignes dans `android_main.cpp`.
  - Critères : choc = secousse visible + ralenti (journal) ; deux modes de direction utilisables ; « réduire les mouvements » supprime secousse et ralenti ; chrono non allongé ; captures `ressenti-*` ; banc sans régression.
  - À réutiliser : `js/game/update.js` (`shake`, `flash`, `timeScale`, `slowmoT`, `navigator.vibrate`), `js/ui/screens.js` (`setSteer`, `setSteerRow`), `js/core/config.js` (`reduceMotion`) ; déjà portés en natif : `ev.shake` (`traffic.cpp`), `timeScale` / `slowmoT` (`course.cpp`).
- **WFS-50** — [MVP] Vérification simulateur du MVP, épic **E7**, P4 Basse : seul ticket prêt d'une épic libre (E2, E4, E5, E8 ont un ticket actif ; E0 et E6 n'ont rien de prêt). Dépendances toutes ✅. Branche `feature/waforspeed-50` (seulement si `tools/qa/**` doit changer ; sinon lancement de `simulateur.yml` sur `main`, `epic=MVP`, `scenario=mvp`). Règles : `regles-de-gestion/WFS-50.md`. Zone : `tools/qa/**`, `.github/workflows/simulateur.yml`. Rapport `qa/rapports/simulateur-MVP.md` par `qa-lead` (pas par le développeur) ; défauts → tickets de correction par le PO ; VALIDÉ → « Livraison MVP » au Journal.
- **Exception « un ticket par agent »** reconduite à la demande de l'utilisateur (agents distincts de la session trafic, zones séparées : E2 WFS-53, E4 WFS-09, E3 WFS-70, E7 WFS-50).

### 2026-11-04 00:20:10 — WFS-24 attribué à Claude (session trafic), 5e sujet en parallèle (PO)
- **WFS-24** — Sauvegarde et progression, épic **E3**, relevé de P4 Basse à **P3 Moyenne** : il débloque la mémorisation attendue par WFS-11 (voiture, peinture), WFS-41 (réglages) et WFS-70 (mode de direction). Branche `feature/waforspeed-24`, règles `regles-de-gestion/WFS-24.md` (précisions RG-07 à RG-11 ajoutées).
- **Pourquoi lui** : E0 et E6 n'ont rien de prêt ni d'utile à créer (WFS-61 attend WFS-60, WFS-64 suspendu ; audio : moteur et bruitages livrés, la musique est hors périmètre). WFS-60 (E5) toucherait `assets/cars` et `tools/convert_car.mjs` en même temps que WFS-11 ; WFS-15 / WFS-65 (E4) et WFS-71 (E2) recouvrent les fichiers des tickets actifs de leur épic. WFS-24 est un module neuf.
- **Exception « un ticket actif par épic »** (E3 : WFS-70 et WFS-24, même agent) : accordée car les fichiers sont disjoints. Zone WFS-24 : `src/game/sauvegarde.*` (neuf), `src/game/course_system.cpp` (record et « NOUVEAU RECORD » sur l'écran de fin), `tests/test_sauvegarde.cpp` (neuf) ; accroches ≤ 10 lignes : `src/game/game.cpp`, `src/platform/android_main.cpp`, `src/platform/desktop_main.cpp`, ≤ 3 lignes dans `src/ui/ecrans/menu.cpp`. Interdit : `src/game/direction.*`, `player.*` (WFS-70).
- Critères : test CPU écriture/relecture, fichier corrompu et identifiant inconnu → défauts ; banc `sauvegarde` lancé deux fois, le second retrouve voiture, peinture et record ; capture `fin-record` avec « NOUVEAU RECORD ».
- Plan : module `Sauvegarde` clé=valeur versionné, écriture atomique sur thread de travail → chargement au démarrage → record en fin de course → enregistrement en arrière-plan → test + scène de banc.

### 2026-11-04 00:33:03 — WFS-11 : garage et compteur en arc en revue (agent 3, session 011cpqZd)
- **Fait** (étapes 1 à 3 du PO) : `src/ui/garage.*` (scène du garage, fiche, 16 peintures, changement de voiture par geste / flèches / points, « Choisir » → course avec voiture et peinture, « Retour » → menu) ; compteur en arc dans `src/ui/hud.cpp` (captures `hud-arc-120`, `hud-arc-320`).
- **Accroches annoncées** : `src/core/system.h` (`GameView::choisirVoiture`, 2 lignes), `src/game/game.*` (choix appliqué entre deux images, 4 lignes), `src/ui/ecrans/menu.cpp` (« Course solo » → garage, 1 ligne ; banc tactile : appui sur « Choisir », 4 lignes), `desktop_main.cpp` et `build.yml` (banc du garage, copies 2400).
- **Vérification locale** (lavapipe) : captures du garage en 1280 et 2400 et compteur regardés ; `test_track`, `test_systems`, `test_audio` OK. Branche synchronisée avec `main` (conflits `desktop_main.cpp` et `build.yml` résolus en gardant caméras, piétons, duel et garage).
- **Limite** : étape 4 (16 autres voitures, ex-WFS-13) non faite : le garage montre les 4 voitures converties. À reprendre dans un ticket dédié.

### 2026-11-04 00:33:58 — Début WFS-65 Effets de conduite et d'accident (Claude, agent 4) — attribué par le PO
- **Ticket** : WFS-65, épic **E4**, P3 Moyenne. Branche `feature/waforspeed-65`. Règles : `regles-de-gestion/WFS-65.md`. Aucune dépendance.
- **Pourquoi ce ticket** : aucun ticket ⏳ Prêt dans une épic libre (E2 WFS-53, E3 WFS-70/WFS-24, E4 WFS-09, E5 WFS-11 en revue, E7 WFS-50, E8 WFS-41 ; E0 et E6 n'ont aucun ticket prêt). WFS-65 est le ticket prêt le plus visible dont le code se sépare proprement du ticket actif de son épic : **exception « un ticket actif par épic »**, comme WFS-24 en E3. WFS-15 (post-traitement) et WFS-60 (roues, `assets/cars` touché par WFS-11) écartés pour risque de conflit.
- **Zone de code autorisée (séparée de WFS-09)** : nouveaux fichiers `src/render/effets_*.{h,cpp}` (particules étincelles / fumée, couche d'écran lignes de vitesse / vignette / flash), `shaders/particules.*` et `shaders/effets_ecran.*`, banc `src/render/bench_effets.*`, tests `tests/test_effets.cpp`. **Interdit** : `scene.frag`, `scene_skin.frag`, `post.frag`, `sky*`, `shadow*` (WFS-09). Accroches ≤ 10 lignes dans `renderer.cpp` / `renderer.h` et pour lire chocs, nitro, accidents depuis le jeu (`src/game`), annoncées au Journal.
- **Critères** : captures `choc-etincelles`, `accident-fumee`, `nitro-lignes` avec effets visibles ; vignette avec la vitesse, flash au choc et au départ du nitro ; coupé / réduit en qualité basse, ≥ 30 im/s ; physique et chrono inchangés ; autres captures inchangées, banc `0 écart(s)`.
- **À réutiliser** : `waaps/waforspeed/js/world/particles.js` (`makeParticleSystem` : nombres, durées, couleurs des étincelles et de la fumée), `js/world/fx.js` (`drawFx` : lignes de vitesse, vignette, flash, seuils de vitesse et nitro). Ne rien inventer qui y existe.
- **Plan court** : 1) système de particules en lots (un appel de dessin, billboards additifs) ; 2) étincelles au choc et fumée des accidentés ; 3) couche d'écran (lignes, vignette, flash) ; 4) qualité basse ; 5) 3 scènes de banc ; une seule CI à la fin, fusion rapide puis `qa/Check.md`.
- **Coordination** : synchroniser souvent avec `main` (WFS-09 touche `renderer.cpp` en parallèle).

### 2026-11-04 00:34:10 — WFS-50 : cause de l'échec du simulateur MVP trouvée (Claude, session trafic)
- Passage 41745827872 (main b01ae36) : fenêtre du jeu 2400x954 (barre de navigation de l'émulateur affichée), écran 2400x1080.
  Le scénario appuyait le gaz à 0,92 × 1080 = 993 px, sous la fenêtre : 0 km/h toute la course ; l'avance automatique après
  « temps écoulé » faisait croire au GO. Le jeu n'est pas en cause (gaz vérifié au banc PC : 12 puis 57 km/h après GO).
- Correction dans `tools/qa/emulateur.sh` (branche `feature/waforspeed-50`, e7f4777) : points d'appui calculés sur
  « Cibles de rendu : WxH (logique) » ; étape go en échec si « temps écoulé » arrive avant une vitesse > 0. Simulateur relancé
  sur la branche (scénario mvp).

### 2026-11-04 00:36:06 — WFS-41 : terrain et réglages codés, PR #45 en revue (Claude, agent 5)
- **Fait** : écran Terrain (`src/ui/ecrans/terrain.*`) : carrousel des 4 cartes (vignette façon `mapArt`, nom, description, longueur `mapLengthKm`), « 01 / 04 », glisser ou flèches, Jour / Nuit (nuit par défaut), Retour, Rouler (partie reconstruite sur la carte). Écran Réglages (`src/ui/ecrans/reglages.*`, WFS-68) depuis le menu et la pause : caméra Poursuite / Capot / Cockpit (`choisirVue` de WFS-66), graphismes Basse / Moyenne / Haute (échelle de scène 50 / 75 / 100 %, ombres), son Activé / Coupé ; enregistrés dans un fichier simple relu au démarrage. Menu : « Course solo » → Terrain ; record gardé entre les cartes.
- **Accroches annoncées** : `src/game/game.*` (5 + 8 lignes) : `Game::demandes()` (carte et moment, `shutdown` + `init` au début de la mise à jour suivante) ; `src/audio/audio_system.cpp` (3 lignes, accord du PO) : sortie audio à zéro si son coupé ; `src/platform/desktop_main.cpp` (7 lignes) : bancs `benchTerrain` / `benchReglages` ; `.github/workflows/build.yml` (1 ligne) : captures `-2400` du groupe menu.
- **Pour WFS-11 (agent 3)** : `menu.cpp` envoie « Course solo » vers `Ecran::Terrain` ; quand le garage arrive, garder Menu → Garage et faire Garage (« Valider ») → `Ecran::Terrain` (ordre du web : étape 1 garage, étape 2 terrain).
- **Limites** : vibrations masquées (décision PO, ticket Android à créer) ; disposition gaucher des commandes non faite (ticket E0 nécessaire) ; pas d'écran de chargement pendant la reconstruction de carte ; « Nuit » visible seulement sur la plage (ville : WFS-09) ; la course n'est pas figée derrière l'écran Terrain (seulement derrière le menu, règle E3).
- **Vérification** : compilation `-fsyntax-only -Wall` des 7 `.cpp` touchés sans erreur ; captures et APK : en attente de la CI.


### 2026-11-04 00:39:55 — WFS-65 en revue, PR #46 ; WFS-21 suite en PR #44 (Claude, agent 4)
- **WFS-65, fait** :
  - Port de `particles.js` (`src/render/effets_particules.*`, testé par `test_effets`).
  - Système d'effets en lecture seule (`src/render/effets_systeme.*`, ordre 350) : étincelles aux accidents, chocs et frottements ; fumée des pneus et des voitures accidentées (5 s) ; `drawFx` avec les lignes sous nitro, la vignette radiale et le flash rouge.
  - Qualité basse en GPU logiciel : moitié moins de particules, pas de lignes.
  - Banc : `choc-etincelles`, `accident-fumee`, `nitro-lignes`. Effets coupés dans les autres scènes du banc.
- **Accroches annoncées** : `src/render/renderer.h` et `renderer.cpp` (`uiQuad`, 6 lignes, quadrilatère à couleurs par sommet) ; `src/platform/desktop_main.cpp` (2 lignes, groupe `partie1`) ; `CMakeLists.txt` et `.github/workflows/build.yml` (`test_effets`). Aucun fichier de WFS-09 touché (`scene.frag`, `post.frag`, `sky*`, `shadow*`).
- **Écarts au texte de RG-04, par fidélité à `fx.js`** : flash rouge et non blanc, lignes de vitesse seulement sous nitro, vignette fonction de l'ambiance et du nitro (pas de la vitesse).
- **WFS-21** : la PR #38 avait été fusionnée par un autre agent ; mon correctif suivant (classement trié par arrivée, pseudo du banc) est en PR #44, dont la CI tourne.

### 2026-11-04 00:42:02 — Règle : fusion directe par confiance (Claude, agent 4, à la demande de l'utilisateur)
- **Règle** : un développeur **confiant à plus de 50%** que son ticket fonctionne **fusionne directement**, sans attendre la fin de la CI ni les captures de sa branche.
- **Conditions minimales** :
  - branche synchronisée avec `main` ;
  - compilation et tests CPU passés en local ;
  - aucune modification hors zone (accroches de 10 lignes au plus, annoncées au Journal).
- **À la fusion** : ligne dans `qa/Check.md` marquée « fusion par confiance », statut ✅ dans `TACHES.md`, entrée au Journal.
- **Après la fusion** : le développeur suit la CI de `main` et corrige tout de suite si elle est rouge ou si une capture est cassée.
- **Confiance de 50% ou moins** : on attend la CI verte et on regarde les captures avant de fusionner.
- **Fichiers modifiés** : `CLAUDE.md`, `TACHES.md` (règle 5), skill `waforspeed-natif` (point 5 bis), agents `po-scrum-master` et `qa-lead`.

### 2026-11-04 00:42:35 — WFS-65 et suite de WFS-21 fusionnés par confiance (Claude, agent 4)
- **PR #46 (WFS-65)** : fusionnée par confiance (environ 70 %), alors que la CI de la branche n'était pas finie. Code compilé, `test_effets` OK, aucun shader modifié, branche synchronisée.
- **PR #44 (suite de WFS-21)** : fusionnée ; sa CI (1fe1ff1) a fini verte.
- **Suivi** : `TACHES.md` passe WFS-65 à ✅ Fusionné ; les deux PR sont ajoutées à `qa/Check.md`, marquées « fusion par confiance ».
- **Suite** : je surveille la CI de `main`. Si les captures `choc-etincelles`, `accident-fumee` ou `nitro-lignes` sont cassées, je corrige tout de suite.

### 2026-11-04 00:44:22 — Début WFS-60 Roues animées (roues séparées, rotation, braquage) (Claude, agent 4) — attribué par le PO
- **Ticket** : WFS-60, épic **E5** (+ API du rendu E0, épic libre), relevé de P4 à **P2 Haute**, **élargi** : absorbe WFS-61 (accroche de rendu par roue) et WFS-62 (animation) ; règles `regles-de-gestion/WFS-60.md` (section « Périmètre élargi »), `WFS-61.md`, `WFS-62.md`. Branche `feature/waforspeed-60`.
- **Pourquoi lui** : aucun ticket ⏳ Prêt dans une épic libre (E2 WFS-53, E3 WFS-70/WFS-24, E4 WFS-09, E5 WFS-11 en revue, E7 WFS-50, E8 WFS-41 en revue ; E0 et E6 sans ticket prêt). Vérification faite : la PR de WFS-11 **ne touche ni `assets/cars` ni `tools/convert_car.mjs`** (garage.*, hud.cpp, menu.cpp, accroches) : la raison qui écartait WFS-60 tombe. Roues qui tournent = défaut le plus visible restant pour la version stable ; débloque WFS-67. Regroupement 60+61+62 : un seul sujet visible, une seule CI (règle des gros tickets).
- **Exception « un ticket actif par épic »** (E5 : WFS-11 en revue et WFS-60) : fichiers disjoints, comme WFS-24 en E3 et WFS-65 en E4.
- **Zone de code autorisée** : `tools/convert_car.mjs`, `assets/cars/*.wfm` (4 voitures), `src/render/model_loader.*`, `src/render/renderer.h` (API) et l'appel de dessin d'une voiture dans `renderer.cpp` (accroche minimale, WFS-09 y travaille), nouveaux `src/game/roues.*`, `tests/test_roues.cpp`, accroches ≤ 10 lignes (`game.cpp`, trafic, `desktop_main.cpp`, `build.yml`) annoncées au Journal. **Interdit** : `player.*`/`direction.*` (WFS-70, sauf ≤ 3 lignes de lecture de `wheelAngle`), `sauvegarde.*`, `garage.*`, `hud.cpp`, `scene*.frag`, `post*`, `sky*`, `shadow*`, `effets_*`.
- **Critères** : `roues-arret-1/2` identiques, `roues-roule` jante tournée, `roues-virage` avant braquées ; `test_roues` (4 roues, rayon 0,25-0,45 m, symétrie, roulage = distance/rayon, braquage ×1,6 ≤ 0,6 rad, Ackermann 1,12/0,9) ; autres captures inchangées, `0 écart(s)`.
- **À réutiliser** : `waaps/waforspeed/js/cars/models.js` (`splitWheels`, `looksLikeWheel`, `WHEEL_RX`, `animWheels`), `js/world/scene.js` (≈ l. 208, 238), modèles `models/<voiture>/`.
- **Plan court** : 1) conversion roues séparées + chargeur rétro-compatible ; 2) API `draw` avec pose des roues (défaut = rendu actuel) ; 3) `src/game/roues.*` (roulage, Ackermann) branché joueur + trafic proche ; 4) test + scènes `roues-*` ; une CI, **fusion directe par confiance** (> 50%, règle de l'utilisateur) puis `qa/Check.md`.

### 2026-11-04 00:44:41 — Règle « confiance > 50% » rendue visible (Claude, agent 4, à la demande de l'utilisateur)
- « 50 % » est réécrit « 50% » partout : une recherche de « 50% » ne trouvait pas la règle.
- `CLAUDE.md` : la règle de fusion par confiance passe **en tête** des consignes, avec un titre « RÈGLE DE FUSION N°1 — CONFIANCE > 50% → FUSION DIRECTE ».

### 2026-11-04 00:45:01 — Fin WFS-11 : garage et compteur en arc fusionnés (PR #43) (agent 3, session 011cpqZd)
- CI verte (run de la PR : android, 4 bancs, publication) ; captures `garage-0-2400`, `menu-depart-go` (menu → garage → « Choisir » → GO) et `garage-course` regardées : rien de bloquant. Fusion `5dcda32`, ajouté à `qa/Check.md`.
- Le job `ecrans-2400` passe à 11 min (caméras WFS-66 + garage en 2400) : c'est maintenant le plus long de la CI.
- Reste à faire (ex-WFS-13) : conversion des 16 autres voitures (le garage montre les 4 voitures converties).

### 2026-11-04 00:44:51 — WFS-50 fusionné (PR #47) (Claude, session trafic)
- Run 41747781094 de la branche : `RÉSULTAT TECHNIQUE : OK`, 10 étapes. Course 1 : score 3240, 0,65 km, 143 km/h max ; Rejouer et Menu
  fonctionnent. Captures regardées (HUD, piétons, écran de fin) : rien de bloquant. CI de construction verte (run 41747775305).
- Fusion immédiate (règle en vigueur), ligne ajoutée dans `qa/Check.md`. Simulateur relancé sur `main` (`epic=MVP`, `scenario=mvp`) :
  la Release `simulateur-MVP` sera écrasée par ce passage, à recetter la nuit (`qa/rapports/simulateur-MVP.md`).

### 2026-11-04 00:46:48 — Fin WFS-41 : PR #45 fusionnée par confiance (Claude, agent 5)
- **Fusion** : PR #45 fusionnée sur `main` (`e300de2`). CI verte sur la branche (run 41747977801 : android, 4 bancs, publication), captures regardées : terrain, réglages et qualité basse conformes. Deux synchros avec `main` ensuite (WFS-11, WFS-65, WFS-50, WFS-21 suite) : compilation et `test_systems` passés en local, puis fusion par confiance (règle du 2026-11-04).
- **Synchro avec WFS-11** : le parcours suit le web : menu → garage (« Choisir ») → terrain → « Rouler » → course. Accroche d'une ligne dans `src/ui/garage.cpp` (E5) : après « Choisir », `Ecran::Terrain` au lieu de `Ecran::Course`. Au changement de carte, la voiture et la peinture du garage sont gardées.
- **Corrigé avant fusion** : les vibreurs du circuit dépassaient de la vignette (`terrain-1`).
- **Ajouté à `qa/Check.md`** ; je suis la CI de `main` après la fusion.


### 2026-11-04 00:47:17 — Début WFS-13 : les 16 autres voitures (agent 3, session 011cpqZd) — attribué par le PO
- **Ticket** : WFS-13 **rouvert** (était fusionné dans WFS-11 ; étape 4 de WFS-11 non faite), épic **E5**, **P2 Haute**, branche `feature/waforspeed-13`. Règles : `regles-de-gestion/WFS-13.md`, section « Réouverture du 2026-11-04 » (RG-10 à RG-17, prime sur l'ancien texte).
- **Pourquoi lui** : aucun ⏳ Prêt attribuable dans une épic libre à priorité utile. WFS-15 (E4) recouvre `post*` / `scene*` de WFS-09 ; WFS-71 (E2) recouvre `ville.cpp` / `plage.cpp` de WFS-53 ; WFS-22 (E7, P4, iOS) ne se voit pas sur le téléphone de l'utilisateur. Le garage montre 20 fiches mais 4 voitures : c'est le manque le plus visible, et il prépare la variété du trafic (WFS-67). Gros ticket d'un seul sujet, une CI.
- **Exception « un ticket actif par épic »** (E5 : WFS-60 et WFS-13) : fichiers disjoints. WFS-60 garde `tools/convert_car.mjs` et les 4 `.wfm` existants ; WFS-13 ajoute 16 `.wfm` neufs et un script neuf. Si WFS-60 fusionne avant : merge de `main` et relance du script (roues séparées pour les 16).
- **Zone de code autorisée** : `assets/cars/<16 nouveaux>.wfm`, nouveau `tools/convertir_voitures.sh`, `assets/licences/<id>.txt`, `src/ui/garage.*` (si besoin pour lister les voitures disponibles) ; accroches **données seulement** annoncées au Journal : 16 entrées de `CARS` dans `src/game/cars.h` (E3), 16 lignes de `VOICES` dans `src/audio/engine_synth.cpp` (E6) ; banc : `src/platform/desktop_main.cpp` et `.github/workflows/build.yml` (≤ 10 lignes, **hors job `ecrans-2400`**). **Interdit** : `tools/convert_car.mjs`, `model_loader.*`, `renderer.*`, `roues.*` (WFS-60), `player.*` / `direction.*` (WFS-70), `traffic.*` (WFS-67).
- **Critères** : `vitrine-<id>-0` × 16 (sens, 4 roues, peinture, feux arrière non blancs) ; 20 voitures au garage, `garage-<id>` × 2, course avec une nouvelle voiture ; tailles `.wfm` + APK au Journal ; autres captures inchangées.
- **À réutiliser** : `waaps/waforspeed/js/cars/catalog.js` (`CARS`, `CAR_MODELS` : longueur, `rotateY`, `paint`, `tail`), `js/audio/sfx.js` (`ENGINE_SET`), modèles `models/<voiture>/<voiture>.json` + `license.txt` ; `tools/convert_car.mjs` tel quel ; `carPerf` / `carStats` déjà portés.
- **Plan court** : 1) script de conversion des 16 ; 2) catalogue, sons, licences ; 3) captures vitrines (1 par voiture) ; 4) une CI, fusion par confiance si > 50%, `qa/Check.md`.
- **Ménage PO** : WFS-22 passe ⏳ En attente → ⏳ Prêt (WFS-29 ✅). Note de l'agent 3 : `ecrans-2400` dure 11 min ; pas de ticket dédié (ce serait un micro-ticket) : WFS-13 ne doit rien y ajouter, et le prochain ticket E7 qui touche la CI le scindera en deux jobs.

### 2026-11-04 00:48:56 — PO : limites de WFS-41 → WFS-74 et WFS-75 ; WFS-74 attribué à agent 5 (po-scrum-master)
- **Analyse des limites laissées par WFS-41** : (1) vibrations Android : **déjà couvertes par WFS-70** (RG-10, `vibration_android.cpp`, en cours) → pas de nouveau ticket pour le vibreur ; (2) disposition gaucher : touche `Game::onTouch` / `drawControls` dont WFS-70 déplace le volant vers `direction.cpp` → à faire **après** WFS-70 ; (3) écran de chargement au changement de carte et (4) course non figée derrière Terrain / Réglages : indépendants, faisables tout de suite.
- **Créés** (gros tickets, regroupés par moment possible) : **WFS-74** (E0, P2 Haute, ⏳ Prêt) changement de carte avec écran de chargement (progression réelle de `Game::init`, fondu, course neuve avec décompte) + course figée sur tout écran autre que `Course` ; **WFS-75** (E8, P3 Moyenne, ⏳ En attente de WFS-70) réglages « Commandes : droitier / gaucher » (miroir des commandes) et « Vibrations : oui / non ». Règles : `regles-de-gestion/WFS-74.md`, `regles-de-gestion/WFS-75.md`. Aucune anomalie 🆕 dans `anno.md`.
- **Début WFS-74** — attribué à **Claude (agent 5, session 011cpqZaDiS2u)**, épic E0 (libre ; seuls tickets Prêt restants WFS-15 / WFS-71 bloqués par WFS-09 / WFS-53 dans leurs épics). Branche `feature/waforspeed-74`.
  - Zone : `src/game/game.{h,cpp}` ; accroches ≤ 10 lignes annoncées au Journal : `src/game/course_system.cpp` (E3, WFS-24 et WFS-70 en cours : synchroniser avec `main`), `src/ui/ecrans/chargement.cpp` / `terrain.cpp`, `src/platform/android_main.cpp` / `desktop_main.cpp`. Ne pas toucher la partie « volant » de `onTouch` / `drawControls` (WFS-70).
  - Critères : `terrain-chargement`, `terrain-arrivee` (décompte, chrono à zéro, voiture gardée), `terrain-fige-1/2` (chrono figé au journal), pause → reprise sans remise à zéro, autres captures inchangées.
  - Plan court : point d'extension de progression dans `Game` branché par la plateforme → écran WFS-35 pendant `init` → course neuve (Rejouer) → `CourseSystem` figé hors `Course` → scènes de banc → fusion (règle en vigueur), `qa/Check.md`.

### 2026-11-04 00:51:56 — PO : plus de tickets simulateur (po-scrum-master)
- **Décision de l'utilisateur** : le simulateur est hors objectif. Règle ajoutée à `po-scrum-master.md` : plus de création ni d'attribution de ticket simulateur (« Vérification simulateur EX » supprimée du processus du PO). Tickets déjà fusionnés ou en cours non touchés.

### 2026-11-04 00:52:07 — WFS-13 : 16 voitures en revue (agent 3, session 011cpqZd)
- **Fait** : `tools/convertir_voitures.sh` (16 conversions, `convert_car.mjs` inchangé, peinture trouvée partout) ; 16 `.wfm` + licences ; 16 entrées `CARS` et 16 `VOICES` (Model 3 : son citadine en attendant l'électrique) ; fiche Mustang ajoutée au garage ; job CI `vitrines`.
- **Accroches annoncées** : `src/game/cars.h` (données + `TRAFIC_MODELES = 4`), `src/audio/engine_synth.cpp` (données), `src/game/traffic_system.cpp` (2 lignes : le trafic garde ses 4 modèles, sinon il chargerait les 20 ; variété : WFS-67), `desktop_main.cpp` + `build.yml` (groupe `vitrines`).
- **Tailles (RG-16)** : p208 6,70 Mo · abarth 4,98 · tcross 6,01 · compass 8,42 · s3 6,31 · x5 7,24 · giulia 6,54 · a45 6,43 · model3 6,45 · corsa 4,83 · gt86 5,36 · elantra 5,98 · v60 2,54 · rrsport 6,01 · mustang 5,85 · sf90 7,40 → 92 Mo ; `assets/cars` 22 → 114 Mo ; APK de `main` avant : 31,4 Mo (après : relevé à la CI).
- **Triangles** : compass 154 k, x5 138 k, model3 135 k, p208 132 k dépassent la cible de 120 k du joueur (versions `-lod` possibles plus tard).
- **À regarder** : Giulia et Elantra ont un motif de camouflage venant de la texture de peinture de leur modèle.

### 2026-11-04 00:52:19 — WFS-60 roues animées fusionné par confiance, PR #48 (Claude, agent 4)
- **Fait** :
  - `src/game/roues.*` porte `splitWheels`, `looksLikeWheel`, `rigWheels` et `animWheels` (`js/cars/models.js`) et les angles de `scene.js`.
  - Le chargeur redécoupe chaque matériau en une partie fixe et une partie par roue. Les 4 voitures ont 4 roues chacune, **sans reconversion des `.wfm`** (la Golf et la 911 n'ont aucun matériau marqué « roue »).
  - Les jantes de la 911, fusionnées avec le chrome, sont récupérées dans le cylindre de chaque roue ; les étriers sont exclus (règle des 10/12 du tour).
  - Rendu : `draw(model, world, paint, RouesPose*)`. Sans pose, rien ne change (garage, duel).
- **Accroches annoncées** :
  - `src/render/renderer.h` et `renderer.cpp` : API et matrice par roue, environ 8 lignes ;
  - `src/game/game.cpp` : 2 lignes ;
  - `src/game/traffic_system.cpp` : 2 lignes ;
  - `src/platform/desktop_main.cpp` : 2 lignes, groupe `partie2` ;
  - `CMakeLists.txt` et `build.yml` : `test_roues`.
  - Nouveau fichier `src/game/roues_banc.*` pour le banc. `player.*` n'a pas été touché (angle recalculé depuis `steer` et `speed`).
- **Vérification** :
  - `test_roues` OK sur les 4 `.wfm` : rayons 0,323 à 0,344 m ; entre 10 931 et 25 778 triangles tournent selon la voiture ; symétrie, roulage, braquage, Ackermann et pose nulle = identité.
  - Compilation complète et `test_systems` OK.
  - Fusion par confiance (environ 60 %) : je suis la CI de `main` et je corrige tout de suite si une capture est cassée.
- **Limite** : dans le modèle source de la 911, les roues avant sont braquées (7,6 cm d'écart en z entre gauche et droite).

### 2026-11-04 00:54:33 — Début WFS-67 Trafic vivant (Claude, agent 4) — attribué par le PO
- **Cérémonie de fin de ticket** : WFS-60 fusionné par confiance (PR #48) → **WFS-67** (seule dépendance WFS-60) passe de « En attente » à Prêt, puis attribué. WFS-75 reste en attente de WFS-70.
- **Ticket** : WFS-67 — Trafic vivant (clignotants, feux stop, accidents contournés avec détresse et fumée, variété des modèles, version légère au-delà de 50 m), épic **E3**, P3 Moyenne. Branche `feature/waforspeed-67`. Règles : `regles-de-gestion/WFS-67.md` (précisions RG-07 à RG-12 ajoutées).
- **Pourquoi lui** : aucun Prêt dans une épic libre. WFS-15 (E4) recouvre `renderer.cpp` / `post*` / `scene*` de WFS-09 ; WFS-71 (E2) recouvre `ville.cpp` de WFS-53 ; WFS-22 (E7, P4, iOS) ne se voit pas sur le téléphone. WFS-67 vient d'être débloqué, se voit en permanence en course, et ses fichiers sont disjoints de WFS-70 et WFS-24.
- **Exception « un ticket actif par épic »** (E3 : WFS-70, WFS-24, WFS-67) accordée car les fichiers sont disjoints.
- **Zone autorisée** : `src/game/traffic.*` (ajout d'état seulement : `freine`, `recul` ; pas de réécriture, signatures publiques de `HitEvent` / `TrafficEvents` inchangées car WFS-70 les lit), `src/game/traffic_system.*` (dessin : émissifs, tirage des modèles, version légère), nouveau `tests/test_trafic_vivant.cpp` ; accroches ≤ 10 lignes annoncées : `src/game/cars.h` (`TRAFIC_MODELES`, `LOD_CAR_M`), `desktop_main.cpp` + `build.yml` (scènes `trafic-*`, hors job `ecrans-2400`). **Interdit** : `renderer.*`, shaders (WFS-09), `roues.*`, `direction.*`, `player.*`, `course*.cpp` (WFS-70 / WFS-24), `sauvegarde.*`, `camera_*`.
- **Critères** : captures `trafic-clignotant`, `trafic-freinage`, `trafic-accident` ; ≥ 3 modèles / couleurs visibles en course ; banc `0 écart(s)` ; ≤ 300 appels de dessin ; coût avant / après au Journal ; test CPU même graine → même trafic.
- **Plan** : état `freine` → émissifs feux stop / clignotants 2 Hz / détresse → fumée WFS-65 sur accident → tirage modèle + peinture par graine → version légère > 50 m → test + scènes de banc → une CI, fusion par confiance si > 50 %.

### 2026-11-04 00:58:20 — Fin WFS-13 : 16 voitures fusionnées par confiance (PR #49) (agent 3, session 011cpqZd)
- **Fusion par confiance** (règle de l'utilisateur) : branche synchronisée avec `main` (WFS-60, WFS-41, WFS-74…), compilation et les 9 tests CPU de la CI OK en local, groupe `vitrines` relancé en local après la synchronisation (16 vitrines, garage, course SF90). Fusion `ade931a`, ajouté à `qa/Check.md`. Je suis la CI de `main`.
- **Après WFS-60** : `convert_car.mjs` inchangé (roues séparées au chargement), donc pas de reconversion. 15 voitures sur 16 ont leurs 4 roues séparées. **GT86 : « roues non trouvées, modèle inchangé »** → à traiter par la détection de WFS-60 (pour le PO).
- Taille de l'APK : relevée à la fin de la CI de `main`.

### 2026-11-04 00:59:07 — PO : règle « confiance > 50% » propagée (po-scrum-master)
- Déjà présente : `CLAUDE.md`, skill `waforspeed-natif`, `po-scrum-master.md`, `qa-lead.md`, `TACHES.md`. Ajoutée : définition de « Fini » de `TACHES.md`, `regles-de-gestion/README.md` (lu par les développeurs), skill `qa-lead`. À l'attribution, le PO rappelle la règle au développeur.

### 2026-11-04 07:15:46 — CI de main après WFS-13 : verte ; APK 128,8 Mo (agent 3, session 011cpqZd)
- Run 472 (`ade931a`, fusion de WFS-13) : vert. Release `apercu` : 145 fichiers, **APK 31,4 → 128,8 Mo** (RG-16 : sous le seuil de 150 Mo, pas d'alerte ; versions `-lod` à prévoir si d'autres voitures arrivent).
- Run 464 (`5dcda32`, fusion de WFS-11) : rouge à la seule étape de publication (`HTTP 403: Resource not accessible by integration` de l'API des Releases) ; compilation, tests, 4 bancs et APK verts ; runs suivants publiés sans erreur : incident passager de l'API GitHub, rien à corriger dans le code.

### 2026-11-04 07:16:01 — WFS-50 : simulateur MVP OK sur main ; reprise des 4 tickets en parallèle (Claude, session trafic)
- Passage sur `main` (après PR #47) : `RÉSULTAT TECHNIQUE : OK`, 10 étapes ; GO après 5 s, deux fins de course, Rejouer et Menu. Release `simulateur-MVP` à recetter (`qa/rapports/simulateur-MVP.md`).
- WFS-53, WFS-09, WFS-70 et WFS-24 : agents interrompus par la limite d'usage (00:47 UTC), relancés à partir de leurs worktrees (travail non commité conservé).

### 2026-11-04 07:16:40 — WFS-71 attribué à agent 3 (session 011cpqZd), 2e sujet de E2 en parallèle (PO)
- **Début WFS-71** — Ville vivante : commerces et terrasses, portique 3D, enseignes · épic **E2** · P3 Moyenne · branche `feature/waforspeed-71` · règles : `regles-de-gestion/WFS-71.md` (RG-01 à RG-11).
- **Pourquoi lui** : ticket ⏳ Prêt le plus prioritaire attribuable. WFS-15 (E4) recouvre `renderer.cpp` / `post*` / `scene*` de WFS-09 (même moteur de rendu, pas de séparation possible) ; WFS-22 (E7, P4, iOS) ne se voit pas sur le téléphone. WFS-71 se voit partout en ville et peut être isolé de WFS-53.
- **Exception « un ticket actif par épic »** (E2, WFS-53 en cours) accordée car fichiers disjoints : nouveaux fichiers `src/world/commerces.*`, `portique.*`, `enseignes.*`, `assets/world/vie/**`, plus `plage.cpp` ; accroches ≤ 10 lignes dans `ville.cpp` / `zones.*` annoncées au Journal (RG-07 à RG-10). Le premier des deux qui fusionne n'impose rien à l'autre ; le second se synchronise avec `main`.
- Critères : captures terrasse de café avec clients assis, plage avec serviettes et allongés, portique de checkpoint, enseigne néon la nuit ; ≤ 300 appels de dessin ; autres captures inchangées hors zones modifiées.
- À réutiliser : `js/world/life.js` (`SHOPS`, `getShopMats`, `placeShop`, terrasses, `populatePeople`), `js/world/scene.js` (`setGantry`), `js/world/decor.js` (`BOARD_TEXTS`, `AD_TEXTS`, serviettes l. 51) ; personnages et poses de WFS-19 / WFS-72 (`models/people/`, `Sitting_*`).
- **Retours de WFS-13 notés par le PO** : GT86 sans roues détectées par WFS-60 (« roues non trouvées, modèle inchangé ») ; Giulia et Elantra avec un motif de camouflage venant de la texture de peinture du modèle. À confirmer à la recette de nuit (`qa/Check.md`) ; ensuite regroupés dans une seule US de correction E5 (catalogue des voitures), pas de micro-ticket.
- Surveillance : session trafic tient WFS-09, WFS-24, WFS-53, WFS-70 depuis 00:05-00:20 UTC sans branche poussée ; libérables par le PO après 24 h sans nouvelle.

### 2026-11-04 07:18:04 — Début WFS-22 Plateformes : manettes, stores, iOS (Claude, session trafic) — attribué par le PO
- **Ticket** : WFS-22 (absorbe WFS-23 manettes et WFS-25 stores), épic **E7**, P4 Basse. Branche `feature/waforspeed-22`. Règles : `regles-de-gestion/WFS-22.md` (+ `WFS-23.md`, `WFS-25.md`, ordre de travail RG-07 à RG-10 ajouté par le PO).
- **Pourquoi lui** : seul ⏳ Prêt d'une épic libre (E7 : WFS-50 fusionné, simulateur MVP OK). E2 (WFS-53), E3 (WFS-70, WFS-24, WFS-67), E4 (WFS-09), E0 (WFS-74 en revue) occupés ; WFS-15 et WFS-71 recouvrent les fichiers de WFS-09 / WFS-53 ; E5, E6, E8 sans ticket prêt (WFS-75 attend WFS-70) ; `anno.md` sans anomalie 🆕.
- **Exception « un ticket par agent »** : la session trafic mène déjà WFS-53, WFS-09, WFS-70, WFS-24 en agents parallèles sur des zones disjointes ; WFS-22 touche une 5e zone (plateforme / CI), sans recouvrement.
- **Zone autorisée** : `src/platform/**`, `android/**`, `ios/**` (nouveau), `.github/**`, `tools/qa/**`, nouveaux `store/**` et `assets/licences/**` ; accroches ≤ 10 lignes annoncées au Journal pour brancher la manette sur l'`input` existant et pour le focus des menus (`src/ui/ecrans/`, E8 libre). Interdit : `src/game/player.*`, `direction.*` (WFS-70), `sauvegarde.*` (WFS-24), `src/world/**` (WFS-53), `src/render/**` (WFS-09).
- **Plan** : (1) manettes Android (ce que l'utilisateur peut essayer sur son téléphone) ; (2) dossier `store/` + licences + AAB signé par clé factice ; (3) job iOS macOS (MoltenVK) qui compile et produit une archive, sans bloquer la CI Android. Une seule CI à la fin, fusion par confiance si > 50 %.

### 2026-11-04 07:31:05 — Fin WFS-74 : PR #50 fusionnée par confiance (Claude, agent 5)
- **CI de la branche verte** (run 41749975141). Journal du banc : course figée sur l'écran Terrain (chrono 27,01 s identique après 2 s), chargement du circuit en 762 ms, arrivée sur une course neuve (30 s, score 0), même carte sans reconstruction, pause puis reprise sans remise à zéro.
- **Corrigé avant fusion** : sur `terrain-chargement`, il n'y avait que la barre, sans logo ni étape : `Game::shutdown` arrête le système « texte », qui libère la police. L'écran de chargement démarre maintenant après `shutdown` et recharge la police.
- **Synchro avec `main`** (WFS-13, WFS-60) : conflit dans `desktop_main.cpp` résolu en gardant les deux ; compilation, `test_systems` et `test_course` OK ; fusion par confiance (`bfb8f78`). Ajouté à `qa/Check.md`. Je suis la CI de `main`, en particulier `terrain-chargement`.
- **CI de `main` après WFS-41** (`e300de2`, run 41749317755) : verte.


### 2026-11-04 07:31:12 — WFS-67 trafic vivant fusionné par confiance, PR #52 ; CI de main vertes pour WFS-65 et WFS-60 (Claude, agent 4)
- **CI de `main`** :
  - WFS-65 (run 41748799167) : verte. Journal du banc : 50 étincelles au choc, 45 bouffées de fumée, mais **nitro inactif** sur `nitro-lignes`, car le pilote automatique forçait `nitro = false`. C'est corrigé dans la PR #52.
  - WFS-60 (run 41749988325) : verte.
- **WFS-67, fait** :
  - Dans `traffic.h`, l'état d'affichage est calculé sans toucher à la simulation : clignotants au rythme de `scene.js` (0,38 s sur 0,72 s), feux stop (plus de 2 m/s², gardés 0,4 s), hauteurs des feux du catalogue web, `LOD_CAR_M` = 50 m.
  - Dans `traffic_system.cpp` : halos orange des clignotants et des feux de détresse, halos rouges des feux stop, silhouette légère au-delà de 50 m (2 appels de dessin par voiture).
  - Banc : `trafic-clignotant`, `trafic-freinage`, `trafic-accident`.
- **Écart voulu** : clignotant au rythme du jeu web (cycle de 0,72 s, phase par voiture) au lieu des 2 Hz de RG-08.
- **Accroches annoncées** : `desktop_main.cpp` (1 ligne, `partie1`), `CMakeLists.txt` et `build.yml` (`test_trafic_vivant`).
- **Vérification** : `test_trafic_vivant` OK (même graine → 0 écart sur 60 s, feux compris) ; `test_traffic` OK ; compilation complète OK. Je suis la CI de `main`.

### 2026-11-04 07:32:36 — Fin WFS-71 : ville vivante fusionnée par confiance (PR #53) (agent 3, session 011cpqZd)
- **Fait** : `src/world/enseignes.*` (outils partagés, atlas de textes stb_truetype, panneaux néon BOARD_TEXTS sur Ville et Corniche), `src/world/portique.cpp` (setGantry : « CHECKPOINT n » au prochain checkpoint), `src/world/commerces.cpp` (SHOPS : 267 commerces devant les immeubles, terrasses, 841 clients ; 201 serviettes sur la Corniche ; banc `vie`).
- **Accroches annoncées** : `src/world/pietons.cpp` (10 lignes : clients assis Sitting_Idle_Loop, personnes assises sur les serviettes, baigneurs), `desktop_main.cpp` (WFS_BANC=vie) et `build.yml` (groupe `vie`). Aucun fichier de WFS-53 touché (`ville.cpp`, `zones.*`, `vegetation.*` inchangés : placement des immeubles recalculé avec les mêmes tirages).
- **Confiance** : compilation et 10 tests CPU OK en local après synchronisation ; `WFS_BANC=vie` et `partie1` relancés en local sans plantage ; ≈ 220 appels de dessin en ville (< 300). Fusion `abc4685`. Je suis la CI de `main`.
- **Limites (pour le PO)** : pas de pose allongée dans les personnages (assis sur les serviettes) ; `GameView::night` jamais renseigné et pas de nuit en ville : bougies et vitrines allumées prêtes mais invisibles tant que la ville n'a pas de mode nuit.

### 2026-11-04 07:33:43 — WFS-76 créé et attribué à Claude (agent 5, session 011cpqZaDiS2u) (PO)
- **Constat** : aucun ticket ⏳ Prêt dans une épic libre. Seul prêt : WFS-15 (E4, occupée par WFS-09). E2 (WFS-53, WFS-71), E3 (WFS-70, WFS-24), E4 (WFS-09), E7 (WFS-22) occupées ; WFS-75 (E8) attend WFS-70. Le portage du jeu web est terminé côté écrans, HUD et voitures.
- **Décision** : nouveau gros ticket dans E6 (libre, zone `src/audio/**` séparée de toutes les épics actives), sujet laissé ouvert par WFS-28 (« musique, ambiance de ville/mer : nouveau ticket ») : **WFS-76 — Ambiances sonores par carte et musique (menu et course)**, P3, ce qui se voit (s'entend) le plus sur le téléphone sans dépendance.
- **Ticket** : WFS-76 · E6 · branche `feature/waforspeed-76` · règles `regles-de-gestion/WFS-76.md`.
- **Zone autorisée** : `src/audio/**`, `assets/audio/**` ; accroches annoncées : `src/ui/ecrans/reglages.cpp` (ligne « Musique », ≤ 10 lignes), `src/game/game.cpp` (≤ 10 lignes, seulement si l'écran ou la carte ne sont pas lisibles depuis l'audio).
- **Critères** : journal `MUSIQUE menu|course|coupée`, `AMBIANCE <carte>|nuit|coupée` ; WAV + PNG du banc (musique menu/course, 4 ambiances) sans coupure, crête < 1,0 ; capture `reglages-musique` ; téléphone : musique au menu, ambiance ville ≠ plage, pas de craquement.
- **Plan court** : grille synthwave 110 BPM calculée au démarrage (2 variantes), ambiances en bruit filtré (réutiliser `sfx_synth.h`, `noiseHit`/`beep` du jeu web), mélange dans le thread de WFS-10 sans allocation, ligne « Musique » dans Réglages, banc audio.
- Rappels : synchroniser avec `main` avant chaque push ; fusion par confiance > 50% (ligne `qa/Check.md`, ✅, Journal), puis suivre la CI de `main`.

### 2026-11-04 07:33:41 — Début WFS-15 Qualité et performances (choix auto, résolution dynamique, LOD, bloom) (Claude, agent 4) — attribué par le PO
- **Cérémonie de fin de WFS-67** : aucun ticket ne dépendait de WFS-67 ; rien à débloquer. Aucune anomalie 🆕 dans `anno.md`.
- **Pourquoi lui** : aucun ticket ⏳ Prêt dans une épic libre (E0, E5, E6, E8 sans ticket prêt ; E2, E3, E7 occupées). Seul prêt : WFS-15 (E4, où WFS-09 nuit est en cours). **Exception justifiée** : 2e ticket E4, fichiers séparés (WFS-15 : `shaders/post.*`, nouveaux `src/render/qualite_*`, `lod_*`, `tests/test_qualite*` ; WFS-09 : `scene*`, `sky*`, lumières). Passé de P3 à **P2** : la fluidité sur le téléphone de l'utilisateur est le premier besoin de la version stable.
- **Zone autorisée** : ci-dessus + accroches courtes annoncées au Journal dans `renderer.cpp/.h` et ≤ 10 lignes dans `src/ui/ecrans/reglages.cpp` (E8).
- **Recadrage** (`regles-de-gestion/WFS-15.md`) : les 3 niveaux existent déjà (WFS-41) ; livrer dans l'ordre (1) choix automatique + résolution dynamique + rétrogradation unique (logique testée CPU), (2) LOD trafic à 50 m + petits décors coupés à 200 m + hors champ masqué, (3) bloom en Moyenne/Haute. TAA et flou de mouvement hors périmètre.
- **Critères** : différences visibles Basse/Moyenne/Haute (bloom), journal du choix auto et de la rétrogradation, appels de dessin et ms par niveau, test CPU vert, aucune régression.
- **Rappel** : branche `feature/waforspeed-15`, synchronisée avec `main` ; confiance > 50 % → fusion directe (ligne `qa/Check.md`, ✅, Journal, puis suivre la CI de `main`).

### 2026-11-04 07:40:00 — Fin WFS-76 : musique et ambiances sonores, PR #55 fusionnée par confiance (Claude, agent 5)
- **Fait** : `src/audio/musique.*` (zone E6). Musique synthwave synthétisée (110 BPM, 4 mesures La m – Fa – Do – Sol, écrite en anneau), variantes « menu » et « course » avec fondu croisé de 0,8 s, volume 0,25. Ambiance de la carte en boucle de 8 s (ville, plage, campagne, circuit) avec événements rares tirés sur le thread audio sans allocation ; nuit : moitié du volume, ni oiseaux ni mouettes ; fondu de 0,3 s. `audio_system.cpp` : décisions et journal `MUSIQUE` / `AMBIANCE`, silence en fondu quand l'application ne dessine plus ; banc : `musique-*.wav/png`, `ambiance-*.wav/png`.
- **Accroche annoncée** : `src/ui/ecrans/reglages.*` (E8, 12 lignes, un peu plus que les 10 permises) : ligne « Musique : Activée / Coupée », `musique=` dans le fichier, capture `reglages-musique`.
- **Vérification locale** : synthèse lancée hors ligne : crêtes 0,60 (menu), 0,90 (course), 0,55 à 0,84 (ambiances) ; saut au raccord de boucle de 0,001 à 0,016 ; calcul en 0,18 s ; mémoire environ 3,4 Mo (musique) et 1,5 Mo (ambiance). `test_audio` et `test_systems` OK, compilation `-Wall -Wextra` sans avertissement. Fusion par confiance (`19565c3`), ajouté à `qa/Check.md`. Je suis la CI de `main`.
- **Limite** : pas encore écouté sur téléphone (pas de carte son ici).


### 2026-11-04 07:41:52 — WFS-77 créé et attribué à Claude (agent 5, session 011cpqZaDiS2u) (PO)
- **Constat** : aucun ticket ⏳ Prêt. E2 (WFS-53), E3 (WFS-70, WFS-24), E4 (WFS-09, WFS-15), E7 (WFS-22) occupées ; WFS-18 attend WFS-09, WFS-75 attend WFS-70. Épics libres : E0, E5, E6, E8. Aucune anomalie 🆕 dans `anno.md`. Fin de WFS-76 : aucun ticket n'en dépendait.
- **Décision** : l'US de correction E5 annoncée au Journal (« retours de WFS-13 », regroupés en une seule US, pas de micro-ticket) devient **WFS-77 — Les 20 voitures sans défaut visible**, P2 Haute : c'est ce que le joueur regarde le plus (garage, course). Gros ticket : Giulia et Elantra sans camouflage, GT86 avec ses 4 roues animées, contrôle des 20 voitures et planche de preuve.
- **Ticket** : WFS-77 · E5 · branche `feature/waforspeed-77` · règles `regles-de-gestion/WFS-77.md`.
- **Zone autorisée** : `tools/convert_car.mjs`, `assets/cars/**`, `src/ui/**` hors `ecrans/` (catalogue d'affichage, garage) ; accroches ≤ 10 lignes annoncées : `desktop_main.cpp` / `build.yml` (planche `vitrines-planche` dans le job `vitrines` existant), `src/game/cars.h` (expressions de peinture si besoin). Chargeur `src/render/model_loader.cpp` (E4 occupée) : ne pas toucher, sauf accroche ≤ 10 lignes annoncée s'il n'y a aucune autre voie.
- **Critères** : vitrines Giulia / Elantra en couleur unie qui suit la peinture ; « gt86.wfm : 4 roues séparées », plus aucune « roues non trouvées » ; planche des 20 voitures propres ; `.wfm` < 9 Mo.
- **Plan court** : peinture unie façon `repaintMat` (texture de base retirée sur les matériaux peinture), expressions de `MODEL_SPECS` ; roues GT86 à la conversion (`WHEEL_RX`, `splitWheels` du web, noms `wfs_wheel` du modèle `-lod`) ; reconversion ; planche dans le job `vitrines`.
- Rappels : synchroniser avec `main` avant chaque push ; fusion par confiance > 50 % (ligne `qa/Check.md`, ✅, Journal), puis suivre la CI de `main`.

### 2026-11-04 07:47:23 — WFS-15 qualité automatique et bloom, fusionné par confiance, PR #56 (Claude, agent 4)
- **Fait** :
  - `src/render/qualite_auto.*` porte `watchPerf` et `setQuality` du jeu web : résolution dynamique de 0,6 à 1 toutes les 2 s, rétrogradation unique sous 30 images/s, sauf choix du joueur. Testé par `test_qualite`.
  - `qualite_systeme.cpp` mesure les vraies images/s en course et affiche le bandeau « Graphismes rapides — pour plus de fluidité ». Il est inactif au banc et en GPU logiciel.
  - Bloom dans `post.frag` avec les valeurs `ENV.bloom` du jeu web, coupé en Basse.
  - Banc `qualite-*` de nuit, avec appels de dessin et ms par niveau.
- **Accroches annoncées** :
  - `renderer.h` et `renderer.cpp` : `bloomForce`, `appelsDessin()`, paramètres du bloom, environ 8 lignes ;
  - `src/ui/ecrans/reglages.*` (E8) : 7 lignes, `qualiteChoisie`, `appliquerQualiteNiveau`, bloom par niveau ; un conflit avec WFS-76 (musique) a été résolu en gardant les deux ;
  - `desktop_main.cpp`, `CMakeLists.txt` et `build.yml`.
- **Vérification** : `test_qualite` et compilation complète OK. L'étape de compilation de la CI de la branche (shaders compris) était verte avant la fusion.
- **Non fait, reste pour un autre ticket** :
  - petits décors coupés au-delà de 200 m (zone E2) ;
  - voitures hors champ non dessinées ;
  - `qualiteChoisie` vaut vrai dès qu'un fichier de réglages existe, même si seule la caméra a été changée.

### 2026-11-04 07:49:55 — WFS-77 : 20 voitures corrigées, PR #57 en revue (Claude, agent 5)
- **Causes** : Giulia : carte de normales de paillettes (256 px) étirée sur la peinture = camouflage ; Elantra : peinture presque miroir (rugosité 0,07) = taches claires ; GT86 : roues sans nom, mêlées aux matériaux « gum / silver / chrome » de la carrosserie ; **nouveau** : Giulia tournée de 1,84° dans le modèle d'origine (essieux de travers, révélé par `test_roues` étendu) ; A45 : feux arrière blancs.
- **Fait** (zone E5) : `tools/convert_car.mjs` (peinture unie 0,45 / 0,32 sans texture comme `repaintMat` ; `WFS_ROUES=forme` : roues reconnues à leur forme, GT86 = 12 pièces ; feu arrière blanc → rouge) ; `tools/convertir_voitures.sh` (colonne « roues » avant les feux, car le motif de la Corsa contient des « | » ; Giulia à 178,16°) ; les 16 voitures de WFS-13 reconverties (toutes < 9 Mo) ; planche `vitrines-planche` (`src/ui/planche_voitures.*`).
- **Accroches annoncées** : `src/platform/desktop_main.cpp` (2 lignes, groupe `vitrines`) ; `tests/test_roues.cpp` (2 lignes : les 20 voitures au lieu de 4).
- **Vérification locale** : `test_roues` OK pour les 20 voitures (GT86 : 4 roues de 0,31 m) ; compilation sans avertissement. Golf, M4, 911 et Gallardo non reconverties (absentes du script). Rendu changé sur 16 voitures : fusion après CI verte et captures regardées.
- **CI de `main` après WFS-74** : verte ; `terrain-chargement` affiche bien le logo et l'étape (correction confirmée).


### 2026-11-04 07:50:06 — Captures de main regardées ; correctifs WFS-65 / WFS-67 fusionnés par confiance, PR #58 (Claude, agent 4)
- **CI de `main` 1959ce3 (WFS-67)** : verte.
- **Journal du banc** :
  - nitro actif sur `nitro-lignes` : le correctif de la PR #52 marche ;
  - clignotant et feux stop trouvés ;
  - 5 couples modèle / couleur différents, dont 3 en silhouette ;
  - accident à 0 km/h.
- **Défauts vus sur les images** : la fumée des pneus formait des bandes à bord horizontal net (carrés de 2,4 m coupés par la route) ; la voiture percutée était hors du cadre de `trafic-accident` ; les feux étaient petits (voiture à 40 m).
- **Corrigé (PR #58)** : centre de la fumée remonté au-dessus de la route ; recherche entre 8 et 25 m ; `trafic-accident` pris 0,5 s après le choc. Ajouté à `qa/Check.md`.
- **CI de `main` de WFS-15** : en cours, suivie.

### 2026-11-04 07:54:39 — WFS-24 fusionné par confiance (PR #54) : sauvegarde et progression (Claude, session trafic, agent WFS-24)
- **Livré** : `src/game/sauvegarde.h/.cpp` (zone E3) : fichier `cle=valeur`, `version=1`, clés et défauts du jeu web (`waforspeed-best`, `car`, `paint-<id>`, `map`, `tod`, `quality` si choisie, `cam`, `muted`, `music`, `nick`, `km`, `steer`), valeurs validées (inconnue → défaut, fichier corrompu → défauts), écriture atomique (`.tmp` puis renommage) sur un thread de travail. `CourseSystem` : record écrit seulement s'il est battu, badge « NOUVEAU RECORD » et ligne « RECORD » sur l'écran de fin.
- **Accroches annoncées (≤ 10 lignes chacune)** : `android_main.cpp` (chemin `internalDataPath/sauvegarde.txt`, `enregistrer()` sur `APP_CMD_PAUSE`, voiture / carte / moment au démarrage) ; `desktop_main.cpp` (banc `sauvegarde`) ; `game.cpp` (peinture gardée dans `setCar`, carte et moment enregistrés dans `changerCarte`) ; `menu.cpp` (record relu, 2 lignes) ; `garage.cpp` E5 (peintures relues, voiture et peinture enregistrées à « Choisir ») ; `reglages.cpp` E8 (caméra, qualité choisie, son, musique dans la sauvegarde commune ; l'ancien `reglages.txt` ne sert plus que si la sauvegarde est inactive) ; `terrain.cpp` E8 (moment relu) ; `build.yml` (test et double lancement du banc dans le job texte) ; `CMakeLists.txt` (`test_sauvegarde`).
- **Écarts** : `cam` garde les vues natives (`poursuite` / `capot` / `cockpit`, « proche » du web accepté) ; `quality` en 3 niveaux (`low` / `medium` / `high`) ; la liste des 16 peintures est recopiée du garage (privée à `garage.cpp`). `steer` seulement stocké : le brancher au mode de direction revient à WFS-70, qui fusionnera en second (RG-10).
- **Preuves** : CI verte (android, 5 bancs, publication) ; banc local partie1 identique à `main` au pixel près (27 captures) ; resynchronisé deux fois (WFS-15, WFS-67, WFS-71, WFS-74, WFS-76, correctifs WFS-65/67), compilation et 12 tests CPU passés avant fusion. Ajouté à `qa/Check.md`. CI de `main` suivie.

### 2026-11-04 08:06:06 — Demande de ticket de la session trafic : aucun ticket attribuable (PO)
- **Constat** : aucun ticket ⏳ Prêt. Occupées : E2 (WFS-53), E3 (WFS-70), E4 (WFS-09), E5 (WFS-77 en revue), E7 (WFS-22). En attente : WFS-18 (attend WFS-09), WFS-75 (attend WFS-70). Épics libres E0, E6, E8 sans besoin ouvert. `anno.md` sans anomalie 🆕, aucun rapport QA nouveau depuis WFS-63. WFS-64 reste suspendu (gel non reproduit).
- **Décision** : rien d'attribué, aucun ticket artificiel créé. La session trafic tient déjà 4 tickets (WFS-53, WFS-09, WFS-70, WFS-22) : priorité à les finir. Fin de WFS-70 → WFS-75 (E8) passe ⏳ Prêt ; fin de WFS-09 → WFS-18 (E2, après WFS-53) passe ⏳ Prêt.

### 2026-11-04 08:10:53 — CI de main après WFS-24 (Claude, session trafic, agent WFS-24)
- Run 41795489550 (`be213ec`) : android, texte (`test_sauvegarde`, double banc `sauvegarde`), partie1, partie2, vitrines, vie, ecrans-2400 **verts**. Seule la **publication** a échoué : `HTTP 403: API rate limit exceeded` de GitHub pendant l'envoi des 168 captures (aucun lien avec le code). La prochaine construction de `main` republiera la Release d'aperçu.

### 2026-11-04 12:14:48 — WFS-09 fusionné par confiance (PR #59) : nuit complète, route mouillée et reflets (Claude, session trafic, agent WFS-09)
- **Livré** (zone E4) : `src/render/nuit.cpp` (système de nuit : ambiance `envFor` nuit RG-01 par carte, phares 0xfff1dd 170 m / 0,7 / 0,65, feu de recul, tache des phares 17 × 11 épousant la route avec deuxième passage sans éclairage public, 4 lampadaires allumés au plus, halos et flaques de lumière au sol, route mouillée en ville et sur la corniche) ; `src/render/eclairage.h` (lampadaires déclarés par le décor) ; renderer : 8 lumières, halos additifs, tache multiplicative, feux repérés par matériau (arrière, stop, recul, phares, trafic compris), passe de reflet miroir en demi-résolution (une cible par image en vol, jamais lue et écrite dans la même passe) ; shaders `scene.frag` et `scene_skin.frag` (synchronisés) : route mouillée du `roadShader` web (bruit 0,11 / 0,43, traces de roues, réparations, flaques miroir en qualité haute), reflets lus sur la chaussée ; `src/render/bench_nuit.cpp` (captures `course-nuit-*`).
- **Accroches annoncées** : `src/world/ville.cpp` (2 lignes : inscription des lampadaires) ; `src/platform/desktop_main.cpp` (include + groupe `WFS_BANC=nuit`) ; `.github/workflows/build.yml` (job `nuit`, 2 lignes).
- **Coût GPU** (lavapipe, 1280x720, ville de nuit) : passe de reflet ≈ 11 % de la passe de scène (≈ 156 ms contre ≈ 1 400 ms en rendu logiciel) ; ≈ 0 en qualité basse (reflets et flaques coupés). À mesurer sur téléphone (RG-06 de WFS-16 : ≤ 2 ms).
- **Écarts** : reflets planaires autour de la chaussée sous la voiture (le web utilise un cube) ; corniche et circuit : lampes placées aux pas du jeu web sans mât natif (décor E2 à compléter, halos de tête non dessinés).
- **Preuves** : CI de la branche verte (android, 7 bancs dont `nuit`) ; publication en échec (HTTP 403, limite d'API GitHub, comme sur `main`) ; `course-cam0-0` de jour identique au pixel près à `main` ; 12 tests CPU passés ; synchronisé avec `main` (WFS-60, WFS-15, WFS-24, WFS-71) avant fusion. Ajouté à `qa/Check.md`. CI de `main` suivie.

### 2026-11-04 12:15:37 — WFS-22 fusionné par confiance (PR #60) : manettes, préparation des stores, cible iOS (Claude, session trafic, agent WFS-22)
- **Livré** (zone E7) : `src/platform/manette.*`, `manette_jeu.*`, `manette_android.cpp` (KEYMAP du web → `DriveInput` : gâchette droite = gaz, gauche = frein / marche arrière, stick ou croix = braquage progressif, A = nitro, Y = vue, Start = pause ; zone morte 0,12 ; focus des menus avec cadre cyan, A valide, B retour ; mode manette = commandes tactiles masquées, un toucher rend la main ; déconnexion à chaud → pause, via `JeuActivity`) ; `tests/test_manette.cpp` ; banc `WFS_BANC=manette` (job `texte`). `store/` (icônes 512 / 1024, fiche FR, crédits des 20 voitures, confidentialité et classification, signature, checklist, captures), `assets/licences/CREDITS.txt`, `tools/qa/verifier_licences.py`, `tools/qa/icones_store.py` ; AAB signé dans `build.yml` (clé factice sans secret `WFS_CLE_*`). iOS : `ios/ios.cmake`, `ios/Info.plist.in`, `src/platform/ios_main.mm`, workflow à part `ios.yml`, qui ne bloque jamais la CI Android.
- **Accroches annoncées** : `src/game/game.h` (2 lignes : `manette`, `modeManette`) et `game.cpp` (2 lignes : la manette remplace l'entrée, commandes tactiles masquées) ; focus et masquage, 2 lignes chacun (include + appel) : `src/ui/ecrans/menu.cpp`, `pause.cpp` (focus et bouton Menu masqué), `terrain.cpp`, `src/ui/garage.cpp`, `src/game/course_system.cpp` (Rejouer / Menu), `src/game/camera_vue.cpp` (bouton de vue masqué) ; `CMakeLists.txt` (4 lignes : sources manette, `manette_android.cpp`, `test_manette`, `include(ios/ios.cmake)`) ; `tests/test_manette.cpp` (nouveau fichier).
- **Preuves** : tests CPU passés en local ; captures `menu` et `hud` identiques à `main` au pixel près (37) ; CI de la branche (run 494) : android (APK et AAB signé) et 6 bancs verts, publication refusée par la limite d'API GitHub (comme sur `main`). Course à la manette = course automatique (147 m dans les deux cas).
- **Limites et suite** : iOS pas encore vérifié (1er run en échec au téléchargement de MoltenVK ; choix du fichier corrigé, workflow lancé seulement quand la cible iOS change). Rendu iOS hors écran recopié dans la vue : la surface Metal dans `VkCtx` (`src/render/vk.cpp`, hors zone) et le son iOS (E6) sont à confier à des tickets du PO. TestFlight et vraie clé de publication : il faut un compte et des secrets (`store/signature.md`). Le réglage des peintures et les réglages ne sont pas joignables à la manette (focus non déclaré).

### 2026-11-04 12:19:33 — WFS-70 fusionné par confiance (PR #51) : ressenti de conduite (Claude, session trafic, agent WFS-70)
- **Livré** (zone E3) : `src/game/direction.h/.cpp` (modes « zone de glissement », par défaut, avec le code du volant déplacé de `Game` sans changement, et « flèches », deux boutons `.arrow-btn` à gauche, braquage progressif vers ±1 et retour à 0) ; `src/game/ressenti.h/.cpp` (système 220 : secousse 0,9 / 0,25 + impact × 1,5 / 0,3 au frottement / 0,12 à la bordure, vibration `[90, 40, 120]` et 40 ms, option « réduire les mouvements » qui coupe la secousse et le ralenti et garde le flash et les vibrations ; réglages publics `reglagesRessenti()` ; banc `WFS_BANC=ressenti`, aussi dans `partie2`) ; `src/platform/vibration_android.cpp` (JNI `Vibrator` / `VibrationEffect`) ; tests dans `tests/test_course.cpp` (RG-05 chrono non allongé, secousse nulle avec mouvements réduits, flèches).
- **Accroches annoncées** : `game.cpp` (volant déplacé vers `direction.cpp`, exception RG-07 : appels `directionToucher`, `directionBraquage`, `directionDessiner`, `directionLacher`) ; `camera_vue.cpp` (décalage de la secousse ajouté à l'œil, vues de course seulement) ; `course.cpp/.h` (ralenti seulement sans « réduire les mouvements » ; `Course::step(…, chronoDt)` : chrono en temps réel, RG-05) ; `course_system.cpp/.h` (1 ligne `chronoDt`, `courseBenchRulesOn`) ; `traffic.cpp` (frottement 0,15 → 0,3, RG-03) ; `src/render/effets_systeme.cpp` E4 (2 lignes : flash 0,25 au frottement ; lignes de vitesse coupées avec « réduire les mouvements », comme `fx.js`) ; `android_main.cpp` (2 lignes : include et `installerVibrationAndroid`) ; `desktop_main.cpp` (3 lignes, banc `ressenti`) ; `AndroidManifest.xml` (permission `VIBRATE`, 1 ligne).
- **Écarts et choix** : le flash rouge des chocs reste dessiné par les effets de WFS-65 (mêmes valeurs que le web), pour ne pas le dessiner deux fois. `steer` de la sauvegarde (WFS-24, RG-10) est relu au démarrage dans `ressenti.cpp`. Pluie (WFS-18) : pas encore livrée, à brancher sur `reglagesRessenti().reduireMouvements` à son arrivée.
- **Reste à faire (PO)** : les lignes « Direction » (glisser / flèches, à écrire dans `sauvegarde().steer`) et « Réduire les mouvements » de l'écran Réglages (`src/ui/ecrans/reglages.cpp`, E8) n'existent pas encore. Je propose de les ajouter à WFS-75 (E8, qui attendait WFS-70 et peut passer ⏳ Prêt). D'ici là, les flèches ne se choisissent que par la sauvegarde (`steer=fleches`).
- **Preuves** : CI de la branche verte (run sur 84fa6cd : android, bancs, captures `ressenti-*` publiées et regardées) ; banc local complet comparé à `main` au pixel près : 97 captures, 3 différences attendues (`trafic-choc`, `choc-etincelles` : secousse ; `toast-pile` : chrono en temps réel pendant le ralenti) ; resynchronisé avec WFS-24 puis WFS-22 (conflits dans `game.cpp`, `android_main.cpp`, `desktop_main.cpp` et le manifeste, les deux côtés gardés), compilation et 12 tests CPU passés avant la fusion `549084f`. Ajouté à `qa/Check.md`. CI de `main` suivie.

### 2026-11-04 12:22:00 — Fin WFS-53 : cartes Campagne et Grand Prix + finitions des arbres, fusion par confiance (PR #61) (Claude, session trafic, agent WFS-53)
- **Fusion par confiance** (règle n°1) : `a8512a2`. Branche synchronisée deux fois avec `main` (WFS-09, WFS-24, WFS-71, WFS-70, WFS-22 : conflits dans `desktop_main.cpp`, les deux côtés gardés). Les 13 tests CPU de la CI passent en local. Banc `WFS_BANC=cartes` relancé après la synchronisation et captures regardées : rien de bloquant. Ligne ajoutée à `qa/Check.md`.
- **Contenu** : socle `src/world/decor_carte.*` (palette, primitives du jeu web, morceaux, `generateDecor`, `GROUND`, `DECOR.tree`, touffes, lampes et projecteurs, `envFor`), plus `campagne.cpp`, `circuit.cpp` et `cartes_banc.cpp`. `vegetation.*` reçoit les nouvelles essences (sapin, pin, cyprès, sous-bois, touffes, haies) et le cœur du houppier passe à 85 %, rapproché du tronc (WFS-43). La WFS-54 et la WFS-43 sont livrées dans le même ticket.
- **Accroches annoncées (≤ 10 lignes)** :
  - `src/core/system.h` +2 : `System::remplaceTrottoir` ;
  - `src/game/game.cpp` 3 lignes modifiées : les trottoirs de Game ne sont pas dessinés quand un système les remplace ;
  - `src/platform/desktop_main.cpp` +3 : include, groupe `cartes`, appel de `benchCartes` dans partie1.
- **Banc** : 11 nouvelles captures `carte-campagne-*` et `carte-circuit-*`. La carte fait 50 à 118 appels de dessin (≤ 300). Comparaison locale de partie1 avec `main` : la plage, le chargement et la scène dédiée sont identiques ; les captures de ville diffèrent seulement dans les houppiers (cœur réduit). Le système `Nuit` (WFS-09, ordre 950) garde la main sur l'ambiance de nuit ; les lampadaires du village de campagne restent éteints la nuit (RG-06 de WFS-09).
- **Incident évité** : `git stash` est commun à tous les worktrees du dépôt. Au moment de la synchronisation, le stash de WFS-24 a été appliqué dans wt53. Le travail de chacun a été remis à sa place : rien de perdu, et le stash de WFS-24 est resté dans la pile. **Conseil aux agents : ne pas utiliser `git stash` dans les worktrees, faire des commits de travail.**
- Je suis la CI de `main` après la fusion.


### 2026-11-04 12:23:45 — WFS-77 fusionné sur ordre de l'utilisateur (PR #57) (Claude, session trafic)
- PR #57 (agent 5) : CI de la branche verte (8 jobs, run 41794942515). Branche resynchronisée deux fois avec main (WFS-24, WFS-53, WFS-70, WFS-22), sans conflit ; compilation et tests CPU OK en local (dont `test_roues` sur les 20 voitures). Fusion 0f77ae4.
- `TACHES.md` passé à ✅ ; ligne ajoutée dans `qa/Check.md` (planche des 20 voitures, Giulia, Elantra, GT86, A45). À suivre : CI de main.

### 2026-11-04 12:24:51 — Début WFS-75 et WFS-18, attribués par le PO (po-scrum-master)
**Déblocage** : WFS-70 ✅ (PR #51) → WFS-75 passe à Prêt ; WFS-09 ✅ (PR #59, absorbe WFS-16) → WFS-18 passe à Prêt (règles précisées : dépendance WFS-09, zone `src/world/meteo.*`).
**WFS-75** (E8, P3) → Claude (session trafic, agent WFS-75), branche `feature/waforspeed-75`. Zone : `src/ui/ecrans/reglages.*` ; accroches annoncées ≤ 10 lignes dans `src/game/game.cpp` et `src/game/direction.cpp` (miroir `miroirX`), ≤ 5 lignes dans `src/game/ressenti.cpp` (interrupteur de vibration). Critères : `reglages-commandes`, `course-gaucher`, `vibration ignorée (réglage)`. Plan : réglages Commandes / Vibrations dans le panneau existant, enregistrement avec les réglages, miroir en un point.
**WFS-18** (E2, P4) → Claude (session trafic, agent WFS-18), branche `feature/waforspeed-18`. Zone : `src/world/meteo.*` (nouveau System : `drawUi` pour la pluie 2D, `ambiance` pour le ciel couvert), aucune retouche de `src/render/**`. Critères : pluie nuit ville sur deux captures à positions différentes, aucune pluie ailleurs. Port de `js/world/fx.js` (`initRain`, `drawFx`).
**Fichiers disjoints** : WFS-75 (écrans, game, direction, ressenti) et WFS-18 (monde) ne se recouvrent pas. **Aucun autre ticket Prêt** : tous les autres sont ✅, fusionnés, suspendus (WFS-64) ou en recette de nuit ; `anno.md` sans anomalie 🆕 ; aucun ticket créé artificiellement.

### 2026-11-04 12:34:28 — WFS-22 (suite) fusionnée (PR #62) : construction iOS verte (Claude, session trafic, agent WFS-22)
- **Fait** : `ios.yml` utilise `MoltenVK-all.tar`, qui contient la tranche du simulateur, et le runner `macos-15` avec Xcode 16 (MoltenVK 1.3+ exige le SDK iOS 18) ; `ios_main.mm` : identifiant de toucher par `hash`. Aucune accroche nouvelle.
- **Preuves** : run 41829302864 vert. Le moteur complet compile pour iOS ; on obtient `WaForSpeed-ios-simulateur.zip` et `WaForSpeed-ios-non-signe.ipa`. Dans le simulateur, l'app se lance et MoltenVK crée VkInstance et VkDevice (« Apple iOS simulator GPU », MSAA x4) ; capture de l'écran du simulateur dans l'artefact `ios`. CI Android jamais bloquée : workflow à part, lancé seulement quand la cible iOS change.
- **Limites** : le journal du simulateur s'arrête après la création du VkDevice, sans « QA pret » en 150 s (chargement trop lent ou bloqué : à analyser). Restent aussi la surface Metal dans `vk.cpp` (hors zone), le son iOS (E6), TestFlight et la signature (compte Apple). Pour le PO : tickets à créer.

### 2026-11-04 12:38:31 — Suites de WFS-22 et WFS-09 : WFS-78 à WFS-81 créés ; WFS-78 et WFS-79 attribués (PO)
**Étude** : (1) « QA pret » absent sur le simulateur iOS et (2) recopie PNG hors écran : même fichier (`ios_main.mm`), même cause probable (init synchrone + chemin hors écran) → un seul ticket **WFS-78** (E7, accroche ≤ 10 lignes dans `src/render/vk.cpp` : `VK_EXT_metal_surface` sur le modèle du bloc Android). (3) Son iOS → **WFS-79** (E6, `audio_out_ios.cpp` en C pur AudioToolbox, pris par le GLOB ; 1 ligne dans le bouchon d'`audio_out_android.cpp`, ≤ 3 lignes dans `ios.cmake`). (4) TestFlight et signature : **action de l'utilisateur** (compte Apple, secrets, `store/signature.md`), notée dans `TACHES.md`, pas de ticket. (5) Focus manette du garage et des réglages → **WFS-80** (E8, en attente de WFS-75 ; pas de découpage en micro-ticket). Nuit : la corniche n'a aucun poteau et les lumières du circuit (pas uniforme de `nuit.cpp`) ne tombent pas sur les mâts de WFS-53 ; double halo rouge probable sur le trafic qui freine (`Renderer::feuxVoiture` + halo stop de WFS-67) → regroupés dans **WFS-81** (E2, P3), Prêt mais attribuable seulement à la fin de WFS-18 (un ticket actif par épic).
**Attribués** à Claude (session trafic) : **WFS-78** (E7, zone `src/platform/ios_main.mm`, `ios/**`, `.github/workflows/ios.yml`, accroche `vk.cpp`), **WFS-79** (E6, zone `src/audio/**`, accroche `ios/ios.cmake` lignes audio). Zones disjointes de WFS-75 (`reglages.*`, game, direction, ressenti) et WFS-18 (`meteo.*`). Pas de 3e ticket : WFS-80 attend WFS-75, WFS-81 attend la fin de WFS-18. Plans : WFS-78 = CAMetalLayer + surface, étapes de chargement réparties et horodatées ; WFS-79 = RemoteIO mono flottant 48 kHz, session ambiante.

### 2026-11-04 12:42:55 — WFS-79 fusionné par confiance : son iOS par Core Audio (Claude, session trafic, agent WFS-79)
**Fait** (PR #63, fusion c903880) : `src/audio/audio_out_ios.cpp` (API C d'AudioToolbox, sous `#if defined(__APPLE__)`) : unité RemoteIO, flux mono flottant 32 bits à 48 kHz, rappel → `fill(out, frames)`, `audioOutStart` renvoie la fréquence obtenue ou 0 (message d'erreur, jeu sans son). Session « ambiante » par le runtime Objective-C (`AVAudioSession setCategory:`). Journal : `audio iOS : 48000 Hz`, puis `audio iOS : n rappels` au bout de 10 s. Arrière-plan : on laisse tourner (RG-03, pas de branchement simple sans toucher `ios_main.mm`). Bouchon du banc de `audio_out_android.cpp` : `#else` → `#elif !defined(__APPLE__)`.
**Accroche annoncée** (E7, 1 ligne) : `ios/ios.cmake`, `target_link_libraries(WaForSpeed "-framework AudioToolbox" "-framework AVFAudio")`, ajoutée après la liste des frameworks ; reste du fichier et `ios_main.mm` non touchés (WFS-78).
**Tests** : Linux, compilation complète, `test_audio` et `test_systems` OK. Suivi du run `ios.yml` en cours.

### 2026-11-04 12:43:22 — WFS-77 : captures contrôlées après fusion ; CI de main verte après WFS-74 et WFS-76 (Claude, agent 5)
- **WFS-77** (PR #57, fusionnée sur ordre de l'utilisateur) : CI de la branche verte (run 41794942515). `vitrines-planche` : 20 voitures en Turquoise, noms lisibles, bon sens, roues au sol ; journal du banc : `planche : <id> roues 4/4, peinture 1 matériau` pour les 20 ; `gt86.wfm : 4 roues séparées (rayon 0.313 m)`. `vitrine-giulia-0` : rouge uni, plus de camouflage (restent des reflets clairs du ciel et des immeubles sur les portières, comme les autres voitures métallisées : pas un motif).
- **CI de `main`** : run 41792785110 (WFS-74) et run 41793804878 (WFS-76) verts (android, 6 bancs, publication).

### 2026-11-04 12:46:11 — Début WFS-82 [STABLE] Performances téléphone : masquage (Claude, agent 4) — attribué par le PO
- **Pourquoi ce ticket** : aucun ticket ⏳ Prêt attribuable (WFS-81 attend la fin de WFS-18, E2 occupée ; WFS-80 attend WFS-75). Le banc de WFS-15 (nuit, ville) mesure 346 appels de dessin en Basse et 607 en Moyenne, au-dessus de la cible ≤ 300 de WFS-67 : la fluidité sur téléphone est la priorité de la version stable. Le PO crée WFS-82 (E4, P2 Haute) pour finir les points non faits de WFS-15 (voitures hors champ, petits décors au loin) **dans le renderer**, sans toucher E2.
- **Épic** : E4 (libre). **Zone autorisée** : `src/render/**` (sauf `ui_*`), nouveaux `src/render/masquage.{h,cpp}`, `tests/test_masquage*.cpp` ; accroche CMake ≤ 10 lignes et éventuelle accroche trafic ≤ 10 lignes annoncées au Journal. Interdit : `src/world/**` (WFS-18 en cours).
- **Critères** : Basse ≤ 300, Moyenne ≤ 450, Haute sans hausse (avant/après, ms, triangles au Journal) ; rien ne manque à moins de 150 m sur les captures ; phares et feux du trafic attachés à leur voiture ; test CPU `masquage` vert ; aucune régression.
- **Plan court** : sphères englobantes au chargement → test frustum par objet (scène et ombres) → coupure par distance des petits objets (200 m, 150 m en Basse) → si besoin, instanciation des objets répétés → mesures au banc.
- **Règles** : `regles-de-gestion/WFS-82.md`. Fusion par confiance > 50 % autorisée (compilation et tests CPU locaux, branche synchronisée).

### 2026-11-04 12:47:00 — WFS-83 créé et attribué à Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd) (po-scrum-master)
**Constat** : aucun ticket Prêt attribuable (WFS-81 attend la fin de WFS-18 dans E2 ; E7, E8 et E4 occupées par WFS-78, WFS-75 et WFS-82 (créé en même temps par une autre session du PO, d’où le numéro 83) ; `anno.md` sans anomalie 🆕). Épics libres : E0, E3, E4, E5, E6. Le plus gros manque visible sur le téléphone : **« Duel en ligne » est grisé dans le menu** alors que toute la logique du duel existe (WFS-21, transport en mémoire seulement).
**Création** : **WFS-83** (E3, P2 Haute) « Duel jouable entre deux téléphones sur le même Wi-Fi : écran Salon, transport réseau local, menu actif ». Règles : `regles-de-gestion/WFS-83.md` (RG-01 à RG-10). Réutilisation : `js/net/duel.js` (renderLobby, renderOpenRooms, advertise, createRoom, joinRoom, followHostMap, leaveRoom) et `index.html` `#lobby` ; `p2p.js` seulement pour le principe (PeerJS / WebRTC non portable : diffusion UDP sur le Wi-Fi à la place, sans serveur) ; natif `duel::Transport`, `Session`, `DuelSystem`, `sauvegarde`, `terrain.cpp`, `vibration_android.cpp`.
**Début WFS-83** : Claude (agent 5), branche `feature/waforspeed-83`. Zone : E3 `src/game/duel*`, `src/game/reseau_udp.*` (nouveau), `tests/**` ; **exception du PO** : nouveaux fichiers `src/ui/ecrans/salon.*` (E8 occupée par WFS-75, mais fichiers disjoints de `reglages.*`). Accroches permises, à annoncer au Journal : `src/core/ecran.h` ≤ 5 lignes, `menu.cpp` ≤ 10, `game.cpp` ≤ 10, `AndroidManifest.xml` (4 permissions), `android_main.cpp` ≤ 3 + nouveau `src/platform/reseau_android.cpp` (verrou de diffusion JNI), `CMakeLists.txt` ≤ 3, `desktop_main.cpp` ≤ 3. Interdit : `reglages.*` (WFS-75), `ios/**` et `ios_main.mm` (WFS-78), `src/world/**` (WFS-18).
**Critères** : test CPU deux `TransportUdp` en boucle locale (annonce, rejoindre, prêts, même graine, horloge ±50 ms, hostile ignoré, retrait 3 s) ; banc `menu-duel`, `salon-accueil`, `salon-hote`, `salon-pret`, `duel-reseau-course` ; APK sans Wi-Fi : note orange, aucun plantage.
**Plan court** : 1) `TransportUdp` + test CPU ; 2) écran Salon (accueil, salon) branché sur `duelSession()` ; 3) menu dégrisé, `Ecran::Salon`, chargement de la carte de l'hôte ; 4) Android (permissions, verrou) ; 5) banc et captures ; fusion par confiance si > 50 %.

### 2026-11-04 12:48:00 — WFS-75 fusionné par confiance : réglages Commandes gaucher et Vibrations (Claude, session trafic, agent WFS-75)
**Fusion** : PR #65, commit de fusion `35183c6`, fusion par confiance (règle n°1) : synchronisé avec `main`, compilation et tests CPU passés en local, banc `WFS_BANC=menu` regardé. CI de `main` à suivre.
**Fait** : deux lignes de plus au panneau des réglages, « Commandes : Droitier / Gaucher » et « Vibrations : Activées / Coupées », appliquées tout de suite (même depuis la pause) et enregistrées par la sauvegarde WFS-24 (clés `commandes=`, `vibrations=`) ou, sans elle, par le fichier de WFS-41. Mode gaucher : miroir des commandes tactiles par `ecrans::miroirX` et `ecrans::margesCommandes` (`src/ui/ecrans/reglages.h`), au dessin comme au toucher. La flèche gauche reste à gauche dans le mode flèches. La nitro est abaissée à 40 % de la hauteur en mode gaucher pour ne pas passer sous les boutons Menu et caméra du HUD (écart à « même hauteur » de RG-02, imposé par « rien sous le HUD »).
**Accroches annoncées** : `src/game/game.cpp` 9 lignes (zones des pédales et dessin des pédales passés par miroirX), `src/game/direction.cpp` 9 lignes (zone de direction, boutons flèches, volant au repos), `src/game/ressenti.cpp` 2 lignes (`vibrer()` : « vibration ignorée (réglage) »), `src/game/sauvegarde.h/.cpp` 4 lignes (champs `gaucher` et `vibrations`, indispensables pour que le réglage soit enregistré par WFS-24).
**Captures** : `reglages-commandes`, `course-gaucher` (groupe menu et partie2). Journal du banc : « gaucher, appui à gauche = gaz : 65 → 86 km/h », « vibration ignorée (réglage) : 90-40-120 ». Les captures de course droitier sont identiques à `main`. `reglages-menu`, `-pause` et `-musique` ne changent que par le panneau à 6 lignes.
**Pas fait (non prévu par les règles WFS-75)** : lignes « Direction » (glisser / flèches) et « Réduire les mouvements » du panneau. C'est au PO de décider.

### 2026-11-04 12:49:07 — Début WFS-80 attribué à Claude (session trafic) (po-scrum-master)
- Ticket : WFS-80, épic E8, P3 Moyenne (remonté de P4 : il absorbe les deux lignes de réglages laissées par WFS-70). Dépendances WFS-22 ✅, WFS-75 ✅ (PR #65).
- Décision PO : les lignes « Direction » (clé `steer`) et « Réduire les mouvements » vont dans WFS-80 (même fichier `reglages.cpp`, mêmes boutons, pas de micro-ticket). « Réduire les mouvements » est mémorisé dans le fichier de réglages de `reglages.cpp` (clé `mouvements`), sans toucher `src/game/**`.
- Zone autorisée : `src/ui/ecrans/reglages.*` ; accroches ≤ 10 lignes annoncées : `src/ui/garage.cpp`, `src/platform/manette_jeu.*` si besoin.
- Critères : lignes visibles et appliquées tout de suite, choix relus au relancement, focus manette sur pastilles et choix (A applique, B retour), captures tactiles du garage inchangées.
- Plan : 1) deux lignes dans `LIGNES` + lecture / écriture ; 2) zones de focus des choix ; 3) accroche garage ; 4) banc manette ; fusion par confiance si > 50 %.

### 2026-11-04 12:49:44 — WFS-18 fusionné par confiance (PR #66) (Claude, session trafic, agent WFS-18)
- Fait : météo dans `src/world/meteo.h` / `meteo.cpp` (nouveaux, E2). Système `pluie` (ordre 199) : port de `initRain` / `drawFx` (fx.js), min(160, L×H/9000) gouttes, 10-24 px, 900-1500 px/s × (0,7 + 0,6 p), inclinaison (−0,12 − braquage×0,25)×(0,4 + p), trait rgba(170,190,255,.2) de 1 px, dessiné dans `drawUi` avant la vignette (350) et le HUD ; nuit + ville + écran Course seulement ; 0 goutte avec « réduire les mouvements » (`reglagesRessenti`, WFS-70). Système `ciel-couvert` (ordre 960, après `Nuit` 950) : couleur des nuages de `makeSkyMaterial` (mix(mid, hor, 0,5)×0,9) mêlée au ciel à nightClouds 0,35, brouillard rapproché de (1 − 0,35×0,52).
- Accroches : **aucune** (enregistrement par `WFS_SYSTEM`, aucun fichier du noyau, de `src/render/**` ni de `renderer.h` modifié).
- Vérification locale : tests CPU de build.yml OK ; banc `WFS_BANC=nuit` : pluie active seulement en ville, `course-nuit-ville-0` et `-1` différentes (gouttes déplacées), coût 0,008 ms/image (102 gouttes, 1280x720) ; capture de jour en ville (`ressenti-choc`) sans pluie.
- Limites : la pluie est discrète (alpha 0,2 du jeu web) ; RG-05 « désactivée en qualité basse si baisse sous 60 i/s » non appliquée (coût négligeable mesuré). Option « réduire les mouvements » non vérifiée par capture.

### 2026-11-04 12:50:28 — Début WFS-81 attribué à Claude (session trafic, agent WFS-81) (po-scrum-master)
- Ticket : WFS-81 — Nuit corniche et circuit (vrais lampadaires, lumières alignées, double halo trafic) · épic E2 (libre depuis la fusion de WFS-18, PR #66) · branche `feature/waforspeed-81` · règles `regles-de-gestion/WFS-81.md`.
- Zone autorisée : `src/world/plage.cpp`, `src/world/circuit.cpp`, `src/world/decor_carte.*`, banc `cartes_banc.cpp` ; accroche ≤ 10 lignes dans `src/game/traffic_system.cpp` (E3) seulement si le double halo se voit, annoncée au Journal. Interdit : `src/world/meteo.*`, `src/render/**` (dont `nuit.cpp`).
- Critères : poteaux blancs à boule sous chaque lumière de la corniche ; lumière sous chaque mât du circuit (compte au journal) ; un seul halo rouge par feu sur une voiture du trafic qui freine la nuit.
- Plan : porter les lignes `lamp(…, 'beach'/'flood')` de `js/world/decor.js` et les positions de `js/world/scene.js` l. 270-282 ; style beach dans `CarteDecor::lampe` ; inscription dans `lampadaires()` comme `ville.cpp` ; contrôle du double halo sur les captures `nuit-*` de `main`.
- En parallèle (zones séparées) : WFS-78 (E7), WFS-79 (src/audio), WFS-80 (E8), WFS-82 (E4).
