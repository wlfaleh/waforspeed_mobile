---
name: waforspeed-natif
description: Règles du moteur natif de WaForSpeed Mobile (C++20 + Vulkan, Android puis iOS/consoles, aucun moteur de jeu tiers). À utiliser pour toute modification du code de ce dépôt — rendu Vulkan, shaders, voitures (.wfm), piste, conduite, trafic, commandes tactiles, threads, mémoire, construction de l'APK, tests et captures — et pour suivre le processus de travail (TACHES.md, livraisons de 5 fonctionnalités).
---

# WaForSpeed natif — règles du moteur

Jeu de course de rue en 3D, version native du jeu web (`wlfaleh/waaps`, dossier `waforspeed/`).
**Pas de Unity, Godot, Unreal ni autre moteur de jeu** : moteur maison. Petites bibliothèques libres autorisées
(stb ; plus tard SDL, Jolt… si utile), toujours copiées dans `third_party/` avec leur licence.

## Processus de travail (obligatoire) — style Jira, voir `TACHES.md`
0. **Statuts toujours à jour** : après chaque avancement (réservation, construction, PR, verdict QA, fusion), mettre à jour
   `TACHES.md` et l'envoyer sur `main` immédiatement, pour que les autres agents travaillent sans conflit.
1. **Tickets** `WFS-XX` dans `TACHES.md` (statut, responsable, branche, PR, critères d'acceptation).
2. **Se faire attribuer un ticket par l'agent `po-scrum-master`** avant de créer une branche (il l'inscrit dans `TACHES.md`
   sur `main`). Un ticket en cours par épic, un par agent ; ne modifier que la **zone de code de l'épic** du ticket.
3. **Une branche par ticket** : `feature/waforspeed-XX` (2 chiffres). Ne modifier que les fichiers annoncés.
4. **Synchroniser avec `main` avant chaque validation et avant chaque demande de fusion** (`git fetch origin && git merge origin/main`),
   puis relancer tests / CI / captures.
5. **PR** `WFS-XX : <titre>` vers `main`, statut 🔍. La CI publie l'APK du ticket dans la Release `apercu-waforspeed-XX`.
6. **QA après chaque APK** : l'agent `qa-lead` regarde les captures et rend un verdict.
   **Ne fusionner qu'avec son verdict `VALIDÉ`** (`qa/rapports/WFS-XX.md`). Puis statut ✅ et entrée « Fin » au Journal.
7. **Ne jamais supprimer de branche** (locale ou distante), même fusionnée : l'utilisateur s'en occupe.
8. **Journal** à chaque étape : fait / fichiers / vérification / limites. Ne jamais annoncer comme vérifié ce qui ne l'est pas.
9. Livraison = 5 tickets terminés (APK de `main` dans la Release `apercu`). Langue : français partout.

