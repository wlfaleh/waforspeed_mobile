# WaForSpeed Mobile — tableau des tâches

Version native (C++20, Vulkan) du jeu WaForSpeed, pour Android puis iOS, conçue pour pouvoir être portée sur consoles.
Aucun moteur de jeu tiers : moteur maison. Le contenu (voitures 3D, sons, règles, cartes, physique) vient du jeu web
`wlfaleh/waaps` (dossier `waforspeed/`). Organisation inspirée de Jira : tickets numérotés, une branche par ticket,
revue et validation avant fusion.

## Statuts
| Statut | Signification |
|---|---|
| ⏳ À faire | personne ne travaille dessus |
| 🔧 En cours | réservé (Responsable renseigné), branche créée, développement en cours |
| 🔍 En revue | demande de fusion (PR) ouverte, CI verte, en attente du verdict de l'agent QA (`qa-lead`) |
| ✅ Terminé | validé et fusionné dans `main` |
| ⛔ Bloqué | en attente d'une décision ou d'un autre ticket (préciser dans le journal) |

## Processus (obligatoire pour chaque ticket)
0. **Le tableau reflète toujours l'état réel.** Après CHAQUE avancement (réservation, branche créée, construction
   réussie ou en échec, PR ouverte, verdict QA, correction, fusion), mettre à jour le statut, la colonne QA et le Journal,
   puis envoyer `TACHES.md` sur `main` immédiatement : c'est ce que lisent les autres agents pour éviter les conflits.
1. **Réserver** : `git checkout main && git pull`, vérifier que le ticket n'a pas de Responsable, puis dans ce fichier :
   statut 🔧, Responsable, Branche ; entrée « Début » dans le Journal (plan, fichiers prévus).
   Envoyer ce changement sur `main` **immédiatement**, avant d'écrire du code.
2. **Branche** : `git checkout -b feature/waforspeed-XX` (XX = numéro du ticket sur 2 chiffres, ex. `feature/waforspeed-06`).
   Une branche = un ticket. Ne modifier que les fichiers annoncés (sinon le noter au Journal).
3. **Synchroniser avant chaque validation** : `git fetch origin && git merge origin/main` (ou rebase), corriger les conflits,
   puis relancer les vérifications (tests, CI, captures).
4. **Demande de fusion** : synchroniser à nouveau avec `main`, ouvrir la PR `WFS-XX : <titre>` vers `main`, statut 🔍,
   lien de la PR dans le tableau. La CI publie l'APK du ticket dans la Release `apercu-waforspeed-XX`.
5. **Valider (QA)** : après chaque APK de la branche, l'agent **`qa-lead`** regarde les captures (émulateur Android +
   banc d'essai) et rend un verdict dans `qa/rapports/WFS-XX.md`. **Ne jamais fusionner sans verdict `VALIDÉ`.**
6. **Fusionner** : synchroniser une dernière fois, fusionner la PR, statut ✅, Responsable libéré, entrée « Fin » au Journal
   (fait / fichiers / vérification / limites).
7. Livraison = 5 tickets terminés : APK de `main` dans la Release `apercu`, entrée « Livraison N » au Journal.
8. **Ne jamais supprimer de branche** : c'est l'utilisateur qui gère les branches (même après fusion).
9. Une réservation sans nouvelle depuis 24 h peut être reprise, en le notant au Journal.

