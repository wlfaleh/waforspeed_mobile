# WaForSpeed Mobile — tableau des tâches

Version native (C++20, Vulkan) du jeu WaForSpeed, pour Android puis iOS, portable sur consoles. Moteur maison, aucun moteur
de jeu tiers. Le contenu vient du jeu web `wlfaleh/waaps` (dossier `waforspeed/`). Organisation type Jira / Scrum :
**épics** (une zone de code chacune), **tickets** WFS-XX priorisés, plusieurs agents en parallèle, en autonomie.

## Rôles
| Rôle | Agent | Ce qu'il fait |
|---|---|---|
| PO / Scrum Master | `po-scrum-master` (`.claude/agents/po-scrum-master.md`) | étudie le besoin, découpe en épics et tickets, priorise, **attribue les tickets**, débloque |
| Développeur | toute session Claude | réalise le ticket attribué, sur sa branche, dans la zone de son épic |
| QA lead | `qa-lead` (`.claude/agents/qa-lead.md`) | regarde les captures de chaque APK, classe les anomalies (bloquante / mineure), verdict VALIDÉ / À CORRIGER / PASSABLE AVEC ANOMALIES MINEURES / REFUSÉ |

## Statuts
| Statut | Signification |
|---|---|
| ⏳ En attente | dépendances pas encore terminées |
| ⏳ Prêt | dépendances terminées, critères clairs : peut être attribué |
| 🔧 En cours | attribué (Responsable), branche créée |
| 🔍 En revue | PR ouverte, CI verte, attente du verdict QA |
| ✅ Terminé | VALIDÉ (ou PASSABLE AVEC ANOMALIES MINEURES) et fusionné dans `main` |
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
5. **QA** : après chaque construction, le développeur **lance l'agent `qa-lead`**, qui regarde les captures du
   **banc d'essai PC** et écrit `qa/rapports/WFS-XX.md`. Le développeur ne fait jamais la QA de son propre ticket.
   **Le QA décide de la gravité de chaque anomalie** :
   - au moins une anomalie **bloquante** → `REFUSÉ` : pas de fusion, le développeur corrige jusqu'à ce qu'il n'y en ait plus ;
   - anomalies **mineures** seulement → `À CORRIGER` : le QA donne la main au développeur **deux fois au plus** pour les corriger ;
     si elles restent après le 2e tour → `PASSABLE AVEC ANOMALIES MINEURES` : fusion autorisée ; le QA inscrit chaque anomalie
     restante au **tableau des anomalies** (`anno.md`) ; le **PO** les priorise et crée des **US de correction dédiées** ;
   - aucune anomalie → `VALIDÉ`.
   **Fusion rapide (décision de l'utilisateur, 2026-11-03, MVP)** : sur ordre de l'utilisateur, le développeur peut fusionner sans
   attendre le verdict du QA dès que la compilation, les tests et l'APK sont verts et que ses captures ne montrent rien de bloquant.
   Le QA fait alors son passage **après la fusion**, sur `main` ; une anomalie bloquante trouvée ensuite devient un ticket de
   correction prioritaire, une mineure va dans `anno.md`. Noter chaque fusion rapide au Journal.
   **Fin d'épic** : le PO crée le ticket « Vérification simulateur EX » (émulateur Android,
   workflow `simulateur.yml`, rapport `qa/rapports/simulateur-EX.md`).
6. **Statuts à jour** après chaque avancement, envoyés sur `main` tout de suite, en modifiant seulement ses lignes à partir
   de la dernière version de `main` (ne jamais écraser le fichier).