## Architecture
Voir `references/architecture.md`. En bref :
- `src/core` — `Jobs` (pool de threads + `parallelFor`), `Arena`/`Pool` (mémoire), `math.h`, `log.h`, `platform.h`.
- `src/render` — `VkCtx` (instance, appareil, swapchain, utilitaires), `Renderer` (passe HDR MSAA → passe finale tonemapping + UI), `model_loader`.
- `src/game` — `Track` (port exact de track.js), `Player` (port de update.js), `Game` (route par morceaux, caméra, tactile).
- `src/platform` — `android_main.cpp` (NativeActivity), `desktop_main.cpp` (banc d'essai hors écran → PNG).

## Ajouter un module (systèmes enfichables, WFS-30)
Tout nouveau module (ville, météo, sons, interface, HUD…) est un **système** : une classe dérivée de `wfs::System`
(`src/core/system.h`) dans **son propre fichier** de la zone de son épic, enregistrée par `WFS_SYSTEM(Classe, ordre)`.
- Aucun fichier du noyau à modifier : CMake compile tous les `.cpp` de `src/core|render|game|world|ui|audio`,
  et `Game` crée tous les systèmes enregistrés (ordre croissant).
- Méthodes : `init` (ressources), `update` (pas fixe 1/120 s), `render` (3D, après la route), `drawUi` (2D),
  `onTouch` (renvoyer true pour garder le toucher), `shutdown` (libérer ses ressources GPU).
- Lecture seule de la partie via `GameView` : piste, joueur, voiture, carte, caméra (vue / projection), temps, taille d'écran.
  Ne pas modifier le joueur ni reconstruire la piste depuis un système.
- Ordres conseillés : 100-199 monde (E2), 200-299 gameplay (E3), 300-399 rendu (E4), 400-499 interface (E5), 500-599 audio (E6).



### 1. Fidélité au jeu web (CRITIQUE)
- Même piste, mêmes graines (`Mulberry32` identique à `mulberry32`), mêmes unités (1 unité = 6 mm, `SEG` = 200, `RW` = 7,5 m).
- Toute fonction portée garde le nom de la fonction JS d'origine en commentaire et est vérifiée par un test (`tests/`).
- Les valeurs de réglage (PHYS, carPerf, cartes, trafic) se recopient du JS, pas de « à peu près ».

### 2. Threads et mémoire (CRITIQUE)
- Le thread principal possède Vulkan et l'état du jeu. Les threads de `jobs()` ne font que du calcul CPU sur des données qu'ils possèdent
  ou qu'ils lisent seulement (ex. : `Track::pose()` doit être pré-calculé sur le thread principal avant un `parallelFor`).
- Pas d'allocation dans la boucle de course quand on peut l'éviter : `Arena` pour les données d'une image, `Pool` pour les objets nombreux.
- Une ressource GPU encore utilisée par une image en vol ne se détruit pas tout de suite : file de destruction différée (`Game::retire`).

### 3. Rendu Vulkan
- Profondeur **inversée** (1 = près, 0 = loin, test `GREATER_OR_EQUAL`), plan lointain à l'infini.
- Scène en HDR `R16G16B16A16_SFLOAT` avec MSAA (mémoire transitoire « lazy » sur mobile), puis passe finale ACES.
- **Pré-rotation Android** : la swapchain est dans l'orientation native ; on dessine en taille logique (paysage) et la passe finale
  tourne l'image (`Renderer::rotationMatrix`). L'UI utilise la même rotation. Les touches arrivent déjà en coordonnées logiques.
- Matériaux : drapeaux communs C++/shaders/.wfm (`MatFlags`). Peinture = `MAT_PAINT` (couleur poussée par la partie, vernis dans le shader).
- Surfaces procédurales (route, trottoir, sol) : uv en MÈTRES, dessin dans `scene.frag`.
- Shaders GLSL → SPIR-V par `glslc` à la construction (`-mfmt=num`, inclus via `*.inc`). Aucun shader compilé à la main.

### 4. Performances mobiles
- Budget : 60 i/s sur un téléphone milieu de gamme ; triangles par voiture ≤ 120 k (joueur), trafic en LOD (à venir).
- Mises à jour par pas fixe (1/120 s pour la conduite), rendu à la fréquence de l'écran (FIFO).
- Regrouper par matériau : un appel de dessin par matériau et par modèle.

### 5. Contenu et licences
- Voitures : convertir depuis le jeu web avec `tools/convert_car.mjs` (voir `references/ajouter-une-voiture.md`).
- Copier la licence de chaque modèle dans `assets/licences/` et le crédit dans `CARS[]` (`src/game/cars.h`).
- Ne jamais reprendre logo, nom ou visuel d'un jeu existant.

## Construire, tester, livrer
Voir `references/construction-et-tests.md`. Points clés :
- Ici (conteneur Claude), on peut compiler le C++ (`g++ -fsyntax-only` avec les en-têtes Vulkan) et lancer les tests CPU (`test_track`).
- Les shaders, le banc d'essai Vulkan (lavapipe) et l'APK sont construits par GitHub Actions (`.github/workflows/build.yml`).
- Après chaque envoi : vérifier la CI, lire les erreurs, regarder les captures publiées dans la Release `apercu`.
