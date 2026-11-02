# WaForSpeed Mobile — tableau des tâches

Version native (C++20, Vulkan) du jeu WaForSpeed, pour Android puis iOS, portable sur consoles. Moteur maison, aucun moteur
de jeu tiers. Le contenu vient du jeu web `wlfaleh/waaps` (dossier `waforspeed/`). Organisation type Jira / Scrum :
**épics** (une zone de code chacune), **tickets** WFS-XX priorisés, plusieurs agents en parallèle, en autonomie.

## Rôles
| Rôle | Agent | Ce qu'il fait |
|---|---|---|
| PO / Scrum Master | `po-scrum-master` (`.claude/agents/po-scrum-master.md`) | étudie le besoin, découpe en épics et tickets, priorise, **attribue les tickets**, débloque |
| Développeur | toute session Claude | réalise le ticket attribué, sur sa branche, dans la zone de son épic |
| QA lead | `qa-lead` (`.claude/agents/qa-lead.md`) | regarde les captures de chaque APK, verdict VALIDÉ / REFUSÉ |

## Statuts
| Statut | Signification |
|---|---|
| ⏳ En attente | dépendances pas encore terminées |
| ⏳ Prêt | dépendances terminées, critères clairs : peut être attribué |
| 🔧 En cours | attribué (Responsable), branche créée |
| 🔍 En revue | PR ouverte, CI verte, attente du verdict QA |
| ✅ Terminé | VALIDÉ et fusionné dans `main` |
| ⛔ Bloqué | raison au Journal |

## Règles de travail en parallèle (obligatoires)
1. **Attribution par le PO** : avant de créer une branche, un agent développeur **demande un ticket à l'agent
   `po-scrum-master`**. Le PO choisit le ticket `⏳ Prêt` le plus prioritaire **dont l'épic n'a aucun ticket en cours**,
   l'écrit dans ce fichier (🔧, Responsable, Branche, entrée « Début » au Journal) et l'envoie sur `main`. Personne ne
   se réserve un ticket seul.
2. **Un ticket en cours maximum par épic** et **un ticket maximum par agent** : chaque épic a sa zone de code, donc deux
   agents ne modifient jamais les mêmes fichiers.
3. **Zone de code** : un ticket ne modifie que les fichiers de la zone de son épic (tableau des épics). Exception unique :
   une « ligne d'accroche » (≤ 10 lignes, ex. enregistrer un système dans `Game`) annoncée au Journal.
   Tout autre changement du noyau passe par un ticket de l'épic E0.
4. **Branche** `feature/waforspeed-XX` par ticket (même si la session propose `claude/…`). Synchroniser avec `main` avant
   chaque validation et avant chaque PR.
5. **QA** : après chaque APK, le développeur **lance l'agent `qa-lead`**, qui regarde les captures et écrit
   `qa/rapports/WFS-XX.md`. Le développeur ne fait jamais la QA de son propre ticket. Fusion seulement si VALIDÉ.
6. **Statuts à jour** après chaque avancement, envoyés sur `main` tout de suite, en modifiant seulement ses lignes à partir
   de la dernière version de `main` (ne jamais écraser le fichier).
