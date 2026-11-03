# WaForSpeed Mobile — journal

Journal du projet (tickets WFS-XX de `TACHES.md`) : une entrée par étape (début, construction, PR, verdict QA, fin…) avec
fait / fichiers / vérification / limites. **Nouvelles entrées à la fin du fichier.** Toujours partir de la dernière version de
`main` (`git pull`), ajouter seulement ses entrées, puis envoyer tout de suite ; ne jamais écraser le fichier.

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
  `chargement-fondu` (ville visible à 50 %), `chargement-erreur` (message rouge, pas de plantage), journal (durée par étape et
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
**Parallélisme (épics attribuables maintenant)** : E7 WFS-29 [MVP] ; E3 WFS-33 / WFS-24 ; E2 WFS-14 / WFS-43 / WFS-19 ;
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