7. **Fin de ticket** : ✅, Responsable libéré, entrée « Fin » au Journal ; le PO passe en `⏳ Prêt` les tickets débloqués.
8. **Ne jamais supprimer de branche** (gérées par l'utilisateur).
9. Une réservation sans nouvelle depuis 24 h peut être réattribuée par le PO (noté au Journal).

**Définition de « Prêt »** : fichier `regles-de-gestion/WFS-XX.md` écrit par le PO (règles de gestion + éléments du jeu web à réutiliser), dépendances terminées, critères d'acceptation vérifiables sur captures, zone de code connue,
taille ≤ 1 journée (sinon le PO découpe).
**Définition de « Fini »** : critères cochés, CI verte (banc d'essai, APK, émulateur), verdict QA `VALIDÉ` ou `PASSABLE AVEC ANOMALIES MINEURES` (anomalies mineures restantes inscrites au tableau des anomalies), synchronisé, fusionné,
Journal à jour.

## Épics et zones de code
| Épic | Nom | Zone de code (fichiers modifiables) | But |
|---|---|---|---|
| E0 | Noyau partagé (catalyseur) | `src/core/**`, `src/game/game.*`, `src/render/renderer.h` (API), `CMakeLists.txt` | Stabiliser les points d'extension pour que les autres épics ajoutent des fichiers sans toucher au noyau. |
| E1 | Socle moteur (livraison 1) | — | Terminé. |
| E2 | Monde et décor | `src/world/**`, `assets/world/**`, `shaders/world_*` | Ville, cartes, météo, piétons : tout ce qui entoure la route. |
| E3 | Gameplay et simulation | `src/game/` sauf `game.*` (trafic, course, duel, sauvegarde), `tests/**` | Règles du jeu portées du jeu web, tests de fidélité. |
| E4 | Rendu et performances | `src/render/**` (sauf `ui_*`), `shaders/scene*`, `sky*`, `post*`, `shadow*` | Ombres, nuit, reflets, post-traitement, qualité, LOD. |
| E5 | Interface et contenu | `src/ui/**` (sauf `src/ui/ecrans/`), `shaders/ui*`, `assets/fonts/**`, `assets/cars/**`, `tools/convert_car.mjs` | Texte, menus, garage, HUD, voitures et peintures. |
| E6 | Audio | `src/audio/**`, `assets/audio/**` | Moteur, bruitages. |
| E7 | Plateformes et publication | `src/platform/**`, `android/**`, `ios/**`, `.github/**`, `tools/qa/**` | Android, iOS, manettes, stores, CI. |
| E8 | Écrans et menus | `src/ui/ecrans/**` | Chargement de l'application, menu principal, navigation, pause, choix du terrain. |

## MVP du 2026-11-03 (décision du PO, détail au Journal)
Parcours jouable sur téléphone : **chargement → menu → Course solo → décompte → course (trafic, chrono, checkpoints, HUD,
notifications) → fin → Rejouer / Menu**, en plein écran Android. Tickets marqués **[MVP]**, en tête du tableau, dans l'ordre :
WFS-35 → WFS-27 → WFS-37 → WFS-29 → WFS-38 → WFS-47 → WFS-57 (centrage fin, ANO-14) → WFS-56 (scénario simulateur) → WFS-50 (vérification simulateur). Hors MVP : garage, terrain, pause,
duel, sauvegarde, compteur en arc, italique, nuit, météo, bruitages.

## Tableau (par ordre de priorité)
| Prio | Clé | Titre | Épic | Statut | Responsable | Dépend de | Branche | PR | QA |
|---|---|---|---|---|---|---|---|---|---|
| — | WFS-01 | Projet C++ et construction automatique de l'APK | E1 | ✅ Terminé |  | — | `feature/waforspeed-01`/`05` | #1 | ✅ VALIDÉ |
| — | WFS-02 | Noyau : boucle, threads, mémoire, journal | E1 | ✅ Terminé |  | — | `feature/waforspeed-01`/`05` | #1 | ✅ VALIDÉ |
| — | WFS-03 | Rendu Vulkan PBR, HDR, tonemapping, MSAA | E1 | ✅ Terminé |  | — | `feature/waforspeed-01`/`05` | #1 | ✅ VALIDÉ |
| — | WFS-04 | Vraies voitures du jeu web (4 modèles) | E1 | ✅ Terminé |  | — | `feature/waforspeed-01`/`05` | #1 | ✅ VALIDÉ |
| — | WFS-05 | Route, conduite tactile, caméra (+ retours QA) | E1 | ✅ Terminé |  | — | `feature/waforspeed-01`/`05` | #1, #2 | ✅ VALIDÉ |
| 0.1 | WFS-35 | [MVP] Écran de chargement de l'application (logo, barre de progression, fondu) | E8 | ✅ Terminé |  | WFS-26 | `feature/waforspeed-35` | PR #12 (fusionnée) | 🟡 PASSABLE AVEC ANOMALIES MINEURES (ANO-10) |
| 0.15 | WFS-27 | [MVP] HUD de course : vitesse, temps, score, checkpoint, nitro (texte, version réduite) | E5 | ✅ Terminé |  | WFS-26, WFS-12 | `feature/waforspeed-27` | PR #16 | ✅ VALIDÉ par qa-lead au tour 2/2 (main 6e672a9, ANO-11 et ANO-12 corrigées) [rapport](qa/rapports/WFS-27.md) |
| 0.2 | WFS-36 | Point d'extension : écrans de l'application (menu, garage, course, pause…) | E0 | ✅ Terminé | | — | `feature/waforspeed-36` | #13 | ✅ VALIDÉ par qa-lead (e3d7ea4, [rapport](qa/rapports/WFS-36.md)) |
| 0.3 | WFS-37 | [MVP] Menu principal : affichage (logo, choix, record, ta voiture) | E8 | ✅ Terminé (fusionné sur ordre de l'utilisateur, QA après fusion) |  | WFS-35, WFS-36 | `feature/waforspeed-37` | PR #23 (fusionnée) | ⏳ QA après fusion |
| 0.35 | WFS-29 | [MVP] Android : mode plein écran immersif, icône, nom | E7 | ✅ Terminé | | — | `feature/waforspeed-29` | #17 | ✅ VALIDÉ par qa-lead ([rapport](qa/rapports/WFS-29.md)) |
| 0.4 | WFS-38 | [MVP] Menu tactile : appuis et départ de course, fin → Rejouer / Menu | E8 | ✅ Terminé | Claude (session trafic), 2026-11-03 | WFS-37, WFS-12 | `feature/waforspeed-38` | #27 (fusionnée) | 🟡 PASSABLE AVEC ANOMALIES MINEURES tour 2/2 (ANO-14) — [rapport](qa/rapports/WFS-38.md) |
| 0.405 | WFS-57 | [MVP] Correction : centrer la paire « Rejouer » / « Menu » de l'écran de fin (ANO-14) | E3 | ✅ Terminé | Claude (session trafic), 2026-11-03 | WFS-38 ✅ | `feature/waforspeed-57` | #29 (fusionnée) | ✅ VALIDÉ par qa-lead (cc76b01, banc PC, ANO-14 corrigée) — [rapport](qa/rapports/WFS-57.md) |
| 0.42 | WFS-47 | [MVP] Notifications de course (checkpoint, quasi-contact, accident) | E5 | ✅ Terminé | Claude (session trafic), 2026-11-03 | WFS-27 ✅ | `feature/waforspeed-47` | #25 (fusionnée) | ✅ VALIDÉ par qa-lead (c813a56) — [rapport](qa/rapports/WFS-47.md) |
| 0.43 | WFS-51 | Point d'extension : marges de sécurité de l'écran (encoche), commandes tactiles | E0 | ✅ Terminé | Claude (session trafic), 2026-11-03 | WFS-29 ✅ | `feature/waforspeed-51` | #20 (fusionnée) | ✅ VALIDÉ par qa-lead (66c13bc, banc PC) — [rapport](qa/rapports/WFS-51.md) |
| 0.435 | WFS-52 | HUD et notifications dans les marges de sécurité (encoche) | E5 | 🔧 En cours | Claude (session trafic), 2026-11-03 | WFS-51 ✅, WFS-27 ✅ | `feature/waforspeed-52` |  |  |
| 0.441 | WFS-56 | [MVP] Simulateur : scénario « parcours MVP » par appuis (étape 1 de WFS-50) | E7 | 🔧 En cours | Claude (session 01UY1Vv1), 2026-11-03 | WFS-31 ✅ (essai complet après WFS-38) | `feature/waforspeed-56` |  |  |
| 0.44 | WFS-50 | [MVP] Vérification simulateur du MVP (parcours complet sur Android) | E7 | ⏳ En attente |  | WFS-35, WFS-27, WFS-37, WFS-38, WFS-29, WFS-47, WFS-56, WFS-57 | `feature/waforspeed-50` |  |  |
| 0.45 | WFS-44 | Point d'extension : scène dédiée par écran (caméra, fond, brouillard, ce que Game dessine) | E0 | ✅ Terminé |  | WFS-36 | `feature/waforspeed-44` | #15 (fusionnée) | ✅ VALIDÉ par qa-lead (b9771e0, [rapport](qa/rapports/WFS-44.md)) |
| 0.46 | WFS-45 | Lumières : projecteurs (spots), hémisphère, brouillard linéaire | E4 | ⏳ Prêt |  | — | `feature/waforspeed-45` |  |  |
| 0.47 | WFS-49 | Banc : captures du chargement en 2400x1080 sans planche de texte (ANO-10) | E7 | ✅ Terminé | Claude (session trafic), 2026-11-03 | WFS-35 ✅ | `feature/waforspeed-49` | #22 (fusionnée) | ✅ VALIDÉ par qa-lead (52648fd) — [rapport](qa/rapports/WFS-49.md) |
| 0.48 | WFS-48 | Texte en italique (ANO-04) | E5 | ⏳ Prêt |  | WFS-26 | `feature/waforspeed-48` |  |  |
| 0.49 | WFS-46 | Compteur de vitesse en arc (zone rouge) | E5 | ⏳ Prêt |  | WFS-27 ✅ | `feature/waforspeed-46` |  |  |
| 0.5 | WFS-11 | Garage 3D : plateau tournant et choix de la voiture | E5 | ⏳ En attente |  | WFS-45 (WFS-26, WFS-36, WFS-44 ✅) | `feature/waforspeed-11` |  |  |
| 0.6 | WFS-39 | Pause en course et retour au menu | E8 | ⏳ Prêt |  | WFS-38 ✅ | `feature/waforspeed-39` |  |  |
| 0.7 | WFS-40 | Garage : fiche de la voiture et peintures | E5 | ⏳ En attente |  | WFS-11 | `feature/waforspeed-40` |  |  |
| 0.8 | WFS-41 | Choix du terrain et du moment (jour / nuit) | E8 | ⏳ Prêt |  | WFS-38 ✅ | `feature/waforspeed-41` |  |  |
| 1 | WFS-30 | Points d'extension du moteur (systèmes enfichables, CMake par dossier) | E0 | ✅ Terminé | — | — | `feature/waforspeed-30` | #3 | ✅ VALIDÉ par qa-lead |
| 0 | WFS-31 | QA par banc d'essai PC ; simulateur Android en fin d'épic (CI) | E7 | ✅ Terminé | — | — | `feature/waforspeed-31` | #5 | ✅ VALIDÉ par qa-lead |
| 2 | WFS-08 | Trafic multi-thread et collisions | E3 | ✅ Terminé | | — | `feature/waforspeed-08` | #6 | ✅ VALIDÉ par qa-lead après coup ([rapport](qa/rapports/WFS-08.md)) |
| 3 | WFS-06 | Ville : bâtiments, trottoirs, lampadaires, arbres | E2 | ✅ Terminé | — | WFS-30 | `feature/waforspeed-06` | #4 | ✅ VALIDÉ par qa-lead |
| 4 | WFS-26 | Rendu de texte (police, nombres, libellés) | E5 | ✅ Terminé | Claude (session moteur-natif), 2026-11-02 | WFS-30 | `feature/waforspeed-26` | PR #9 | ✅ VALIDÉ par qa-lead |
| 4.5 | WFS-34 | Arbres réalistes portés du jeu web (correction de WFS-06) | E2 | ✅ Terminé | Claude (session moteur-natif), 2026-11-02 | WFS-06 | `feature/waforspeed-34` | PR #14 | ✅ VALIDÉ par qa-lead |
| 5 | WFS-10 | Sons de moteur (AAudio, régime et rapports) | E6 | ✅ Terminé | | WFS-30 | `feature/waforspeed-10` | #7 | ✅ VALIDÉ par qa-lead (d65cd1e, banc PC) — [rapport](qa/rapports/WFS-10.md) |
| 5.5 | WFS-32 | Carrosseries : supprimer les points blancs en trame (reflets du vernis) | E4 | ✅ Terminé | | — | `feature/waforspeed-32` | #8 | ✅ VALIDÉ par qa-lead (84b489b, banc PC) — [rapport](qa/rapports/WFS-32.md) |
| 6 | WFS-07 | Ombres dynamiques du soleil | E4 | ✅ Terminé | Claude (session moteur-natif), 2026-11-02 | WFS-30 | `feature/waforspeed-07` | PR #11 | ✅ VALIDÉ par qa-lead |
| 7 | WFS-12 | Règles de course : chrono, checkpoints, score, nitro, quasi-contacts | E3 | ✅ Terminé |  | WFS-08 | `feature/waforspeed-12` | PR #10 (fusionnée) | 🟡 PASSABLE AVEC ANOMALIES MINEURES (ANO-04) |
| 7.5 | WFS-33 | Banc d'essai : scènes de trafic plus probantes (peloton, trafic en course, choc borné) | E3 | ✅ Terminé | Claude (session moteur-natif), 2026-11-03 | WFS-08 | `feature/waforspeed-33` | PR #18 | ✅ VALIDÉ par qa-lead |
| 8 | WFS-42 | Vitrages et jantes : trame de points (lunette arrière de la Golf, rayons des jantes) | E4 | ✅ Terminé | Claude (session moteur-natif), 2026-11-03 | WFS-32 | `feature/waforspeed-42` | PR #24 | ✅ fusion décidée par l'utilisateur (captures sans blocage) |
| 9 | WFS-09 | Nuit : vrais phares et éclairage public | E4 | ⏳ Prêt |  | WFS-07 | `feature/waforspeed-09` |  |  |
| 12 | WFS-14 | Cartes : socle multi-cartes et Corniche (plage) | E2 | ✅ Terminé | Claude (session trafic), 2026-11-03 | WFS-06 | `feature/waforspeed-14` | #19, #21 (fusionnées) | ✅ VALIDÉ tour 2/2 par qa-lead (e6eaa57) — [rapport](qa/rapports/WFS-14.md) |
| 12.05 | WFS-55 | Correction : PI en double dans le tracé de la route (test_cartes à 1e-9, captures renouvelées) | E3 | ✅ Terminé | Claude (session facile), 2026-11-03 | WFS-14 ✅ | `feature/waforspeed-55` | #26 (fusionnée) | ✅ fusion rapide (CI verte, banc non terminé) — recette de nuit : `qa/Check.md` |
| 12.1 | WFS-53 | Carte Campagne | E2 | ⏳ Prêt |  | WFS-14 ✅ | `feature/waforspeed-53` |  |  |
| 12.2 | WFS-54 | Carte Grand Prix (circuit) | E2 | ⏳ Prêt |  | WFS-14 ✅ | `feature/waforspeed-54` |  |  |
| 13 | WFS-13 | Les 20 voitures et 16 peintures | E5 | ⏳ En attente |  | WFS-11 | `feature/waforspeed-13` |  |  |
| 14 | WFS-15 | Réglages de qualité selon le téléphone | E4 | ⏳ Prêt |  | WFS-07 | `feature/waforspeed-15` |  |  |
| 15 | WFS-28 | Bruitages : chocs, nitro, crissements, ambiance | E6 | ⏳ Prêt |  | WFS-10, WFS-08 | `feature/waforspeed-28` |  |  |
| 16 | WFS-20 | Performances : LOD, tri des objets visibles en parallèle | E4 | ⏳ Prêt |  | WFS-06, WFS-08 | `feature/waforspeed-20` |  |  |
| 17 | WFS-19 | Piétons animés et conducteurs | E2 | ⏳ Prêt |  | WFS-06 | `feature/waforspeed-19` |  |  |
| 17.5 | WFS-43 | Arbres : finitions (cœur du houppier, ombres des arbres) | E2 | ⏳ Prêt |  | WFS-34, WFS-07 | `feature/waforspeed-43` |  |  |
| 18 | WFS-16 | Route mouillée avec reflets | E4 | ⏳ En attente |  | WFS-09 | `feature/waforspeed-16` |  |  |
| 19 | WFS-18 | Météo : pluie, flaques | E2 | ⏳ En attente |  | WFS-16 | `feature/waforspeed-18` |  |  |
| 20 | WFS-17 | Post-traitement : bloom, TAA, flou de mouvement | E4 | ⏳ En attente |  | WFS-15 | `feature/waforspeed-17` |  |  |
| 21 | WFS-24 | Sauvegarde et progression | E3 | ⏳ Prêt |  | WFS-12 | `feature/waforspeed-24` |  |  |
| 22 | WFS-21 | Duel en ligne | E3 | ⏳ Prêt |  | WFS-12 | `feature/waforspeed-21` |  |  |
| 23 | WFS-23 | Manettes de jeu | E7 | ⏳ En attente |  | WFS-29 | `feature/waforspeed-23` |  |  |
| 24 | WFS-22 | Version iOS (Metal via MoltenVK) | E7 | ⏳ En attente |  | WFS-29 | `feature/waforspeed-22` |  |  |
| 25 | WFS-25 | Préparation des stores | E7 | ⏳ En attente |  | WFS-22 | `feature/waforspeed-25` |  |  |

## Critères d'acceptation des tickets ouverts

**WFS-35 — Écran de chargement de l'application** · E8 · priorité 0.1 · ✅ Terminé (QA : PASSABLE AVEC ANOMALIES MINEURES, [rapport](qa/rapports/WFS-35.md)), PR #12 (règles : `regles-de-gestion/WFS-35.md`)
- [ ] `chargement-0/1/2` : fond `#0A0B10`, sur-titre cyan, logo « WAFOR / SPEED » (SPEED orange), barre cyan → orange à 0 %, 20-70 %, 100 %, étape + « n % »
- [ ] mêmes captures en 2400x1080 : bloc à gauche, rien de coupé ; `chargement-fondu` : ville visible à travers (fondu 300 ms)
- [ ] `chargement-erreur` : « Le moteur 3D n'a pas pu démarrer sur cet appareil. » en rouge, pas de plantage
- [ ] journal : durée de chaque étape + total, première image < 0,5 s ; autres captures inchangées

**WFS-36 — Point d'extension : écrans de l'application** · E0 · priorité 0.2 · ✅ Terminé (règles : `regles-de-gestion/WFS-36.md`)
- [ ] `Ecran` lu dans `GameView`, changement demandé par un système, `System::onEcran` ; hors `Course` : pas de conduite ni de commandes tactiles
- [ ] captures du banc identiques (écran `Course` par défaut), tests à 0 écart ; capture `ecran-menu-vide`

**WFS-27 — [MVP] HUD de course (version réduite)** · E5 · priorité 0.15 · ✅ Terminé, PR #16 (règles : `regles-de-gestion/WFS-27.md`)
- [ ] `course-*` et `hud-2400` : temps, checkpoint + barre, score, vitesse (bas centre), nitro 10 cases lisibles, rien sous les commandes
- [ ] `hud-temps-bas` (rouge sous 8 s), `hud-nitro` (nitro active), `hud-combo` (« Combo ×2 »)
- [ ] anciennes barres de `Game::drawUi` retirées ; `course-decompte-*`, `course-fin` inchangées ; banc `0 écart(s)`

**WFS-37 — [MVP] Menu principal : affichage** · E8 · priorité 0.3 · ✅ Terminé (PR #23, QA après fusion) (règles : `regles-de-gestion/WFS-37.md`)
- [ ] `menu-0` / `menu-1` (720p, 2400x1080, sans planche de texte) : ville assombrie, logo, 3 choix, record « — », « Golf 8 GTI » ; `menu-anim`
- [ ] aucune commande de conduite ni HUD sur le menu ; autres captures inchangées, banc `0 écart(s)`

**WFS-29 — [MVP] Android : mode plein écran immersif, icône, nom** · E7 · priorité 0.35 · ✅ Terminé (règles : `regles-de-gestion/WFS-29.md`)
- [ ] manifeste : `android:label="WaForSpeed"`, icône adaptative, thème plein écran ; appel immersif au démarrage et à `onWindowFocusChanged`
- [ ] captures du banc PC inchangées (vérification à l'écran par WFS-50)

**WFS-38 — [MVP] Menu tactile : appuis et départ de course** · E8 · priorité 0.4 · ✅ Terminé (PR #27, PASSABLE, ANO-14) — Claude (session trafic), 2026-11-03 (règles : `regles-de-gestion/WFS-38.md`)
- [ ] `menu-appui`, `menu-depart-3` / `menu-depart-go` (décompte de WFS-12), `menu-bientot`, `fin-menu` (Rejouer + Menu) puis `fin-retour-menu`
- [ ] Android : ouverture sur le menu après le chargement ; autres captures inchangées, banc `0 écart(s)`

**WFS-57 — [MVP] Correction : centrer « Rejouer » / « Menu » (ANO-14)** · E3 · priorité 0.405 · ✅ Terminé (PR #29, VALIDÉ) — Claude (session trafic), 2026-11-03 (règles : `regles-de-gestion/WFS-57.md`)
- [ ] `fin-menu`, `course-fin` : milieu de la paire à x = 640 ± 2 (1280x720) ; `fin-retour-menu` toujours au menu ; autres captures identiques, banc `0 écart(s)`

**WFS-47 — [MVP] Notifications de course** · E5 · priorité 0.42 · ✅ Terminé (Claude, session trafic, PR #25) (règles : `regles-de-gestion/WFS-47.md`)
- [ ] `toast-checkpoint` (« CHECKPOINT 1 » « +11,0 s ») ; `toast-pile` (3 bandeaux cyan / orange / rouge, rien sur les commandes) ; banc `0 écart(s)`

**WFS-51 — Point d'extension : marges de sécurité (encoche)** · E0 · priorité 0.43 · ✅ Terminé (Claude, session trafic, PR #20 fusionnée) (règles : `regles-de-gestion/WFS-51.md`)
- [ ] `encoche-course` (2400x1080, `WFS_ENCOCHE=90`) : commandes tactiles hors de la bande ; autres captures identiques, banc `0 écart(s)`

**WFS-52 — HUD dans les marges de sécurité (encoche)** · E5 · priorité 0.435 · 🔧 En cours — Claude (session trafic), 2026-11-03 (règles : `regles-de-gestion/WFS-52.md`)
- [ ] `encoche-hud` : aucun panneau du HUD dans la bande ; sans encoche, captures identiques

**WFS-56 — [MVP] Simulateur : scénario « parcours MVP » par appuis** · E7 · priorité 0.441 · 🔧 En cours — Claude (session 01UY1Vv1), 2026-11-03 (règles : `regles-de-gestion/WFS-56.md`)
- [ ] `simulateur.yml` avec `scenario=mvp` sur `feature/waforspeed-56` : 10 captures nommées (RG-02), `01-chargement` et `02-menu` corrects, appuis et coordonnées dans `deroulement.txt`, `RÉSULTAT TECHNIQUE : OK`
- [ ] `scenario=conduite` (défaut) : mêmes 7 captures qu'avant ; aucun changement de `src/` ni `android/`

**WFS-50 — [MVP] Vérification simulateur du MVP** · E7 · priorité 0.44 · ⏳ En attente de tout le MVP (règles : `regles-de-gestion/WFS-50.md`)
- [ ] `simulateur.yml` (`epic=MVP`) : chargement → menu → Course solo → décompte → course + HUD → fin → Rejouer puis Menu ; paysage plein écran, aucun plantage
- [ ] rapport `qa/rapports/simulateur-MVP.md` VALIDÉ par `qa-lead`

**WFS-49 — Banc : captures du chargement 2400 sans planche de texte (ANO-10)** · E7 · priorité 0.47 · ✅ Terminé (Claude, session trafic, PR #22) (règles : `regles-de-gestion/WFS-49.md`)
- [ ] `chargement-*-2400` sans panneau « DÉMO DU TEXTE » ; `texte-essai-2400` inchangée

**WFS-48 — Texte en italique (ANO-04)** · E5 · priorité 0.48 · ⏳ Prêt (règles : `regles-de-gestion/WFS-48.md`)
- [ ] décompte, écran de fin et HUD en italique (12°) ; `texte-essai-*` : ligne droite + ligne italique ; autres captures inchangées

**WFS-46 — Compteur de vitesse en arc** · E5 · priorité 0.49 · ⏳ Prêt (règles : `regles-de-gestion/WFS-46.md`)
- [ ] `hud-arc-120`, `hud-arc-320` (zone rouge) en 1280x720 et 2400x1080 ; banc `0 écart(s)`

**WFS-44 — Point d'extension : scène dédiée par écran** · E0 · priorité 0.45 · ✅ Terminé (PR #15) (règles : `regles-de-gestion/WFS-44.md`)
- [ ] Captures du banc identiques à `main` (aucune scène dédiée par défaut), tests à 0 écart
- [ ] Capture `scene-dediee-vide` : fond uni `#07070c`, ni route, ni ville, ni trafic, ni voiture du joueur, ni commandes ; cube repère vu en FOV 36°
- [ ] Doc du point d'extension dans le skill `waforspeed-natif`

**WFS-45 — Lumières : projecteurs (spots), hémisphère, brouillard linéaire** · E4 · priorité 0.46 · ⏳ Prêt (règles : `regles-de-gestion/WFS-45.md`)
- [ ] Captures existantes identiques à `main` (0 spot par défaut)
- [ ] Capture `spots-sol` : 3 taches orange / cyan / blanche aux bords adoucis, hémisphère bleutée, brouillard linéaire, voiture éclairée
- [ ] Coût de la passe de scène ≤ +0,3 ms avec 3 spots (journal du banc)

**WFS-11 — Garage 3D : plateau tournant et choix de la voiture** · E5 · priorité 0.5 · ⏳ En attente de WFS-45 (WFS-44 ✅) (règles : `regles-de-gestion/WFS-11.md`, redécoupé)
- [ ] `garage-*` : plateau, anneau néon, projecteurs, 3 angles ; `garage-golf` / `garage-m4` : changement de voiture, nom et index

**WFS-39 — Pause en course et retour au menu** · E8 · priorité 0.6 · ⏳ Prêt (WFS-38 ✅) (règles : `regles-de-gestion/WFS-39.md`)
- [ ] `pause-bouton`, `pause-0` (3 boutons), `pause-reprise` (chrono conservé), `pause-quitter` (menu)

**WFS-40 — Garage : fiche de la voiture et peintures** · E5 · priorité 0.7 · ⏳ En attente (règles : `regles-de-gestion/WFS-40.md`)
- [ ] `garage-fiche` (fiche + crédit), `garage-peinture-0/1`, course avec la voiture et la peinture choisies

**WFS-41 — Choix du terrain et du moment** · E8 · priorité 0.8 · ⏳ Prêt (WFS-38 ✅) (règles : `regles-de-gestion/WFS-41.md`)
- [ ] `terrain-0` / `terrain-1` (carrousel, Jour / Nuit), `terrain-rouler` (course sur le circuit)

**WFS-30 — Points d'extension du moteur (systèmes enfichables, CMake par dossier)** · E0 · priorité 1 · ✅ Terminé
- [ ] Interface `System` (init / update / render / interface) enregistrée dans `Game` : un nouveau module = un nouveau fichier + 1 ligne d'enregistrement
- [ ] CMake ajoute automatiquement les fichiers de `src/world`, `src/ui`, `src/audio`, `src/game`
- [ ] Accès en lecture pour les systèmes : piste, joueur, caméra, renderer ; doc dans le skill `waforspeed-natif`
- [ ] Aucun changement visible ; CI verte, captures identiques

**WFS-08 — Trafic multi-thread et collisions** · E3 · priorité 2 · ✅ Terminé
- [ ] Port de traffic.js (même graine)
- [ ] Mise à jour sur les threads de travail
- [ ] Collisions et quasi-contacts comme le jeu web

**WFS-06 — Ville : bâtiments, trottoirs, lampadaires, arbres** · E2 · priorité 3 · ✅ Terminé
- [ ] Décor généré par la graine de la carte (comme decor.js)
- [ ] Instanciation GPU, ≤ 300 appels de dessin
- [ ] Captures : rue bordée d'immeubles, lampadaires, arbres

**WFS-26 — Rendu de texte (police, nombres, libellés)** · E5 · priorité 4 · ✅ Terminé (PR #9)
- [ ] Police (licence libre) en atlas, texte en pixels logiques, accents français
- [ ] Lisible sur 2400x1080 et petits écrans
- [ ] Capture : texte d'essai net

**WFS-34 — Arbres réalistes portés du jeu web** · E2 · priorité 4.5 · ✅ Terminé (PR #14) (règles : `regles-de-gestion/WFS-34.md`)
- [ ] Générateurs de `life.js` portés en `src/world/vegetation.*`, atlas exporté en PNG (`assets/world/feuillage.png`), arbres de `ville.cpp` remplacés
- [ ] Capture : platanes avec tronc, branches, feuillage découpé ; ≤ 300 appels de dessin ; banc `0 écart(s)`

**WFS-10 — Sons de moteur (AAudio, régime et rapports)** · E6 · priorité 5 · ✅ Terminé
- [ ] Sons du jeu web, régime qui suit l'accélérateur et les rapports
- [ ] Pas de craquement, coupure en pause

**WFS-32 — Carrosseries : supprimer les points blancs en trame (reflets du vernis)** · E4 · priorité 5.5 · ✅ Terminé
Origine : remarque 5 du [rapport QA WFS-08](qa/rapports/WFS-08.md) (défaut antérieur à WFS-08). Zone : `src/render/**`
(sauf `ui_*`), `shaders/scene*`. Pistes : reflet spéculaire du vernis (`scene.frag`) trop fin / non filtré (aliasing
spéculaire, normales non normalisées, NaN, valeurs HDR extrêmes avant le tonemapping), résolution MSAA.
- [ ] Cause identifiée et notée au Journal (avec la capture qui la montre)
- [ ] Captures du banc PC `vitrine-golf-0`, `vitrine-p911-1`, `course-cam0-1`, `trafic-choc` : plus aucun point blanc
  isolé en trame sur les carrosseries (Golf près des feux arrière, avant de la 911, Gallardo)
- [ ] Les reflets du vernis restent visibles (peinture toujours brillante, pas d'aspect mat) ; aucune autre régression visible
- [ ] Coût GPU inchangé à ±0,5 ms (journal du banc)

**WFS-07 — Ombres dynamiques du soleil** · E4 · priorité 6 · ✅ Terminé (PR #11) (règles : `regles-de-gestion/WFS-07.md`)
- [ ] `course-cam0-*` en jour : ombres nettes des voitures, immeubles et lampadaires sur chaussée et trottoirs
- [ ] Deux captures à 1 m d'écart : bords d'ombre stables (calage sur la grille des texels)
- [ ] Nuit : aucune ombre solaire, ombre de contact conservée
- [ ] Journal du banc : passe d'ombre ≤ 3 ms ; test de fidélité inchangé

**WFS-12 — Règles de course : chrono, checkpoints, score, nitro, quasi-contacts** · E3 · priorité 7 · ✅ Terminé (QA : PASSABLE AVEC ANOMALIES MINEURES, [rapport](qa/rapports/WFS-12.md)), PR #10 (règles : `regles-de-gestion/WFS-12.md`)
- [ ] Règles identiques au mode solo web (temps, checkpoints, score, combos, nitro)
- [ ] Tests automatiques de fidélité (test unitaire : checkpoint 1 → +11 s, checkpoint 10 → +7,4 s (checkpoint 11 → +7 s), quasi-contact ×3 → 750 points)

**WFS-33 — Banc d'essai : scènes de trafic plus probantes (peloton, trafic en course, choc borné)** · E3 · priorité 7.5 · ✅ Terminé (PR #18) (règles : `regles-de-gestion/WFS-33.md`)
Origine : remarques 1, 2 et 4 du [rapport QA WFS-08](qa/rapports/WFS-08.md). Zone : `src/game/traffic_system.cpp`
(scènes du banc uniquement ; la simulation `traffic.cpp` ne change pas, le test de fidélité doit rester à 0 écart). Petite taille.
- [ ] `trafic-cam0` : le joueur roule derrière un **groupe de 2 à 3 voitures** du trafic, visibles ensemble sur la capture,
  chacune dans sa file
- [ ] Captures `course-*` : au moins **3 images sur 6** montrent une voiture du trafic dans le champ, sur la route
  (placement ou densité du trafic près du joueur pour les scènes du banc seulement, pas de changement des règles du jeu)
- [ ] `trafic-choc` (vérification) : le journal donne l'écart latéral de la voiture percutée, borné à ±1,12 comme
  `crashTraffic` du jeu web ; le ticket note au Journal si le dépassement de l'arrière sur le trottoir existe aussi dans
  le jeu web (alors accepté) ou non (alors correction dans la scène ou ticket E3 de correction)
- [ ] Journal du banc : `mode exact : … 0 écart(s)` inchangé ; aucune voiture du trafic dans le décor

**WFS-42 — Vitrages et jantes : trame de points (lunette arrière de la Golf, rayons des jantes)** · E4 · priorité 8 · ✅ Terminé (PR #24) (règles : `regles-de-gestion/WFS-42.md`)
Origine : restes de « Fin WFS-32 » (rapport `qa/rapports/WFS-32.md`). Zone : `src/render/**` (sauf `ui_*`), `shaders/scene*` ; si la cause est dans le modèle (`assets/cars/**`, E5) : la noter au Journal et ouvrir un ticket E5, ne pas toucher au modèle.
- [ ] Cause identifiée et notée au Journal (texture du modèle, transparence du vitrage, reflet spéculaire des rayons…) avec la capture qui la montre
- [ ] `vitrine-golf-0` (agrandie sur la lunette arrière) : plus de trame régulière de petits points clairs ; vitre toujours teintée et réfléchissante
- [ ] `vitrine-p911-1` (agrandie sur les jantes) : rayons sans points brillants isolés, jantes toujours métalliques
- [ ] Aucune autre capture du banc dégradée ; coût GPU au journal du banc inchangé à ±0,5 ms


**WFS-09 — Nuit : vrais phares et éclairage public** · E4 · priorité 9 · ⏳ Prêt
- [ ] Phares réalistes (leçons de la version web)
- [ ] Lampadaires, mode nuit par carte



**WFS-14 — Cartes : socle multi-cartes et Corniche (plage)** · E2 · priorité 12 · ✅ Terminé (règles : `regles-de-gestion/WFS-14.md`, redécoupé)
- [ ] `carte-plage-jour` / `carte-plage-nuit` : océan, palmiers, parasols, sable, ambiance de la plage ; autres captures identiques
- [ ] test de fidélité du tracé des 3 nouvelles cartes (0 écart) ; ≤ 300 appels de dessin

**WFS-55 — Correction : PI en double dans le tracé** · E3 · priorité 12.05 · ✅ Terminé (PR #26) (règles : `regles-de-gestion/WFS-55.md`)
- [ ] `test_cartes` en tolérance 1e-9 ; captures renouvelées listées, sans différence visible

**WFS-53 — Carte Campagne** · E2 · priorité 12.1 · ⏳ Prêt (règles : `regles-de-gestion/WFS-53.md`)
- [ ] `carte-campagne-jour` / `-nuit` : collines, champs, haies, fermes, éoliennes ; ≤ 300 appels de dessin

**WFS-54 — Carte Grand Prix (circuit)** · E2 · priorité 12.2 · ⏳ Prêt (règles : `regles-de-gestion/WFS-54.md`)
- [ ] `carte-circuit-jour` / `-nuit` : vibreurs, tribunes, murs de pneus, gravier ; ≤ 300 appels de dessin

**WFS-13 — Les 20 voitures et 16 peintures** · E5 · priorité 13 · ⏳ En attente
- [ ] Toutes converties et vérifiées en vitrine
- [ ] Taille de l'APK maîtrisée

**WFS-15 — Réglages de qualité selon le téléphone** · E4 · priorité 14 · ⏳ Prêt
- [ ] Niveaux bas / moyen / haut, choix automatique

**WFS-28 — Bruitages : chocs, nitro, crissements, ambiance** · E6 · priorité 15 · ⏳ Prêt
- [ ] Bruitages synchronisés aux évènements du jeu

**WFS-20 — Performances : LOD, tri des objets visibles en parallèle** · E4 · priorité 16 · ⏳ Prêt
- [ ] 60 i/s visés, mesures dans le journal

**WFS-19 — Piétons animés et conducteurs** · E2 · priorité 17 · ⏳ En attente
- [ ] Personnages du jeu web, conducteur face au volant

**WFS-43 — Arbres : finitions (cœur du houppier, ombres des arbres)** · E2 · priorité 17.5 · ⏳ Prêt (règles : `regles-de-gestion/WFS-43.md`)
- [ ] `trafic-cam0` : plus de cœur visible au bord des houppiers ; ombre d'arbre découpée visible sur une capture au soleil
- [ ] ≤ 300 appels de dessin ; banc `0 écart(s)`

**WFS-16 — Route mouillée avec reflets** · E4 · priorité 18 · ⏳ En attente
- [ ] Reflets plausibles, coût maîtrisé

**WFS-18 — Météo : pluie, flaques** · E2 · priorité 19 · ⏳ En attente
- [ ] Pluie animée, route mouillée

**WFS-17 — Post-traitement : bloom, TAA, flou de mouvement** · E4 · priorité 20 · ⏳ En attente (règles : `regles-de-gestion/WFS-17.md`)
- [ ] Activable selon la qualité
- [ ] `vitrine-golf-0`, `vitrine-p911-1` : plus de lignes pointillées sur les jointures de carrosserie (reste de WFS-32, crénelage de géométrie lissé par le TAA)

**WFS-24 — Sauvegarde et progression** · E3 · priorité 21 · ⏳ Prêt
- [ ] Meilleurs scores, voitures et réglages conservés

**WFS-21 — Duel en ligne** · E3 · priorité 22 · ⏳ Prêt
- [ ] Compatible avec le principe du duel web

**WFS-23 — Manettes de jeu** · E7 · priorité 23 · ⏳ En attente
- [ ] Manettes Bluetooth Android

**WFS-22 — Version iOS (Metal via MoltenVK)** · E7 · priorité 24 · ⏳ En attente
- [ ] Construction iOS, test TestFlight

**WFS-25 — Préparation des stores** · E7 · priorité 25 · ⏳ En attente
- [ ] Icônes, signature, fiches

> WFS-01 à WFS-05 (E1) : critères détaillés et rapports dans `qa/rapports/` et dans le Journal (`journal.md`).

## Anomalies et journal (fichiers à part)
- **Anomalies mineures** (QA → PO) : `anno.md`.
- **Journal** du projet (une entrée par étape, nouvelles entrées à la fin) : `journal.md`.
Dans ce fichier, « au Journal » veut dire « dans `journal.md` ».

### 2026-11-03 — Fin WFS-44 Scène dédiée par écran (Claude, session trafic)
**QA** : VALIDÉ par `qa-lead` (`qa/rapports/WFS-44.md`, banc PC, `b9771e0`), PR #15 fusionnée.
**Synchronisation finale** : `main` avait reçu WFS-12 (code de course) pendant la QA ; fusion automatique sans conflit, puis
banc complet relancé en local sur le résultat : 29 captures identiques à `main`, `scene-dediee-vide` identique au pixel près
à celle vérifiée par la QA (le HUD de course n'y apparaît pas).
**Fait** : `System::scene`, `SceneDediee`, `GameView::sceneDediee` ; ville et trafic sans dessin en scène dédiée ; test
`src/core/scene_essai.*` ; doc dans le skill.
**Pour WFS-11** : le garage prend la scène dans `scene()` quand `g.ecran == Ecran::Garage`. À vérifier alors : le
`CourseSystem` (WFS-12) ne dessine pas son HUD dans le garage (il n'apparaît pas dans `scene-dediee-vide`, écran Menu).
WFS-11 reste bloqué par WFS-45 (projecteurs, E4).