7. **Fin de ticket** : ✅, Responsable libéré, entrée « Fin » au Journal ; le PO passe en `⏳ Prêt` les tickets débloqués.
8. **Ne jamais supprimer de branche** (gérées par l'utilisateur).
9. Une réservation sans nouvelle depuis 24 h peut être réattribuée par le PO (noté au Journal).

**Définition de « Prêt »** : dépendances terminées, critères d'acceptation vérifiables sur captures, zone de code connue,
taille ≤ 1 journée (sinon le PO découpe).
**Définition de « Fini »** : critères cochés, CI verte (banc d'essai, APK, émulateur), verdict QA VALIDÉ, synchronisé, fusionné,
Journal à jour.

## Épics et zones de code
| Épic | Nom | Zone de code (fichiers modifiables) | But |
|---|---|---|---|
| E0 | Noyau partagé (catalyseur) | `src/core/**`, `src/game/game.*`, `src/render/renderer.h` (API), `CMakeLists.txt` | Stabiliser les points d'extension pour que les autres épics ajoutent des fichiers sans toucher au noyau. |
| E1 | Socle moteur (livraison 1) | — | Terminé. |
| E2 | Monde et décor | `src/world/**`, `assets/world/**`, `shaders/world_*` | Ville, cartes, météo, piétons : tout ce qui entoure la route. |
| E3 | Gameplay et simulation | `src/game/` sauf `game.*` (trafic, course, duel, sauvegarde), `tests/**` | Règles du jeu portées du jeu web, tests de fidélité. |
| E4 | Rendu et performances | `src/render/**` (sauf `ui_*`), `shaders/scene*`, `sky*`, `post*`, `shadow*` | Ombres, nuit, reflets, post-traitement, qualité, LOD. |
| E5 | Interface et contenu | `src/ui/**`, `shaders/ui*`, `assets/fonts/**`, `assets/cars/**`, `tools/convert_car.mjs` | Texte, menus, garage, HUD, voitures et peintures. |
| E6 | Audio | `src/audio/**`, `assets/audio/**` | Moteur, bruitages. |
| E7 | Plateformes et publication | `src/platform/**`, `android/**`, `ios/**`, `.github/**`, `tools/qa/**` | Android, iOS, manettes, stores, CI. |

## Tableau (par ordre de priorité)
| Prio | Clé | Titre | Épic | Statut | Responsable | Dépend de | Branche | PR | QA |
|---|---|---|---|---|---|---|---|---|---|
| — | WFS-01 | Projet C++ et construction automatique de l'APK | E1 | ✅ Terminé |  | — | `feature/waforspeed-01`/`05` | #1 | ✅ VALIDÉ |
| — | WFS-02 | Noyau : boucle, threads, mémoire, journal | E1 | ✅ Terminé |  | — | `feature/waforspeed-01`/`05` | #1 | ✅ VALIDÉ |
| — | WFS-03 | Rendu Vulkan PBR, HDR, tonemapping, MSAA | E1 | ✅ Terminé |  | — | `feature/waforspeed-01`/`05` | #1 | ✅ VALIDÉ |
| — | WFS-04 | Vraies voitures du jeu web (4 modèles) | E1 | ✅ Terminé |  | — | `feature/waforspeed-01`/`05` | #1 | ✅ VALIDÉ |
| — | WFS-05 | Route, conduite tactile, caméra (+ retours QA) | E1 | ✅ Terminé |  | — | `feature/waforspeed-01`/`05` | #1, #2 | ✅ VALIDÉ |
| 1 | WFS-30 | Points d'extension du moteur (systèmes enfichables, CMake par dossier) | E0 | ✅ Terminé | — | — | `feature/waforspeed-30` | #3 | ✅ VALIDÉ par qa-lead |
| 2 | WFS-08 | Trafic multi-thread et collisions | E3 | 🔧 En cours | Claude (session trafic) | — | `feature/waforspeed-08` |  |  |
| 3 | WFS-06 | Ville : bâtiments, trottoirs, lampadaires, arbres | E2 | 🔧 En cours | Claude (session moteur-natif), 2026-11-02 | WFS-30 | `feature/waforspeed-06` |  | ⏳ construction en cours |
| 4 | WFS-26 | Rendu de texte (police, nombres, libellés) | E5 | ⏳ Prêt |  | WFS-30 | `feature/waforspeed-26` |  |  |
| 5 | WFS-10 | Sons de moteur (AAudio, régime et rapports) | E6 | ⏳ Prêt |  | WFS-30 | `feature/waforspeed-10` |  |  |
| 6 | WFS-07 | Ombres dynamiques du soleil | E4 | ⏳ Prêt |  | WFS-30 | `feature/waforspeed-07` |  |  |
| 7 | WFS-12 | Règles de course : chrono, checkpoints, score, nitro, quasi-contacts | E3 | ⏳ En attente |  | WFS-08 | `feature/waforspeed-12` |  |  |
| 8 | WFS-11 | Menu et garage 3D (voiture + couleur) | E5 | ⏳ En attente |  | WFS-26 | `feature/waforspeed-11` |  |  |
| 9 | WFS-09 | Nuit : vrais phares et éclairage public | E4 | ⏳ En attente |  | WFS-07 | `feature/waforspeed-09` |  |  |
| 10 | WFS-27 | HUD de course (vitesse, chrono, score, nitro en texte) | E5 | ⏳ En attente |  | WFS-26, WFS-12 | `feature/waforspeed-27` |  |  |
| 11 | WFS-29 | Android : mode plein écran immersif, icône, nom | E7 | ⏳ Prêt |  | — | `feature/waforspeed-29` |  |  |
| 12 | WFS-14 | Cartes campagne, plage, circuit | E2 | ⏳ En attente |  | WFS-06 | `feature/waforspeed-14` |  |  |
| 13 | WFS-13 | Les 20 voitures et 16 peintures | E5 | ⏳ En attente |  | WFS-11 | `feature/waforspeed-13` |  |  |
| 14 | WFS-15 | Réglages de qualité selon le téléphone | E4 | ⏳ En attente |  | WFS-07 | `feature/waforspeed-15` |  |  |
| 15 | WFS-28 | Bruitages : chocs, nitro, crissements, ambiance | E6 | ⏳ En attente |  | WFS-10, WFS-08 | `feature/waforspeed-28` |  |  |
| 16 | WFS-20 | Performances : LOD, tri des objets visibles en parallèle | E4 | ⏳ En attente |  | WFS-06, WFS-08 | `feature/waforspeed-20` |  |  |
| 17 | WFS-19 | Piétons animés et conducteurs | E2 | ⏳ En attente |  | WFS-06 | `feature/waforspeed-19` |  |  |
| 18 | WFS-16 | Route mouillée avec reflets | E4 | ⏳ En attente |  | WFS-09 | `feature/waforspeed-16` |  |  |
| 19 | WFS-18 | Météo : pluie, flaques | E2 | ⏳ En attente |  | WFS-16 | `feature/waforspeed-18` |  |  |
| 20 | WFS-17 | Post-traitement : bloom, TAA, flou de mouvement | E4 | ⏳ En attente |  | WFS-15 | `feature/waforspeed-17` |  |  |
| 21 | WFS-24 | Sauvegarde et progression | E3 | ⏳ En attente |  | WFS-12 | `feature/waforspeed-24` |  |  |
| 22 | WFS-21 | Duel en ligne | E3 | ⏳ En attente |  | WFS-12 | `feature/waforspeed-21` |  |  |
| 23 | WFS-23 | Manettes de jeu | E7 | ⏳ En attente |  | WFS-29 | `feature/waforspeed-23` |  |  |
| 24 | WFS-22 | Version iOS (Metal via MoltenVK) | E7 | ⏳ En attente |  | WFS-29 | `feature/waforspeed-22` |  |  |
| 25 | WFS-25 | Préparation des stores | E7 | ⏳ En attente |  | WFS-22 | `feature/waforspeed-25` |  |  |

## Critères d'acceptation des tickets ouverts

**WFS-30 — Points d'extension du moteur (systèmes enfichables, CMake par dossier)** · E0 · priorité 1 · ✅ Terminé
- [ ] Interface `System` (init / update / render / interface) enregistrée dans `Game` : un nouveau module = un nouveau fichier + 1 ligne d'enregistrement
- [ ] CMake ajoute automatiquement les fichiers de `src/world`, `src/ui`, `src/audio`, `src/game`
- [ ] Accès en lecture pour les systèmes : piste, joueur, caméra, renderer ; doc dans le skill `waforspeed-natif`
- [ ] Aucun changement visible ; CI verte, captures identiques

**WFS-08 — Trafic multi-thread et collisions** · E3 · priorité 2 · 🔧 En cours
- [ ] Port de traffic.js (même graine)
- [ ] Mise à jour sur les threads de travail
- [ ] Collisions et quasi-contacts comme le jeu web

**WFS-06 — Ville : bâtiments, trottoirs, lampadaires, arbres** · E2 · priorité 3 · 🔧 En cours
- [ ] Décor généré par la graine de la carte (comme decor.js)
- [ ] Instanciation GPU, ≤ 300 appels de dessin
- [ ] Captures : rue bordée d'immeubles, lampadaires, arbres

**WFS-26 — Rendu de texte (police, nombres, libellés)** · E5 · priorité 4 · ⏳ Prêt
- [ ] Police (licence libre) en atlas, texte en pixels logiques, accents français
- [ ] Lisible sur 2400x1080 et petits écrans
- [ ] Capture : texte d'essai net

**WFS-10 — Sons de moteur (AAudio, régime et rapports)** · E6 · priorité 5 · ⏳ Prêt
- [ ] Sons du jeu web, régime qui suit l'accélérateur et les rapports
- [ ] Pas de craquement, coupure en pause

**WFS-07 — Ombres dynamiques du soleil** · E4 · priorité 6 · ⏳ Prêt
- [ ] Ombres stables sans scintillement
- [ ] Coût ≤ 3 ms sur mobile

**WFS-12 — Règles de course : chrono, checkpoints, score, nitro, quasi-contacts** · E3 · priorité 7 · ⏳ En attente
- [ ] Règles identiques au mode solo web (temps, checkpoints, score, combos, nitro)
- [ ] Tests automatiques de fidélité

**WFS-11 — Menu et garage 3D (voiture + couleur)** · E5 · priorité 8 · ⏳ En attente
- [ ] Choix des voitures et peintures
- [ ] Style design-street-racing

**WFS-09 — Nuit : vrais phares et éclairage public** · E4 · priorité 9 · ⏳ En attente
- [ ] Phares réalistes (leçons de la version web)
- [ ] Lampadaires, mode nuit par carte

**WFS-27 — HUD de course (vitesse, chrono, score, nitro en texte)** · E5 · priorité 10 · ⏳ En attente
- [ ] Vitesse, chrono, score, nitro lisibles
- [ ] Style design-street-racing

**WFS-29 — Android : mode plein écran immersif, icône, nom** · E7 · priorité 11 · ⏳ Prêt
- [ ] Barres système masquées pendant la course
- [ ] Icône et nom WaForSpeed

**WFS-14 — Cartes campagne, plage, circuit** · E2 · priorité 12 · ⏳ En attente
- [ ] Décors et ambiances par carte

**WFS-13 — Les 20 voitures et 16 peintures** · E5 · priorité 13 · ⏳ En attente
- [ ] Toutes converties et vérifiées en vitrine
- [ ] Taille de l'APK maîtrisée

**WFS-15 — Réglages de qualité selon le téléphone** · E4 · priorité 14 · ⏳ En attente
- [ ] Niveaux bas / moyen / haut, choix automatique

**WFS-28 — Bruitages : chocs, nitro, crissements, ambiance** · E6 · priorité 15 · ⏳ En attente
- [ ] Bruitages synchronisés aux évènements du jeu

**WFS-20 — Performances : LOD, tri des objets visibles en parallèle** · E4 · priorité 16 · ⏳ En attente
- [ ] 60 i/s visés, mesures dans le journal

**WFS-19 — Piétons animés et conducteurs** · E2 · priorité 17 · ⏳ En attente
- [ ] Personnages du jeu web, conducteur face au volant

**WFS-16 — Route mouillée avec reflets** · E4 · priorité 18 · ⏳ En attente
- [ ] Reflets plausibles, coût maîtrisé

**WFS-18 — Météo : pluie, flaques** · E2 · priorité 19 · ⏳ En attente
- [ ] Pluie animée, route mouillée

**WFS-17 — Post-traitement : bloom, TAA, flou de mouvement** · E4 · priorité 20 · ⏳ En attente
- [ ] Activable selon la qualité

**WFS-24 — Sauvegarde et progression** · E3 · priorité 21 · ⏳ En attente
- [ ] Meilleurs scores, voitures et réglages conservés

**WFS-21 — Duel en ligne** · E3 · priorité 22 · ⏳ En attente
- [ ] Compatible avec le principe du duel web

**WFS-23 — Manettes de jeu** · E7 · priorité 23 · ⏳ En attente
- [ ] Manettes Bluetooth Android

**WFS-22 — Version iOS (Metal via MoltenVK)** · E7 · priorité 24 · ⏳ En attente
- [ ] Construction iOS, test TestFlight

**WFS-25 — Préparation des stores** · E7 · priorité 25 · ⏳ En attente
- [ ] Icônes, signature, fiches

> WFS-01 à WFS-05 (E1) : critères détaillés et rapports dans `qa/rapports/` et dans le Journal.

## Journal

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
