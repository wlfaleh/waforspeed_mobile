# WaForSpeed Mobile — tableau des tâches

Version native (C++20, Vulkan) du jeu WaForSpeed, pour Android puis iOS, conçue pour pouvoir être portée
sur consoles. Aucun moteur de jeu tiers : moteur maison. Le contenu (voitures 3D, sons, règles de course,
génération des cartes, physique de conduite) est repris du jeu web `wlfaleh/waaps` (dossier `waforspeed/`).

Une **livraison** = 5 fonctionnalités terminées + un APK Android à installer (page *Releases* du dépôt).

Légende : ✅ terminé · 🔧 en cours · ⏳ à faire

## Livraison 1 — « Première course »
| # | Fonctionnalité | État |
|---|---|---|
| F1 | Projet C++ + construction automatique de l'APK Android (GitHub Actions) | 🔧 |
| F2 | Noyau du moteur : boucle de jeu, threads (système de tâches), mémoire (arènes, pools), journal | 🔧 (écrit, à valider sur téléphone) |
| F3 | Rendu Vulkan PBR : soleil, ciel, HDR + tonemapping, MSAA | 🔧 |
| F4 | Vraies voitures importées du jeu web (Golf 8 GTI, BMW M4, Porsche 911, Lamborghini Gallardo) | 🔧 |
| F5 | Route générée + conduite tactile + caméra de poursuite (physique reprise du jeu web) | 🔧 |

## Livraison 2 — « Ville vivante »
| # | Fonctionnalité | État |
|---|---|---|
| F6 | Ville : bâtiments, trottoirs, lampadaires, arbres | ⏳ |
| F7 | Ombres dynamiques du soleil (cascades) | ⏳ |
| F8 | Trafic (mis à jour sur plusieurs threads) + collisions | ⏳ |
| F9 | Nuit : vrais phares et éclairage public | ⏳ |
| F10 | Sons de moteur (AAudio, régime + rapports) | ⏳ |

## Livraison 3 — « Jeu complet »
| # | Fonctionnalité | État |
|---|---|---|
| F11 | Menu et garage 3D (choix voiture + couleur) | ⏳ |
| F12 | Chrono, checkpoints, score, nitro, HUD | ⏳ |
| F13 | Les 20 voitures + 16 peintures | ⏳ |
| F14 | Cartes campagne, plage, circuit | ⏳ |
| F15 | Réglages de qualité selon le téléphone | ⏳ |

## Livraison 4 — « Rendu nouvelle génération »
| # | Fonctionnalité | État |
|---|---|---|
| F16 | Route mouillée avec reflets | ⏳ |
| F17 | Post-traitement : bloom, anticrénelage temporel, flou de mouvement | ⏳ |
| F18 | Météo : pluie, flaques | ⏳ |
| F19 | Piétons animés et conducteurs | ⏳ |
| F20 | Performances : niveaux de détail, tri des objets visibles en parallèle | ⏳ |

## Livraison 5 — « Multijoueur et iOS »
| # | Fonctionnalité | État |
|---|---|---|
| F21 | Duel en ligne | ⏳ |
| F22 | Version iOS (Metal via MoltenVK) | ⏳ |
| F23 | Manettes de jeu | ⏳ |
| F24 | Sauvegarde et progression | ⏳ |
| F25 | Préparation des stores (icônes, signature, fiches) | ⏳ |

---

## Journal

### 2026-11-02 — Démarrage du projet natif
**Fait**
- Dépôt créé, choix techniques : C++20, Vulkan, Android NativeActivity, aucun moteur de jeu tiers.
- Tableau des tâches (5 livraisons de 5 fonctionnalités).
- Skills pour les agents : `waforspeed-natif` (règles du moteur natif, processus, construction), `design-street-racing`
  (direction artistique, reprise de la version web), `waforspeed-3d` (référence de la version navigateur). `CLAUDE.md` à la racine.

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