## Tableau
| Clé | Titre | Type | Priorité | Livraison | Statut | Responsable | Branche | PR | QA |
|---|---|---|---|---|---|---|---|---|---|
| WFS-01 | Projet C++ et construction automatique de l'APK Android | Technique | Haute | L1 | 🔍 En revue | Claude (moteur-natif) | `feature/waforspeed-01` (lot WFS-01→05) | #1 | 🔧 QA en attente : construction avec 1er test sur émulateur en cours |
| WFS-02 | Noyau du moteur : boucle, threads, mémoire, journal | Technique | Haute | L1 | 🔍 En revue | Claude (moteur-natif) | `feature/waforspeed-01` (lot WFS-01→05) | #1 | 🔧 QA en attente : construction avec 1er test sur émulateur en cours |
| WFS-03 | Rendu Vulkan PBR : soleil, ciel, HDR, tonemapping, MSAA | Fonctionnalité | Haute | L1 | 🔍 En revue | Claude (moteur-natif) | `feature/waforspeed-01` (lot WFS-01→05) | #1 | 🔧 QA en attente : construction avec 1er test sur émulateur en cours |
| WFS-04 | Vraies voitures du jeu web (Golf, M4, 911, Gallardo) | Fonctionnalité | Haute | L1 | 🔍 En revue | Claude (moteur-natif) | `feature/waforspeed-01` (lot WFS-01→05) | #1 | 🔧 QA en attente : construction avec 1er test sur émulateur en cours |
| WFS-05 | Route générée, conduite tactile, caméra de poursuite | Fonctionnalité | Haute | L1 | 🔍 En revue | Claude (moteur-natif) | `feature/waforspeed-01` (lot WFS-01→05) | #1 | 🔧 QA en attente : construction avec 1er test sur émulateur en cours |
| WFS-06 | Ville : bâtiments, trottoirs, lampadaires, arbres | Fonctionnalité | Haute | L2 | ⏳ À faire |  | `feature/waforspeed-06` |  | |
| WFS-07 | Ombres dynamiques du soleil (cascades) | Fonctionnalité | Moyenne | L2 | ⏳ À faire |  | `feature/waforspeed-07` |  | |
| WFS-08 | Trafic multi-thread et collisions | Fonctionnalité | Haute | L2 | ⏳ À faire |  | `feature/waforspeed-08` |  | |
| WFS-09 | Nuit : vrais phares et éclairage public | Fonctionnalité | Moyenne | L2 | ⏳ À faire |  | `feature/waforspeed-09` |  | |
| WFS-10 | Sons de moteur (AAudio, régime et rapports) | Fonctionnalité | Moyenne | L2 | ⏳ À faire |  | `feature/waforspeed-10` |  | |
| WFS-11 | Menu et garage 3D (voiture + couleur) | Fonctionnalité | Haute | L3 | ⏳ À faire |  | `feature/waforspeed-11` |  | |
| WFS-12 | Chrono, checkpoints, score, nitro, HUD avec texte | Fonctionnalité | Haute | L3 | ⏳ À faire |  | `feature/waforspeed-12` |  | |
| WFS-13 | Les 20 voitures et 16 peintures | Fonctionnalité | Moyenne | L3 | ⏳ À faire |  | `feature/waforspeed-13` |  | |
| WFS-14 | Cartes campagne, plage, circuit | Fonctionnalité | Moyenne | L3 | ⏳ À faire |  | `feature/waforspeed-14` |  | |
| WFS-15 | Réglages de qualité selon le téléphone | Technique | Moyenne | L3 | ⏳ À faire |  | `feature/waforspeed-15` |  | |
| WFS-16 | Route mouillée avec reflets | Fonctionnalité | Basse | L4 | ⏳ À faire |  | `feature/waforspeed-16` |  | |
| WFS-17 | Post-traitement : bloom, TAA, flou de mouvement | Fonctionnalité | Basse | L4 | ⏳ À faire |  | `feature/waforspeed-17` |  | |
| WFS-18 | Météo : pluie, flaques | Fonctionnalité | Basse | L4 | ⏳ À faire |  | `feature/waforspeed-18` |  | |
| WFS-19 | Piétons animés et conducteurs | Fonctionnalité | Basse | L4 | ⏳ À faire |  | `feature/waforspeed-19` |  | |
| WFS-20 | Performances : LOD, tri des objets visibles en parallèle | Technique | Moyenne | L4 | ⏳ À faire |  | `feature/waforspeed-20` |  | |
| WFS-21 | Duel en ligne | Fonctionnalité | Moyenne | L5 | ⏳ À faire |  | `feature/waforspeed-21` |  | |
| WFS-22 | Version iOS (Metal via MoltenVK) | Technique | Moyenne | L5 | ⏳ À faire |  | `feature/waforspeed-22` |  | |
| WFS-23 | Manettes de jeu | Fonctionnalité | Basse | L5 | ⏳ À faire |  | `feature/waforspeed-23` |  | |
| WFS-24 | Sauvegarde et progression | Fonctionnalité | Basse | L5 | ⏳ À faire |  | `feature/waforspeed-24` |  | |
| WFS-25 | Préparation des stores | Technique | Basse | L5 | ⏳ À faire |  | `feature/waforspeed-25` |  | |

## Détail des tickets (critères d'acceptation)

### Livraison 1 — « Première course »

**WFS-01 — Projet C++ et construction automatique de l'APK Android** · Technique · priorité Haute · 🔍 En revue
- [ ] CMake + Gradle (NativeActivity, arm64) construisent un APK installable
- [x] CI GitHub : banc d'essai Linux + APK + Release d'aperçu
- [ ] L'APK s'installe et démarre (émulateur Android de la QA : « QA pret », pas de plantage)

**WFS-02 — Noyau du moteur : boucle, threads, mémoire, journal** · Technique · priorité Haute · 🔍 En revue
- [ ] Système de tâches multi-thread (un thread par cœur) avec parallelFor
- [ ] Arène par image et réserve d'objets
- [ ] Boucle Android : pause hors focus, reprise propre

