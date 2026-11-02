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
| F2 | Noyau du moteur : boucle de jeu, threads (système de tâches), mémoire (arènes, pools), journal | 🔧 |
| F3 | Rendu Vulkan PBR : soleil, ciel, HDR + tonemapping, MSAA | ⏳ |
| F4 | Vraies voitures importées du jeu web (Golf 8 GTI, BMW M4, Porsche 911, Lamborghini Gallardo) | ⏳ |
| F5 | Route générée + conduite tactile + caméra de poursuite (physique reprise du jeu web) | ⏳ |

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