**WFS-03 — Rendu Vulkan PBR : soleil, ciel, HDR, tonemapping, MSAA** · Fonctionnalité · priorité Haute · 🔍 En revue
- [ ] Passe HDR + MSAA, passe finale ACES
- [ ] Peinture vernie, vitres, ciel analytique
- [ ] Image non déformée en paysage (captures de l'émulateur : commandes rondes)

**WFS-04 — Vraies voitures du jeu web (Golf, M4, 911, Gallardo)** · Fonctionnalité · priorité Haute · 🔍 En revue
- [ ] Convertisseur glTF → .wfm documenté
- [ ] 4 voitures à la bonne taille, avant vers -Z, peinture détectée
- [ ] Textures décodées en parallèle, licences dans assets/licences

**WFS-05 — Route générée, conduite tactile, caméra de poursuite** · Fonctionnalité · priorité Haute · 🔍 En revue
- [x] Piste identique au jeu web (test automatique)
- [ ] Conduite reprise de update.js (pas fixe)
- [ ] Commandes tactiles volant / frein / accélérateur / nitro : vitesse monte, nitro, freinage (logcat QA sur émulateur)

### Livraison 2 — « Ville vivante »

**WFS-06 — Ville : bâtiments, trottoirs, lampadaires, arbres** · Fonctionnalité · priorité Haute · ⏳ À faire
- [ ] Décor généré par la graine de la carte (comme decor.js)
- [ ] Instanciation GPU, ≤ 300 appels de dessin
- [ ] 60 i/s sur téléphone milieu de gamme

**WFS-07 — Ombres dynamiques du soleil (cascades)** · Fonctionnalité · priorité Moyenne · ⏳ À faire
- [ ] Ombres stables sans scintillement
- [ ] Coût ≤ 3 ms sur mobile

**WFS-08 — Trafic multi-thread et collisions** · Fonctionnalité · priorité Haute · ⏳ À faire
- [ ] Port de traffic.js (même graine)
- [ ] Mise à jour sur les threads de travail
- [ ] Collisions et quasi-contacts comme le jeu web

**WFS-09 — Nuit : vrais phares et éclairage public** · Fonctionnalité · priorité Moyenne · ⏳ À faire
- [ ] Phares réalistes (leçons de la version web)
- [ ] Lampadaires, mode nuit par carte

**WFS-10 — Sons de moteur (AAudio, régime et rapports)** · Fonctionnalité · priorité Moyenne · ⏳ À faire
- [ ] Sons du jeu web, régime suit l'accélérateur
- [ ] Latence faible, pas de craquements

### Livraison 3 — « Jeu complet »

**WFS-11 — Menu et garage 3D (voiture + couleur)** · Fonctionnalité · priorité Haute · ⏳ À faire
- [ ] Choix des voitures et peintures
- [ ] Style design-street-racing

**WFS-12 — Chrono, checkpoints, score, nitro, HUD avec texte** · Fonctionnalité · priorité Haute · ⏳ À faire
- [ ] Règles identiques au mode solo web
- [ ] Police lisible sur téléphone

**WFS-13 — Les 20 voitures et 16 peintures** · Fonctionnalité · priorité Moyenne · ⏳ À faire
- [ ] Toutes converties et vérifiées en vitrine
- [ ] Taille de l'APK maîtrisée

**WFS-14 — Cartes campagne, plage, circuit** · Fonctionnalité · priorité Moyenne · ⏳ À faire
- [ ] Décors et ambiances par carte

**WFS-15 — Réglages de qualité selon le téléphone** · Technique · priorité Moyenne · ⏳ À faire
- [ ] Niveaux bas/moyen/haut, choix automatique

### Livraison 4 — « Rendu nouvelle génération »

**WFS-16 — Route mouillée avec reflets** · Fonctionnalité · priorité Basse · ⏳ À faire
- [ ] Reflets plausibles, coût maîtrisé

**WFS-17 — Post-traitement : bloom, TAA, flou de mouvement** · Fonctionnalité · priorité Basse · ⏳ À faire
- [ ] Activable selon la qualité

**WFS-18 — Météo : pluie, flaques** · Fonctionnalité · priorité Basse · ⏳ À faire
- [ ] Pluie animée, route mouillée

**WFS-19 — Piétons animés et conducteurs** · Fonctionnalité · priorité Basse · ⏳ À faire
- [ ] Personnages du jeu web, conducteur face au volant

**WFS-20 — Performances : LOD, tri des objets visibles en parallèle** · Technique · priorité Moyenne · ⏳ À faire
- [ ] 60 i/s stables, mesures dans le journal

### Livraison 5 — « Multijoueur et iOS »

**WFS-21 — Duel en ligne** · Fonctionnalité · priorité Moyenne · ⏳ À faire
- [ ] Compatible avec le principe du duel web

**WFS-22 — Version iOS (Metal via MoltenVK)** · Technique · priorité Moyenne · ⏳ À faire
- [ ] Construction iOS, test TestFlight

**WFS-23 — Manettes de jeu** · Fonctionnalité · priorité Basse · ⏳ À faire
- [ ] Manettes Bluetooth Android

**WFS-24 — Sauvegarde et progression** · Fonctionnalité · priorité Basse · ⏳ À faire
- [ ] Meilleurs scores, voitures et réglages conservés

**WFS-25 — Préparation des stores** · Technique · priorité Basse · ⏳ À faire
- [ ] Icônes, signature de publication, fiches

> Note : WFS-01 à WFS-05 ont été développés ensemble avant la mise en place de ce processus, sur la branche
> `claude/moteur-natif`, renommée `feature/waforspeed-01`. Ils sont fusionnés en un seul lot après validation.
> À partir de WFS-06, une branche et une PR par ticket.

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
