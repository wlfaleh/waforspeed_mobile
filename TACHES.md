# WaForSpeed Mobile — tableau des tâches

** RÈGLE IMPORTANT : il faut voir systématiquement le fichier tache utilisateur.md et répondre en cas de besoin à l'utilisateur

Version native (C++20, Vulkan) du jeu WaForSpeed, pour Android puis iOS, portable sur consoles. Moteur maison, aucun moteur
de jeu tiers. Organisation type Jira / Scrum :
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
   **Toujours synchroniser la branche avec `main` avant de lancer une CI** : avant chaque `git push` de la branche, chaque PR et chaque lancement de workflow (`build.yml`, `simulateur.yml`…), faire `git fetch origin && git merge origin/main`, résoudre les conflits en gardant les deux modifications, recompiler / relancer les tests locaux, puis seulement pousser. Une CI lancée sur une branche en retard sur `main` ne compte pas (captures et verdict non valables).
5. **QA** : après chaque construction, le développeur **lance l'agent `qa-lead`**, qui regarde les captures du
   **banc d'essai PC** et écrit `qa/rapports/WFS-XX.md`. Le développeur ne fait jamais la QA de son propre ticket.
   **Le QA décide de la gravité de chaque anomalie** :
   - au moins une anomalie **bloquante** → `REFUSÉ` : pas de fusion, le développeur corrige jusqu'à ce qu'il n'y en ait plus ;
   - anomalies **mineures** seulement → `À CORRIGER` : le QA donne la main au développeur **deux fois au plus** pour les corriger ;
     si elles restent après le 2e tour → `PASSABLE AVEC ANOMALIES MINEURES` : fusion autorisée ; le QA inscrit chaque anomalie
     restante au **tableau des anomalies** (`anno.md`) ; le **PO** les priorise et crée des **US de correction dédiées** ;
   - aucune anomalie → `VALIDÉ`.
   **Fusion directe par confiance (règle de l’utilisateur, 2026-11-04, prioritaire sur les règles de fusion ci-dessous)** : si le développeur est **confiant à plus de 50%** que son ticket fonctionne, il **fusionne directement**, sans attendre la fin de la CI ni les captures de sa branche. Conditions minimales, toujours : branche synchronisée avec `main` (`git fetch origin && git merge origin/main`), compilation et tests CPU passés en local, aucune modification hors de sa zone (sauf accroches ≤ 10 lignes annoncées au Journal). Au moment de la fusion : ligne obligatoire dans `qa/Check.md` (ticket, PR, date, « fusion par confiance », points à regarder), statut ✅ dans `TACHES.md`, entrée au Journal. Après la fusion, le développeur **suit la CI de `main`** : si elle est rouge ou si une capture est cassée par son ticket, il corrige **tout de suite**, en priorité sur tout le reste. Confiance ≤ 50% (gros changement du rendu ou du noyau, aucun moyen de tester en local, doute sur le résultat) : il attend la CI verte et regarde les captures avant de fusionner.
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
   **Branche poussée dans l'heure** (ordre de l'utilisateur, `tache utilisateur.md` ; précisé par le PO le 2026-11-05) :
   l'agent pousse `feature/waforspeed-XX` dans l'heure qui suit l'attribution (le commit de début suffit). Un ticket 🔧 sans
   branche poussée 2 h après l'attribution est remis en `⏳ Prêt` par le PO, avec une trace au Journal.

10. **Autonomie (règle de l’utilisateur)** : les agents **ne demandent pas de confirmation à l’utilisateur** pour suivre le processus. Ils enchaînent seuls : demander un ticket au PO, créer la branche, lire les règles, coder, construire, fusionner selon la règle en vigueur, mettre à jour `TACHES.md`, `journal.md`, `qa/Check.md`, puis demander le ticket suivant. Pas de « je lance ? », « je continue ? » en fin de message : on agit, puis on rend compte. L’utilisateur n’est sollicité que pour un **vrai blocage** : décision produit que ni les règles de gestion ni le PO ne tranchent, accès ou droits manquants, action irréversible hors processus (supprimer, réécrire l’historique…). Dans ce cas : une seule question précise, avec une recommandation. Une question de règles ou de priorité se pose au PO (`po-scrum-master`), pas à l’utilisateur.

**Définition de « Prêt »** : fichier `regles-de-gestion/WFS-XX.md` écrit par le PO (règles de gestion + éléments du dépôt à réutiliser), dépendances terminées, critères d'acceptation vérifiables sur captures, zone de code connue,
taille ≤ 1 journée (sinon le PO découpe).
**Définition de « Fini »** (sauf fusion par confiance, voir ci-dessus : la CI et la recette viennent après la fusion) : critères cochés, CI verte (banc d'essai, APK, émulateur), verdict QA `VALIDÉ` ou `PASSABLE AVEC ANOMALIES MINEURES` (anomalies mineures restantes inscrites au tableau des anomalies), synchronisé, fusionné,
Journal à jour.

## Épics et zones de code
| Épic | Nom | Zone de code (fichiers modifiables) | But |
|---|---|---|---|
| E0 | Noyau partagé (catalyseur) | `src/core/**`, `src/game/game.*`, `src/render/renderer.h` (API), `CMakeLists.txt` | Stabiliser les points d'extension pour que les autres épics ajoutent des fichiers sans toucher au noyau. |
| E1 | Socle moteur (livraison 1) | — | Terminé. |
| E2 | Monde et décor | `src/world/**` (sauf fichiers E11 / E12 ci-dessous), `assets/world/**` (sauf `assets/world/modeles/**`), `shaders/world_*` | Ville, cartes, météo, piétons : tout ce qui entoure la route. |
| E3 | Gameplay et simulation | `src/game/` sauf `game.*` (trafic, course, duel, sauvegarde), `tests/**` | Règles du jeu, tests de fidélité. |
| E4 | Rendu et performances | `src/render/**` (sauf `ui_*` et fichiers E9 à E13 ci-dessous), `shaders/scene.*` (orchestration ; `sky*`, `post*`, `shadow*` passent à E13 le 2026-11-06) | Ombres, nuit, reflets, post-traitement, qualité, LOD. |
| E5 | Interface et contenu | `src/ui/**` (sauf `src/ui/ecrans/`), `shaders/ui*`, `assets/fonts/**` (`assets/cars/**` et `tools/convert_car.mjs` passent à E10 le 2026-11-06), `tools/convert_car.mjs` | Texte, menus, garage, HUD, voitures et peintures. |
| E6 | Audio | `src/audio/**`, `assets/audio/**` | Moteur, bruitages. |
| E7 | Plateformes et publication | `src/platform/**`, `android/**`, `ios/**`, `.github/**`, `tools/qa/**` | Android, iOS, manettes, stores, CI. |
| E8 | Écrans et menus | `src/ui/ecrans/**` | Chargement de l'application, menu principal, navigation, pause, choix du terrain. |
| E9 | Réalisme — Routes | `shaders/route*.glsl`, `src/render/route_*`, `assets/textures/route/**`, `references/reel/routes/**`, `tests/fidelite_routes*` (+ accroches ≤ 10 lignes `src/game/track.cpp`) | Revêtements, marquages, imperfections, états de surface au plus près du réel (axe Réalisme, `regles-de-gestion/DA-realisme.md`). |
| E10 | Réalisme — Véhicules | `shaders/vehicule*.glsl`, `src/render/vehicule_*`, `assets/cars/**`, `tools/convert_car.mjs`, `references/reel/voitures/**`, `tests/fidelite_voitures*` | Dimensions exactes, matériaux, dynamique visible des voitures. |
| E11 | Réalisme — Personnages | `assets/people/**`, `tools/convert_people*`, `src/world/pietons.*`, `src/world/foule_*`, `src/render/squelette.*`, `shaders/scene_skin*`, `references/reel/humains/**`, `tests/fidelite_humains*` | Humains réalistes, animations, comportements de foule. |
| E12 | Réalisme — Environnements | `src/world/archi_*`, `src/world/mobilier_*`, `src/world/vegetation.*`, `shaders/decor*.glsl`, `assets/textures/decor/**`, `assets/world/modeles/**`, `references/reel/decor/**`, `tests/fidelite_decor*` (+ accroches ≤ 10 lignes par carte) | Bâtiments, mobilier et signalisation, végétation, terrain, mer. |
| E13 | Réalisme — Lumière et image | `shaders/eclairage*.glsl`, `shaders/sky*`, `post*`, `shadow*`, `src/render/eclairage_*`, `ibl_*`, `atmosphere_*`, `post_*`, `references/reel/lumiere/**`, `tests/fidelite_lumiere*` | Unités physiques, IBL, ciel et soleil calculés, cycle jour / nuit, ombres, reflets, post-traitement. |

> Zones E9 à E13 créées le 2026-11-06 par le PO (axe Réalisme). Le ticket en cours WFS-144 (E4) garde ses droits sur ses fichiers jusqu'à sa fusion.

## MVP du 2026-11-03 (décision du PO, détail au Journal)
Parcours jouable sur téléphone : **chargement → menu → Course solo → décompte → course (trafic, chrono, checkpoints, HUD,
notifications) → fin → Rejouer / Menu**, en plein écran Android. **Décision de l'utilisateur (2026-11-03) : le MVP n'attend plus le simulateur ; l'utilisateur teste lui-même l'APK de `main` sur téléphone** (WFS-50, WFS-58 et WFS-64 sans urgence). Tickets marqués **[MVP]** (le tableau est trié par numéro de ticket ; la colonne « Priorité » donne l'urgence), dans l'ordre :
WFS-35 → WFS-27 → WFS-37 → WFS-29 → WFS-38 → WFS-47 → WFS-57 (centrage fin, ANO-14) → WFS-56 (scénario simulateur) → WFS-58 + WFS-59 (corrections simulateur) → WFS-50 (vérification simulateur ; WFS-64 suspendu : le gel ne s'est pas reproduit, run 41719456821). Hors MVP : garage, terrain, pause,
duel, sauvegarde, compteur en arc, italique, nuit, météo, bruitages.

## Axe stratégique « Réalisme » (directive de l'utilisateur, 2026-11-05 et 2026-11-06) — priorité HAUTE
Il n'y a plus de version Web : le jeu natif est la seule référence. Objectif : routes, voitures, personnages, environnements et lumière au plus près
du réel, **fidélité de 99 % mesurée** (part des mesures dans leur tolérance face à des données réelles sourcées), **jamais l'apparence au
détriment de la précision**. Charte, analyse de l'existant, limitations, tolérances et règles communes RC-01 à RC-10 :
**`regles-de-gestion/DA-realisme.md`** (à lire avant toute US de l'axe). Toutes les US de l'axe sont en **P2 Haute**.
**Étape par étape** : une US par partie, dans l'ordre des dépendances ci-dessous. Attribution, QA et fusion : règles de travail en vigueur (`CLAUDE.md`), inchangées.
| Palier | US dans l'ordre (→ = validée avant la suivante) | Épic | Profil d'agent |
|---|---|---|---|
| 0 — Référence et socle | WFS-148 → WFS-149 → WFS-150 | E0 | moteur Vulkan |
| 1 — Lumière | WFS-151 → WFS-152 → WFS-153 → WFS-154 | E13 | rendu / éclairage |
| 2 — Véhicules | WFS-158 (dès WFS-148) → WFS-159 (après WFS-151) → WFS-160 | E10 | technical artist véhicules |
| 2 — Routes | WFS-155 (après WFS-151) → WFS-156 → WFS-157 (après WFS-154) | E9 | rendu matériaux / route |
| 3 — Environnements | WFS-161 (après WFS-151) → WFS-162 → WFS-163 (après WFS-153) | E12 | technical artist décor |
| 4 — Personnages | WFS-164 (après WFS-151) → WFS-165 → WFS-166 | E11 | technical artist personnages, animation |
| 5 — Harmonisation | WFS-167 (après toutes) | E13 | rendu |

## Tableau (par numéro de ticket)

**Priorités (comme Jira)** : **P1 Urgent** (bloque le MVP) · **P2 Haute** · **P3 Moyenne** · **P4 Basse**. À priorité égale, on prend le plus petit numéro de ticket.
| Priorité | Clé | Titre | Épic | Statut | Responsable | Dépend de | Branche | PR | QA |
|---|---|---|---|---|---|---|---|---|---|
| — | WFS-01 | Projet C++ et construction automatique de l'APK | E1 | ✅ Terminé |  | — | `feature/waforspeed-01`/`05` | #1 | ✅ VALIDÉ |
| — | WFS-02 | Noyau : boucle, threads, mémoire, journal | E1 | ✅ Terminé |  | — | `feature/waforspeed-01`/`05` | #1 | ✅ VALIDÉ |
| — | WFS-03 | Rendu Vulkan PBR, HDR, tonemapping, MSAA | E1 | ✅ Terminé |  | — | `feature/waforspeed-01`/`05` | #1 | ✅ VALIDÉ |
| — | WFS-04 | Vraies voitures du jeu web (4 modèles) | E1 | ✅ Terminé |  | — | `feature/waforspeed-01`/`05` | #1 | ✅ VALIDÉ |
| — | WFS-05 | Route, conduite tactile, caméra (+ retours QA) | E1 | ✅ Terminé |  | — | `feature/waforspeed-01`/`05` | #1, #2 | ✅ VALIDÉ |
| 3 | WFS-06 | Ville : bâtiments, trottoirs, lampadaires, arbres | E2 | ✅ Terminé | — | WFS-30 | `feature/waforspeed-06` | #4 | ✅ VALIDÉ par qa-lead |
| 6 | WFS-07 | Ombres dynamiques du soleil | E4 | ✅ Terminé | Claude (session moteur-natif), 2026-11-02 | WFS-30 | `feature/waforspeed-07` | PR #11 | ✅ VALIDÉ par qa-lead |
| 2 | WFS-08 | Trafic multi-thread et collisions | E3 | ✅ Terminé | | — | `feature/waforspeed-08` | #6 | ✅ VALIDÉ par qa-lead après coup ([rapport](qa/rapports/WFS-08.md)) |
| P3 Moyenne | WFS-09 | Nuit complète : phares, éclairage public, route mouillée et reflets | E4 | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic), 2026-11-04 | WFS-07 | `feature/waforspeed-09` | PR #59 | QA de nuit : `qa/Check.md` |
| 5 | WFS-10 | Sons de moteur (AAudio, régime et rapports) | E6 | ✅ Terminé | | WFS-30 | `feature/waforspeed-10` | #7 | ✅ VALIDÉ par qa-lead (d65cd1e, banc PC) — [rapport](qa/rapports/WFS-10.md) |
| P2 Haute | WFS-11 | Garage complet : plateau, 20 voitures, 16 peintures, fiche et compteur en arc | E5 | ✅ Fusionné (QA de nuit à faire ; 16 voitures à suivre) | agent 3 (session 011cpqZd), 2026-11-03 | WFS-45 ✅ (fusionné), WFS-26, WFS-36, WFS-44 ✅, WFS-27 ✅ | `feature/waforspeed-11` |  |  |
| 7 | WFS-12 | Règles de course : chrono, checkpoints, score, nitro, quasi-contacts | E3 | ✅ Terminé |  | WFS-08 | `feature/waforspeed-12` | PR #10 (fusionnée) | 🟡 PASSABLE AVEC ANOMALIES MINEURES (ANO-04) |
| P2 Haute | WFS-13 | Les 16 autres voitures (conversion, catalogue, sons, licences) — rouvert après WFS-11 | E5 | ✅ Fusionné (fusion par confiance, QA de nuit à faire) (exception : 2e ticket E5, fichiers séparés de WFS-60) | agent 3 (session 011cpqZd), 2026-11-04 | WFS-11 ✅ | `feature/waforspeed-13` |  |  |
| 12 | WFS-14 | Cartes : socle multi-cartes et Corniche (plage) | E2 | ✅ Terminé | Claude (session trafic), 2026-11-03 | WFS-06 | `feature/waforspeed-14` | #19, #21 (fusionnées) | ✅ VALIDÉ tour 2/2 par qa-lead (e6eaa57) — [rapport](qa/rapports/WFS-14.md) |
| P2 Haute | WFS-15 | Qualité, performances et post-traitement (réglages téléphone, LOD, bloom, TAA, flou) | E4 | ✅ Fusionné (fusion par confiance, QA de nuit à faire) | Claude (agent 4, session 011cpqZan), 2026-11-04 | WFS-06, WFS-07, WFS-08 | `feature/waforspeed-15` | #56 |  |
| — | WFS-16 | Route mouillée avec reflets | E4 | ↪ Fusionné dans WFS-09 |  | — | `feature/waforspeed-16` |  |  |
| — | WFS-17 | Post-traitement : bloom, TAA, flou de mouvement | E4 | ↪ Fusionné dans WFS-15 |  | — | `feature/waforspeed-17` |  |  |
| P4 Basse | WFS-18 | Météo : pluie, flaques | E2 | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic, agent WFS-18), 2026-11-04 | WFS-09 ✅ | `feature/waforspeed-18` | #66 |  |
| P2 Haute | WFS-19 | Personnages : piétons animés (ville et plage) | E2 | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic), 2026-11-03 | WFS-06 ✅, WFS-63 ✅, WFS-72 ✅ | `feature/waforspeed-19` | #42 (fusionnée, 710c9fc) |  |
| — | WFS-20 | Performances : LOD, tri des objets visibles en parallèle | E4 | ↪ Fusionné dans WFS-15 |  | — | `feature/waforspeed-20` |  |  |
| P2 Haute | WFS-21 | Mode duel en ligne (salon, même trafic, fantômes, classement, revanche) | E3 | ✅ Fusionné (ordre de l'utilisateur, QA de nuit à faire) | Claude (agent 4, session 011cpqZan), 2026-11-03 | WFS-12 ✅ | `feature/waforspeed-21` | #38 (fusionnée, 41e7936) |  |
| P4 Basse | WFS-22 | Plateformes : iOS, manettes et préparation des stores | E7 | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic), 2026-11-04 | WFS-29 ✅ | `feature/waforspeed-22` | #60, #62 |  |
| — | WFS-23 | Manettes de jeu | E7 | ↪ Fusionné dans WFS-22 |  | — | `feature/waforspeed-23` |  |  |
| P3 Moyenne | WFS-24 | Sauvegarde et progression (records, voiture et réglages mémorisés) | E3 | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic), 2026-11-04 | WFS-12 | `feature/waforspeed-24` | [#54](https://github.com/wlfaleh/waforspeed_mobile/pull/54) |  |
| — | WFS-25 | Préparation des stores | E7 | ↪ Fusionné dans WFS-22 |  | — | `feature/waforspeed-25` |  |  |
| 4 | WFS-26 | Rendu de texte (police, nombres, libellés) | E5 | ✅ Terminé | Claude (session moteur-natif), 2026-11-02 | WFS-30 | `feature/waforspeed-26` | PR #9 | ✅ VALIDÉ par qa-lead |
| 0.15 | WFS-27 | [MVP] HUD de course : vitesse, temps, score, checkpoint, nitro (texte, version réduite) | E5 | ✅ Terminé |  | WFS-26, WFS-12 | `feature/waforspeed-27` | PR #16 | ✅ VALIDÉ par qa-lead au tour 2/2 (main 6e672a9, ANO-11 et ANO-12 corrigées) [rapport](qa/rapports/WFS-27.md) |
| P3 Moyenne | WFS-28 | Bruitages : chocs, nitro, crissements, ambiance | E6 | ✅ Fusionné (ordre de l'utilisateur, QA de nuit à faire) | agent 3 (session 011cpqZd), 2026-11-03 | WFS-10 ✅, WFS-08 ✅ | `feature/waforspeed-28` | [#36](https://github.com/wlfaleh/waforspeed_mobile/pull/36) |  |
| 0.35 | WFS-29 | [MVP] Android : mode plein écran immersif, icône, nom | E7 | ✅ Terminé | | — | `feature/waforspeed-29` | #17 | ✅ VALIDÉ par qa-lead ([rapport](qa/rapports/WFS-29.md)) |
| 1 | WFS-30 | Points d'extension du moteur (systèmes enfichables, CMake par dossier) | E0 | ✅ Terminé | — | — | `feature/waforspeed-30` | #3 | ✅ VALIDÉ par qa-lead |
| 0 | WFS-31 | QA par banc d'essai PC ; simulateur Android en fin d'épic (CI) | E7 | ✅ Terminé | — | — | `feature/waforspeed-31` | #5 | ✅ VALIDÉ par qa-lead |
| 5.5 | WFS-32 | Carrosseries : supprimer les points blancs en trame (reflets du vernis) | E4 | ✅ Terminé | | — | `feature/waforspeed-32` | #8 | ✅ VALIDÉ par qa-lead (84b489b, banc PC) — [rapport](qa/rapports/WFS-32.md) |
| 7.5 | WFS-33 | Banc d'essai : scènes de trafic plus probantes (peloton, trafic en course, choc borné) | E3 | ✅ Terminé | Claude (session moteur-natif), 2026-11-03 | WFS-08 | `feature/waforspeed-33` | PR #18 | ✅ VALIDÉ par qa-lead |
| 4.5 | WFS-34 | Arbres réalistes portés du jeu web (correction de WFS-06) | E2 | ✅ Terminé | Claude (session moteur-natif), 2026-11-02 | WFS-06 | `feature/waforspeed-34` | PR #14 | ✅ VALIDÉ par qa-lead |
| 0.1 | WFS-35 | [MVP] Écran de chargement de l'application (logo, barre de progression, fondu) | E8 | ✅ Terminé |  | WFS-26 | `feature/waforspeed-35` | PR #12 (fusionnée) | 🟡 PASSABLE AVEC ANOMALIES MINEURES (ANO-10) |
| 0.2 | WFS-36 | Point d'extension : écrans de l'application (menu, garage, course, pause…) | E0 | ✅ Terminé | | — | `feature/waforspeed-36` | #13 | ✅ VALIDÉ par qa-lead (e3d7ea4, [rapport](qa/rapports/WFS-36.md)) |
| 0.3 | WFS-37 | [MVP] Menu principal : affichage (logo, choix, record, ta voiture) | E8 | ✅ Terminé (fusionné sur ordre de l'utilisateur, QA après fusion) |  | WFS-35, WFS-36 | `feature/waforspeed-37` | PR #23 (fusionnée) | ⏳ QA après fusion |
| 0.4 | WFS-38 | [MVP] Menu tactile : appuis et départ de course, fin → Rejouer / Menu | E8 | ✅ Terminé | Claude (session trafic), 2026-11-03 | WFS-37, WFS-12 | `feature/waforspeed-38` | #27 (fusionnée) | 🟡 PASSABLE AVEC ANOMALIES MINEURES tour 2/2 (ANO-14) — [rapport](qa/rapports/WFS-38.md) |
| P2 Haute | WFS-39 | Pause en course et retour au menu | E8 | ✅ Fusionné (ordre de l'utilisateur, QA de nuit à faire) | Claude (session trafic), 2026-11-03 | WFS-38 ✅ | `feature/waforspeed-39` | #34 |  |
| — | WFS-40 | Garage : fiche de la voiture et peintures | E5 | ↪ Fusionné dans WFS-11 |  | — | `feature/waforspeed-40` |  |  |
| P2 Haute | WFS-41 | Menus complets : choix du terrain et du moment, réglages (qualité, caméra, son, commandes) | E8 | ✅ Fusionné (par confiance, QA de nuit à faire) | Claude (agent 5, session 011cpqZaDiS2u), 2026-11-03 | WFS-38 ✅ | `feature/waforspeed-41` | [#45](https://github.com/wlfaleh/waforspeed_mobile/pull/45) |  |
| 8 | WFS-42 | Vitrages et jantes : trame de points (lunette arrière de la Golf, rayons des jantes) | E4 | ✅ Terminé | Claude (session moteur-natif), 2026-11-03 | WFS-32 | `feature/waforspeed-42` | PR #24 | ✅ fusion décidée par l'utilisateur (captures sans blocage) |
| — | WFS-43 | Arbres : finitions (cœur du houppier, ombres des arbres) | E2 | ↪ Fusionné dans WFS-53 |  | — | `feature/waforspeed-43` |  |  |
| 0.45 | WFS-44 | Point d'extension : scène dédiée par écran (caméra, fond, brouillard, ce que Game dessine) | E0 | ✅ Terminé |  | WFS-36 | `feature/waforspeed-44` | #15 (fusionnée) | ✅ VALIDÉ par qa-lead (b9771e0, [rapport](qa/rapports/WFS-44.md)) |
| P2 Haute | WFS-45 | Lumières : projecteurs (spots), hémisphère, brouillard linéaire | E4 | ✅ Fusionné (ordre de l'utilisateur, QA de nuit à faire) | Claude (agent 4, session 011cpqZan), 2026-11-03 | — | `feature/waforspeed-45` | #35 |  |
| — | WFS-46 | Compteur de vitesse en arc (zone rouge) | E5 | ↪ Fusionné dans WFS-11 |  | — | `feature/waforspeed-46` |  |  |
| 0.42 | WFS-47 | [MVP] Notifications de course (checkpoint, quasi-contact, accident) | E5 | ✅ Terminé | Claude (session trafic), 2026-11-03 | WFS-27 ✅ | `feature/waforspeed-47` | #25 (fusionnée) | ✅ VALIDÉ par qa-lead (c813a56) — [rapport](qa/rapports/WFS-47.md) |
| P2 Haute | WFS-48 | Texte en italique (ANO-04) | E5 | ✅ Fusionné (ordre de l'utilisateur, QA de nuit à faire) | Claude (agent 5, session 011cpqZaDiS2u), 2026-11-03 | WFS-26 | `feature/waforspeed-48` | [#33](https://github.com/wlfaleh/waforspeed_mobile/pull/33) |  |
| 0.47 | WFS-49 | Banc : captures du chargement en 2400x1080 sans planche de texte (ANO-10) | E7 | ✅ Terminé | Claude (session trafic), 2026-11-03 | WFS-35 ✅ | `feature/waforspeed-49` | #22 (fusionnée) | ✅ VALIDÉ par qa-lead (52648fd) — [rapport](qa/rapports/WFS-49.md) |
| P4 Basse | WFS-50 | [MVP] Vérification simulateur du MVP (parcours complet sur Android) | E7 | 🔎 Fusionné (PR #47, correction du scénario : appuis sur la taille logique du jeu) ; parcours OK au run 41747781094 et sur `main` 14241f4 (Release `simulateur-MVP`, `RÉSULTAT TECHNIQUE : OK`, 10 étapes) ; recette `qa-lead` de nuit | Claude (session trafic), 2026-11-04 | WFS-35 ✅, WFS-27 ✅, WFS-37 ✅, WFS-38 ✅, WFS-29 ✅, WFS-47 ✅, WFS-56 ✅, WFS-57 ✅, WFS-58 ✅, WFS-59 ✅ | `feature/waforspeed-50` |  | ⛔ REFUSÉ (SIM-MVP-1 à 4) — [rapport](qa/rapports/simulateur-MVP.md) |
| 0.43 | WFS-51 | Point d'extension : marges de sécurité de l'écran (encoche), commandes tactiles | E0 | ✅ Terminé | Claude (session trafic), 2026-11-03 | WFS-29 ✅ | `feature/waforspeed-51` | #20 (fusionnée) | ✅ VALIDÉ par qa-lead (66c13bc, banc PC) — [rapport](qa/rapports/WFS-51.md) |
| 0.435 | WFS-52 | HUD et notifications dans les marges de sécurité (encoche) | E5 | ✅ Terminé | Claude (session trafic), 2026-11-03 | WFS-51 ✅, WFS-27 ✅ | `feature/waforspeed-52` | #30 (fusionnée, 318ac8b) | ✅ VALIDÉ par qa-lead (d2b19dc) — [rapport](qa/rapports/WFS-52.md) |
| P2 Haute | WFS-53 | Cartes Campagne et Grand Prix + finitions des arbres | E2 | ✅ Fusionné (QA de nuit à faire), fusion par confiance, PR #61 | Claude (session trafic), 2026-11-04 | WFS-07, WFS-14 ✅, WFS-34 | `feature/waforspeed-53` |  |  |
| — | WFS-54 | Carte Grand Prix (circuit) | E2 | ↪ Fusionné dans WFS-53 |  | — | `feature/waforspeed-54` |  |  |
| 12.05 | WFS-55 | Correction : PI en double dans le tracé de la route (test_cartes à 1e-9, captures renouvelées) | E3 | ✅ Terminé | Claude (session facile), 2026-11-03 | WFS-14 ✅ | `feature/waforspeed-55` | #26 (fusionnée) | ✅ fusion rapide (CI verte, banc non terminé) — recette de nuit : `qa/Check.md` |
| 0.441 | WFS-56 | [MVP] Simulateur : scénario « parcours MVP » par appuis (étape 1 de WFS-50) | E7 | ✅ Terminé (fusionné par l'utilisateur, QA après fusion) | Claude (session 01UY1Vv1), 2026-11-03 | WFS-31 ✅ (essai complet après WFS-38) | `feature/waforspeed-56` | PR #28 (fusionnée, 94087c7) | ⏳ QA après fusion |
| 0.405 | WFS-57 | [MVP] Correction : centrer la paire « Rejouer » / « Menu » de l'écran de fin (ANO-14) | E3 | ✅ Terminé | Claude (session trafic), 2026-11-03 | WFS-38 ✅ | `feature/waforspeed-57` | #29 (fusionnée) | ✅ VALIDÉ par qa-lead (cc76b01, banc PC, ANO-14 corrigée) — [rapport](qa/rapports/WFS-57.md) |
| P4 Basse | WFS-58 | [MVP] Correction simulateur : scénario `mvp` piloté par l'état du jeu (SIM-MVP-2, 3, 4) | E7 | ✅ Fusionné (ordre de l'utilisateur, QA de nuit à faire) | Claude (session trafic), 2026-11-03 | — | `feature/waforspeed-58` | #32 |  |
| P1 Urgent | WFS-59 | [MVP] Point d'extension : mode GPU logiciel, temps de jeu au temps réel et rendu allégé (SIM-MVP-1) | E0 | ✅ Fusionné (QA de nuit à faire) — gel du simulateur non reproduit au run 41719456821 (WFS-64 suspendu) | agent 3, 2026-11-03 | — | `feature/waforspeed-59` | [#31](https://github.com/wlfaleh/waforspeed_mobile/pull/31) |  |
| P2 Haute | WFS-60 | Roues animées : roues séparées dans les .wfm, rotation et braquage (absorbe WFS-61, WFS-62) | E5 (+ API E0) | ✅ Fusionné (fusion par confiance, QA de nuit à faire) | Claude (agent 4, session 011cpqZan), 2026-11-04 | — | `feature/waforspeed-60` | #48 |  |
| — | WFS-61 | Roues animées : transformation par roue et animation (rotation, braquage) | E0 | ↪ Fusionné dans WFS-60 |  | — | `feature/waforspeed-61` |  |  |
| — | WFS-62 | Roues animées (rotation selon la vitesse, braquage) | E3 | ↪ Fusionné dans WFS-60 (via WFS-61) |  | — | `feature/waforspeed-62` |  |  |
| P3 Moyenne | WFS-63 | Point d'extension : maillages animés par squelette (skinning) | E0 | ✅ Terminé | Claude (session trafic), 2026-11-03 | — | `feature/waforspeed-63` | #39 (fusionnée) | ✅ VALIDÉ par qa-lead (0eb5465) — [rapport](qa/rapports/WFS-63.md) |
| P4 Basse | WFS-64 | Diagnostic : gel intermittent juste après le chargement sur le simulateur | E0 | ⏸ Suspendu (non reproduit au run 41719456821 ; réactivé si un essai `simulateur.yml` sur `main` se fige de nouveau) |  | WFS-59 ✅ | `feature/waforspeed-64` |  |  |
| P3 Moyenne | WFS-65 | Effets de conduite et d'accident (étincelles, fumée, lignes de vitesse, vignette, flash) | E4 | ✅ Fusionné (fusion par confiance, QA de nuit à faire) | Claude (agent 4, session 011cpqZan), 2026-11-04 | — | `feature/waforspeed-65` | #46 |  |
| P2 Haute | WFS-66 | Caméras et vue cockpit (3 vues, volant, champ de vision selon la vitesse) | E3 | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic), 2026-11-03 | — | `feature/waforspeed-66` | #41 (fusionnée, 3994283) |  |
| P3 Moyenne | WFS-67 | Trafic vivant (clignotants, feux, accidents contournés, variété des modèles) | E3 | ✅ Fusionné (fusion par confiance, QA de nuit à faire) | Claude (agent 4, session 011cpqZan), 2026-11-04 | WFS-60 ✅ | `feature/waforspeed-67` | #52 |  |
| — | WFS-68 | Réglages : qualité, caméra, son, commandes | E8 | ↪ Fusionné dans WFS-41 |  | — | `feature/waforspeed-68` |  |  |
| P1 Urgent | WFS-69 | [STABLE] CI : banc d'essai en jobs parallèles (45 → ≈ 10 min), plus de builds inutiles ni annulés | E7 | ✅ Fusionné (QA de nuit à faire) | agent 3 (session 011cpqZd), 2026-11-03 | — | `feature/waforspeed-69` | [#37](https://github.com/wlfaleh/waforspeed_mobile/pull/37) |  |
| P2 Haute | WFS-70 | Ressenti de conduite : modes de direction, chocs (secousse, ralenti, vibration), réduire les mouvements | E3 | ✅ Fusionné (par confiance, QA de nuit à faire) | Claude (session trafic), 2026-11-04 | — | `feature/waforspeed-70` | [#51](https://github.com/wlfaleh/waforspeed_mobile/pull/51) |  |
| P3 Moyenne | WFS-71 | Ville vivante : commerces et terrasses, portique 3D, enseignes | E2 | ✅ Fusionné (fusion par confiance, QA de nuit à faire) (exception : 2e ticket E2, fichiers séparés de WFS-53 : nouveaux fichiers + accroches ≤ 10 lignes) | agent 3 (session 011cpqZd), 2026-11-04 | WFS-06 ✅ | `feature/waforspeed-71` |  |  |
| P2 Haute | WFS-72 | Point d'extension : apparence des personnages (peau, cheveux, vêtements) | E0 | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic), 2026-11-03 | WFS-63 ✅ | `feature/waforspeed-72` | #40 (fusionnée, 4314bf7) |  |
| P3 Moyenne | WFS-73 | Conducteurs dans les voitures (siège mesuré au chargement, buste figé, joueur / trafic / duel) | E0 | ✅ Fusionné (QA de nuit à faire) |  | WFS-19 | `feature/waforspeed-73` | #42 (fusionnée, 710c9fc) |  |
| P2 Haute | WFS-74 | Changement de carte avec écran de chargement, course figée hors de l'écran Course | E0 | ✅ Fusionné (par confiance, QA de nuit à faire) | Claude (agent 5, session 011cpqZaDiS2u), 2026-11-04 | WFS-41 ✅, WFS-35 ✅ | `feature/waforspeed-74` | [#50](https://github.com/wlfaleh/waforspeed_mobile/pull/50) |  |
| P3 Moyenne | WFS-75 | Réglages : commandes gaucher et vibrations | E8 | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic, agent WFS-75), 2026-11-04 | WFS-41 ✅, WFS-70 ✅ | `feature/waforspeed-75` | #65 |  |
| P3 Moyenne | WFS-76 | Ambiances sonores par carte et musique (menu et course) | E6 | ✅ Fusionné (par confiance, QA de nuit à faire) | Claude (agent 5, session 011cpqZaDiS2u), 2026-11-04 | WFS-10 ✅, WFS-28 ✅, WFS-41 ✅ | `feature/waforspeed-76` | [#55](https://github.com/wlfaleh/waforspeed_mobile/pull/55) |  |
| P2 Haute | WFS-77 | Les 20 voitures sans défaut visible (peinture unie Giulia et Elantra, roues de la GT86, contrôle des 20) | E5 | ✅ Fusionné (PR #57, sur ordre de l'utilisateur ; QA de nuit à faire) | Claude (agent 5, session 011cpqZaDiS2u), 2026-11-04 | WFS-13 ✅, WFS-60 ✅ | `feature/waforspeed-77` | [#57](https://github.com/wlfaleh/waforspeed_mobile/pull/57) |  |
| P4 Basse | WFS-78 | iOS : affichage direct dans une surface Metal et démarrage jusqu'au menu | E7 (+ accroche E4 `vk.cpp`) | ✅ Fusionné, PR #64 — surface Metal OK, démarrage bloqué dans `Renderer::init` sur le simulateur | Claude (session trafic, agent WFS-78), 2026-11-04 | WFS-22 ✅ | `feature/waforspeed-78` | #64 | ⛔ REFUSÉ par qa-lead (QA de nuit : pas de menu iOS) — correction : WFS-88 (PR #76, fusion par WFS-103), aucun nouveau ticket — [rapport](qa/rapports/WFS-78.md) |
| P4 Basse | WFS-79 | Son iOS (sortie Core Audio) | E6 (+ accroche E7 `ios.cmake`) | ✅ Terminé — PR #63 | Claude (session trafic, agent WFS-79), 2026-11-04 | WFS-22 ✅, WFS-76 ✅ | `feature/waforspeed-79` | #63 | ✅ VALIDÉ par qa-lead (QA de nuit) — [rapport](qa/rapports/WFS-79.md) |
| P3 Moyenne | WFS-80 | Manette (peintures, réglages au focus) + réglages « Direction » et « Réduire les mouvements » | E8 (+ accroche E5 `garage.cpp`) | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic), 2026-11-04 | WFS-22 ✅, WFS-75 ✅ | `feature/waforspeed-80` | #74 |  |
| P3 Moyenne | WFS-81 | Nuit corniche et circuit : vrais lampadaires, lumières alignées, feux du trafic sans double halo | E2 (+ accroche E3 `traffic_system.cpp`) | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic, agent WFS-81), 2026-11-04 | WFS-09 ✅, WFS-53 ✅, WFS-67 ✅ | `feature/waforspeed-81` | #75 |  |
| P2 Haute | WFS-82 | [STABLE] Performances téléphone : masquage hors champ et au loin, moins d'appels de dessin (Basse ≤ 300) | E4 | ✅ Fusionné (fusion par confiance, QA de nuit à faire) — Basse 331 > 300 : cible assouplie par le PO (Moyenne 337 ≤ 450 atteinte) | Claude (agent 4, session 011cpqZan), 2026-11-04 | WFS-15 ✅, WFS-67 ✅ | `feature/waforspeed-82` | #67 |  |
| P2 Haute | WFS-83 | Duel jouable entre deux téléphones sur le même Wi-Fi : écran Salon, transport réseau local, menu « Duel en ligne » actif | E3 (+ `src/ui/ecrans/salon.*`, accroches E0 / E7 / E8) | ✅ Fusionné (par confiance, QA de nuit à faire) | Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04 | WFS-21 ✅, WFS-24 ✅, WFS-41 ✅, WFS-74 ✅ | `feature/waforspeed-83` | [#68](https://github.com/wlfaleh/waforspeed_mobile/pull/68) |  |
| P1 Urgent | WFS-84 | [URGENT] Nuit : feux des voitures naturels (petits feux nets, lueur douce, fini l'effet artificiel) | E4 | ✅ Fusionné (par confiance, QA de nuit à faire) | Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04 | WFS-09 ✅ | `feature/waforspeed-84` | [#70](https://github.com/wlfaleh/waforspeed_mobile/pull/70) |  |
| P1 Urgent | WFS-85 | [URGENT] Caméra de poursuite plus haute (voir les voitures devant) | E3 | ✅ Fusionné (QA de nuit à faire) | Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04 | WFS-66 ✅ | `feature/waforspeed-85` | [#71](https://github.com/wlfaleh/waforspeed_mobile/pull/71) |  |
| P1 Urgent | WFS-86 | [URGENT] Voitures en course aussi belles qu'au garage (qualité, échelle de rendu, anticrénelage, modèle complet) | E4 | ✅ Fusionné (fusion par confiance, QA de nuit à faire) | Claude (agent 4, session 011cpqZan), 2026-11-04 | — | `feature/waforspeed-86` | #72 |  |
| P2 Haute | WFS-90 | Conduite libre : demi-tour, marche arrière facile, rouler dans l'autre sens | E3 (exception : 2e ticket E3, fichiers séparés de WFS-89) | ✅ Fusionné (fusion par confiance, QA de nuit à faire) | Claude (agent 4, session 011cpqZan), 2026-11-04 | — | `feature/waforspeed-90` | #78 |  |
| P1 Urgent | WFS-87 | [URGENT] CI : l'APK de `main` toujours publié (Release `apercu` fiable, plus de trou ni d'échec 403) | E7 (exception : 2e ticket E7, fichiers séparés de WFS-78) | ✅ Fusionné par l'utilisateur (PR #73, QA de nuit à faire) | Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04 | WFS-69 ✅ | `feature/waforspeed-87` | [#73](https://github.com/wlfaleh/waforspeed_mobile/pull/73) |  |
| P4 Basse | WFS-88 | iOS : `Renderer::init` ne se bloque plus, démarrage jusqu'au menu | E4 | ⏸ Suspendu (GEL iOS) ; **PR #76 fusionnée le 2026-11-05** (1c28ae5, fusion par confiance, étape 0 de WFS-94 ; ordre de l'utilisateur, `tache utilisateur.md`, 2026-11-04 : seule exception au GEL iOS, aucune autre suite iOS) | Claude (session trafic), 2026-11-04 | WFS-78 ✅ | `feature/waforspeed-88` | #76 |  |
| P2 Haute | WFS-89 | Duel par Internet : salon par code à 4 caractères, compatible avec le jeu web (PeerJS) | E3 (+ accroches E8 `salon.*`, E0 `CMakeLists.txt`, E7 manifeste) | ✅ Fusionné (PR #77, revue WFS-103, décision B : duel Wi-Fi local, saisie du code, « Sans Internet » ; vrai réseau PeerJS → WFS-92 ⏸ ; QA de nuit à faire) | Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04 | WFS-83 ✅, WFS-21 ✅ | `feature/waforspeed-89` |  |  |
| P2 Haute | WFS-91 | [STABLE] Endurance : parcours automatique long sans plantage ni fuite, validation Vulkan à zéro erreur, retours d'arrière-plan | E7 | ✅ Fusionné (par confiance, PR #92 ; version allégée du PO, ANO-52 RG-14) | Claude (agent 3, session 011cpqZd), 2026-11-05 | — | `feature/waforspeed-91` | #92 |  |
| P2 Haute | WFS-92 | Duel par Internet : pair PeerJS réel (WebSocket + WebRTC, libdatachannel + Mbed TLS dans `third_party/`) | E3 | ⏸ Remplacé par WFS-112 (décision du PO : serveur de relais, pas de bibliothèques tierces de 14 Mo) |  | WFS-89 ✅ | `feature/waforspeed-92` |  |  |
| P2 Haute | WFS-93 | Ville : les 6 quartiers du jeu web (résidentiel, parc, port) + panneaux du circuit avec texte, vignes sans marches | E2 | ✅ Terminé (fusion par confiance, PR #89) | Copilot (session 0e5c192d), 2026-11-04 | — | `feature/waforspeed-93` | #89 | ⏳ Recette QA de nuit (`qa/Check.md`) |
| P2 Haute | WFS-94 | Corniche : quartiers Port de plaisance, Côte rocheuse, Village de pêcheurs (villas, bateaux, rochers) + ville de nuit : façades sombres et fenêtres allumées (fin d'ANO-37) ; cadrage ANO-40 ; finitions de WFS-93 : trottoir de la ville jusqu'à RW + 6 m (ANO-49), texte lointain des panneaux du circuit (ANO-51) | E2 | ✅ Fusionné (QA de nuit à faire ; fusion par confiance, PR #123, commit a722e6c) | Claude (session trafic, agent reprise), 2026-11-05 (repris de Copilot sur demande de l'utilisateur) | WFS-93 ✅ | `feature/waforspeed-94` | #123 | ⏳ Recette QA de nuit (`qa/Check.md`) |
| — | WFS-95 | Texte et bandeaux : caractères manquants (« — »), petits libellés lisibles, nom du quartier en course | E5 | ↪ Fusionné dans WFS-102 (lot anomalies interface) |  | — | `feature/waforspeed-95` |  |  |
| P2 Haute | WFS-96 | Menus : Showroom et « Ta voiture » ouvrent le garage, quartiers sur la carte du terrain, focus manette, voile sans bandes, sons d'interface | E8 | ✅ Fusionné (PR #79, revue WFS-103 ; ANO-15 et ANO-34 corrigées ; QA de nuit à faire) | Claude (session trafic), 2026-11-04 | — | `feature/waforspeed-96` |  |  |
| P1 Urgent | WFS-97 | Lot anomalies NUIT : taches de lumière du circuit + façades de la ville trop claires (ANO-37) | E4 (exception : en parallèle de WFS-100, fichier séparé `nuit.cpp` ; bloc des feux du joueur laissé à WFS-109 ; + accroche d'une ligne E2 dans `decor_carte.cpp`, RG-08) | ✅ Terminé (fusion par confiance, PR #90) | Copilot (session dc0cdd3f), 2026-11-05 | — | `feature/waforspeed-97` | #90 | ⏳ Recette QA de nuit (`qa/Check.md`) |
| P1 Urgent | WFS-98 | Correction : commandes tactiles du jeu web (GAZ / FREIN / NITRO / volant), HUD de la vue cockpit, caméra qui suit le demi-tour (ANO-31, ANO-24, ANO-30) | E3 (exception : 2e ticket E3, fichiers séparés de WFS-89 ; + E0 `game.cpp`, E5 `hud.*`) | ✅ Fusionné (fusion par confiance, QA de nuit à faire) | Copilot (session 0e5c192d), 2026-11-04 | — | `feature/waforspeed-98` | #80 |  |
| P1 Urgent | WFS-99 | Correction : garage lisible (voile, néons à droite, taches de lumière au sol, voiture de 3/4 avant) (ANO-22, ANO-23, ANO-33) | E5 (exception : 2e ticket E5, fichiers séparés de WFS-95) | ✅ Terminé (fusion par confiance) | Copilot (session 0e5c192d), 2026-11-04 | — | `feature/waforspeed-99` | #81 | 🟡 PASSABLE AVEC ANOMALIES MINEURES (ANO-47 → WFS-102) — [rapport](qa/rapports/WFS-99.md) |
| P1 Urgent | WFS-100 | Correction : carrosseries sans reflets flottants ni taches, bloom sans images fantômes (ANO-21, ANO-28, ANO-27) | E4 (exception : 2e ticket E4, fichiers séparés de WFS-97) | ✅ Fusionné (PR #94, CI de branche verte, QA de nuit à faire) | Claude (session trafic), 2026-11-04 | — | `feature/waforspeed-100` |  |  |
| P1 Urgent | WFS-101 | Correction : étincelles et lignes de nitro visibles, trafic du banc de près, écran de fin aéré (ANO-25, ANO-26, ANO-36) | E4 (exception : 3e ticket E4, fichiers séparés ; + E3 `traffic_system.cpp`, `course_system.*`) | ✅ Terminé (fusion par confiance, PR #84) | Copilot (session dc0cdd3f), 2026-11-04 | — | `feature/waforspeed-101` | #84 | ✅ VALIDÉ par qa-lead (QA de nuit) — [rapport](qa/rapports/WFS-101.md) |
| P1 Urgent | WFS-102 | Lot anomalies INTERFACE : échelle téléphone (dp), biseaux, textes lisibles et caractères manquants (ANO-16, 17, 18, 19, 20, 32, 35, 38, 39, 41, 42, 43, 44, 45, 47) — absorbe WFS-95 | E8 (+ E5 hors navigation du garage (WFS-113), accroches E0 / E7) | ⛔ ANNULÉ sur `main` le 2026-11-05 (revert 741c998, ordre de l'utilisateur) : garage, menu et HUD cassés sur téléphone 2400x1080 ; à refaire avec captures 2400 regardées, jamais par confiance | Claude (session dev 5bx), 2026-11-05 | WFS-96 ✅, WFS-89 ✅, WFS-98 ✅, WFS-99 ✅, WFS-101 ✅, WFS-105 ✅, WFS-108 ✅ | `feature/waforspeed-102` | #118 | ⛔ explication demandée au QA et au PO |
| P1 Urgent | WFS-103 | Revue et fusion des PR en attente (#77 WFS-89, #79 WFS-96, puis #76 WFS-88 iOS sur ordre de l'utilisateur) | E7 (transverse : n'occupe pas l'épic, WFS-91 reste attribuable) | ✅ Terminé — #77 (WFS-89), #79 (WFS-96) et #76 (WFS-88, iOS, fusionnée le 2026-11-05 01:23 UTC) fusionnées ; CI de main verte (run 42006825688) ; plus aucune PR ouverte | Claude (session trafic), 2026-11-04 ; #76 : Copilot (session 0e5c192d), 2026-11-05 | — | `feature/waforspeed-103` (travail sur les branches des PR) | #77, #79, #76 |  |
| P1 Urgent | WFS-104 | [PRIORITÉ ABSOLUE] Voiture en course : qualité identique au garage (2e passage, résolution 100 %, jamais sous le garage) | E4 (exception : 3e ticket E4, fichiers séparés de WFS-100 / WFS-101 : `qualite_*`, `masquage.*`) | ✅ Terminé (fusion par confiance) | Claude (agent 3, session 011cpqZd), 2026-11-04 | WFS-86 ✅ | `feature/waforspeed-104` | #82 | ✅ VALIDÉ par qa-lead (QA de nuit) — [rapport](qa/rapports/WFS-104.md) |
| P1 Urgent | WFS-105 | [PRIORITAIRE] Caméra de poursuite : angle d'environ 40° (voir les voitures devant) + vue Proche qui montre la route (RG-06) | E3 | ✅ Fusionné (fusion par confiance, QA de nuit à faire) | Copilot (session 1ef8d4d6), 2026-11-04 | WFS-85 ✅ | `feature/waforspeed-105` | #85 |  |
| P1 Urgent | WFS-106 | [STABLE] CI : APK de main en ligne ≈ 5 min après chaque fusion (job APK séparé, quota de l'API préservé, délais maximaux) + banc découpé (CI ≈ 2× plus courte) | E7 | ✅ Terminé (PR #88, après 4 CI de branche vertes) | Copilot (session 6be876ef), 2026-11-04 | WFS-87 ✅, WFS-69 ✅ | `feature/waforspeed-106` | #88 | ⏳ Recette QA de nuit (`qa/Check.md`) ; deux fusions de suite à vérifier sur `main` (Journal) |
| P1 Urgent | WFS-107 | [URGENT] Bug : animation bizarre (« feu blanc ») quand on tourne à gauche ou à droite | E4 (exception : prend la place de WFS-101, mêmes fichiers `effets_*` ; séparés de WFS-100 / WFS-109) | ✅ Terminé (fusion par confiance, PR #87) | Copilot (session dc0cdd3f), 2026-11-04 | WFS-101 ✅ | `feature/waforspeed-107` | #87 | ⏳ Recette QA de nuit (`qa/Check.md`) |
| P1 Urgent | WFS-108 | [URGENT] Commandes tactiles : boutons plus grands, frein et marche arrière revus, option gyroscope | E3 (exception : 2e ticket E3, fichiers séparés de WFS-105, demande de l'utilisateur) | ✅ Terminé (fusion par confiance après CI verte) | Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04 | WFS-98 ✅ | `feature/waforspeed-108` | #83 | ✅ VALIDÉ par qa-lead (QA de nuit, construction 564) — [rapport](qa/rapports/WFS-108.md) |
| P1 Urgent | WFS-109 | [URGENT] Feux de freinage réalistes (2e passage) + halos orange des clignotants / feux de détresse trop gros (ANO-48) | E4 (exception : 2e ticket E4 actif, blocs séparés : bloc des feux de `renderer.cpp`, que WFS-100 ne touche pas ; bloc des feux du joueur de `nuit.cpp` ; + E3 `traffic_system.cpp`, bloc des feux ; + nouveaux `feux_modele.*` et accroche ≤ 10 lignes dans `model_loader.cpp`, RG-13) | 🔧 En cours (reprise de Copilot le 2026-11-05 19:36 UTC ; branche Copilot sans commit de travail, à reprendre telle quelle : section « Reprise » des règles) | ✅ Fusionné (QA de nuit à faire ; fusion par confiance, PR #117, commit 4469c5e) | WFS-84 ✅, WFS-101 ✅ | `feature/waforspeed-109` | #117 | ⏳ Recette QA de nuit (`qa/Check.md`) |
| P1 Urgent | WFS-113 | [URGENT] Showroom : les flèches sautent des voitures et l'écran bugue | E5 | ✅ Fusionné (fusion par confiance, QA de nuit à faire) | Claude (agent 3, session 011cpqZd), 2026-11-05 | WFS-11 ✅ | `feature/waforspeed-113` |  |  |
| P2 Haute | WFS-110 | Modes de course : Course à 5 voitures (1 joueur + 4 ordinateurs), Checkpoint à 60 s, duel sans trafic | E3 (+ accroche E8) | ✅ Fusionné (par confiance, PR #91) | Claude (agent 3, session 011cpqZd), 2026-11-05 | WFS-11 ✅, WFS-13 ✅, WFS-21 ✅ | `feature/waforspeed-110` | #91 |  |
| P2 Haute | WFS-111 | Mode Poursuite police (score 100, 10 km, trafic dense, embouteillages, surprises) + étape 0 : retrait du contournement RG-14 de WFS-91 (suite de WFS-100) | E3 (exception : en parallèle de WFS-115, nouveaux fichiers `src/game/poursuite*` ; + accroches E7 de l'étape 0) | ✅ Fusionné (QA de nuit à faire), PR #98 et #100 (carte « Poursuite » disponible) | Claude (session trafic, 2e agent), 2026-11-05 | WFS-110 ✅, WFS-67 ✅ | `feature/waforspeed-111` |  |  |
| P2 Haute | WFS-112 | Duel entre amis par Internet : serveur de relais, code + lien d'invitation, reconnexion, mieux que le web | E3 (+ accroches E7, E8 ; exception : `src/ui/ecrans/salon.*` partagé avec WFS-102) | ✅ Fusionné (QA de nuit à faire), PR #101 | Claude (session trafic, 3e agent), 2026-11-05 | WFS-89 ✅, WFS-110 ✅ | `feature/waforspeed-112` |  |  |
| P3 Moyenne | WFS-114 | CI : banc d'endurance ≤ 15 min, lancé seulement sur `main`, la nuit et à la demande (plus sur chaque branche) | E7 | ✅ Fusionné (par confiance, PR #93) | Claude (agent 3, session 011cpqZd), 2026-11-05 | WFS-91 ✅ | `feature/waforspeed-114` | #93 |  |
| P1 Urgent | WFS-115 | [URGENT] Menus : « Continuer » après le choix d'un mode ramène au menu principal | E8 (exception : en parallèle de WFS-102, fichiers séparés : `src/game/modes_ecran.cpp`, navigation de `src/ui/garage.cpp`) | ✅ Fusionné (QA de nuit à faire), PR #97 | Claude (session trafic), 2026-11-05 | — | `feature/waforspeed-115` |  |  |
| P2 Haute | WFS-116 | Bouton / geste « Retour » du téléphone : recule d'un écran (course → pause) au lieu de fermer le jeu | E7 | ✅ Fusionné (par confiance, PR #95) | Claude (agent 3, session 011cpqZd), 2026-11-05 | — | `feature/waforspeed-116` | #95 |  |
| P2 Haute | WFS-117 | CI : lancer tous les tests CPU automatiquement (test_retour_systeme et test_commandes jamais lancés aujourd'hui) | E7 | ✅ Fusionné (par confiance, PR #96) | Claude (agent 3, session 011cpqZd), 2026-11-05 | WFS-116 ✅ | `feature/waforspeed-117` | #96 |  |
| P1 Urgent | WFS-118 | [URGENT] APK : même signature à chaque build (installer par-dessus sans désinstaller) | E7 | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic), 2026-11-05 | — | `feature/waforspeed-118` | #99 |  |
| P2 Haute | WFS-128 | Configuration du jeu par un fichier configgame.json à la racine (scores, compteurs, joueurs, espaces, caméras, zoom, …) | E0 (+ accroches E3, E4, E5, E7, E8, ≤ 10 lignes par fichier, RG-13 à RG-16) | ✅ Fusionné (par confiance, PR #110) (étape 1 : chargeur + fichier + tests + README ; étape 2 : branchements par zone, une seule CI) | Claude, agent 3 (session 011cpqZd), 2026-11-05 | — | `feature/waforspeed-128` |  |  |
| P2 Haute | WFS-129 | Android prêt pour Google Play 2026 : cible Android 15 (SDK 35), pages mémoire de 16 Ko, bord à bord + banc d'endurance fiable (seuil au début de cycle, Course à 5 et Poursuite au parcours) | E7 (+ accroche ≤ 10 lignes `desktop_main.cpp`) | ✅ Fusionné (par confiance, PR #111) (QA de nuit à faire) | Claude (session trafic), 2026-11-05 | WFS-29 ✅, WFS-91 ✅, WFS-114 ✅, WFS-116 ✅, WFS-118 ✅, WFS-126 ✅ | `feature/waforspeed-129` |  |  |
| P1 Urgent | WFS-130 | Correction : la Course à 5 ne garde plus en mémoire les voitures des courses passées (défaut trouvé par WFS-129) + retrait de `memoire-course5-connue` | E3 (+ accroche E7 `tools/qa/validation_connues.txt`) | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic), 2026-11-05 | WFS-110 ✅, WFS-129 ✅ | `feature/waforspeed-130` | PR #113 | QA de nuit : `qa/Check.md` |
| P2 Haute | WFS-119 | Poursuite police, 2e passage : 2 surprises de plus (camion en travers, ralentisseur), un seul score au HUD, mode Poursuite gardé, coût en Basse + correction ANO-53 (« Rejouer » de la course à 5 bloque l'appui suivant) | E3 (exception : 2e ticket E3 actif à côté de WFS-112, fichiers `src/game/poursuite*` ; accroches `hud.cpp`, `modes_ecran.cpp`, `traffic_system.cpp` hors feux, `course5_system.cpp` ≤ 3 lignes RG-08) | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic, agent WFS-119), 2026-11-05 | WFS-111 ✅, WFS-115 ✅ | `feature/waforspeed-119` | PR #106 |  |
| P3 Moyenne | WFS-120 | Sons de conduite : pétarades au lever de pied, souffle du turbo au passage de rapport, rupteur | E6 | ✅ Fusionné (par confiance, PR #102) | Claude (agent 3, session 011cpqZd), 2026-11-05 | WFS-10 ✅, WFS-28 ✅, WFS-76 ✅ | `feature/waforspeed-120` |  |  |
| P3 Moyenne | WFS-121 | Sons du trafic : passages de voitures (effet Doppler), klaxon quand on frôle ou qu'on colle, rumeur du trafic proche | E6 | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic), 2026-11-05 | WFS-10 ✅, WFS-28 ✅, WFS-67 ✅, WFS-120 ✅ | `feature/waforspeed-121` | #103 |  |
| P3 Moyenne | WFS-122 | Duel entre amis, suite : derniers adversaires au salon (RG-05 bonus de WFS-112), salon à 4 joueurs vérifié en course | E3 (+ `src/ui/ecrans/salon.*`) | ⏳ En attente (E3 occupée par WFS-119 ; `salon.*` par WFS-102) |  | WFS-112 ✅, WFS-119, WFS-102 | `feature/waforspeed-122` |  |  |
| P2 Haute | WFS-123 | Tenue sur téléphone : chauffe (état thermique d'Android → rétrogradation de qualité), 60 images/s stables sur écrans 90 / 120 Hz, 30 images/s dans les menus, performances soutenues | E7 (+ accroches ≤ 10 lignes `qualite_systeme.cpp` / `qualite_auto.h`) | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic), 2026-11-05 | WFS-15 ✅, WFS-86 ✅, WFS-104 ✅, WFS-29 ✅ | `feature/waforspeed-123` | #105 |  |
| P3 Moyenne | WFS-124 | Sons de course, 2e passage : tic du chrono sous 8 s (plus aigu sur 3, 2, 1), bip de combo qui monte, « combo perdu » sur choc | E6 | ✅ Fusionné (par confiance, PR #104) | Claude (agent 3, session 011cpqZd), 2026-11-05 | WFS-28 ✅, WFS-96 ✅, WFS-120 ✅ | `feature/waforspeed-124` |  |  |
| P2 Haute | WFS-125 | Records par mode (Checkpoint, Course à 5, Poursuite) et fantôme de ton record en Checkpoint | E3 (+ accroches `CMakeLists.txt` 1 ligne, `desktop_main.cpp` 2 lignes) | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic, agent WFS-125), 2026-11-05 | WFS-24 ✅, WFS-110 ✅, WFS-119 ✅, WFS-21 ✅ | `feature/waforspeed-125` → PR #108 |  |  |
| P2 Haute | WFS-126 | Rapport de plantage sur le téléphone (Partager au lancement suivant) + banc de chauffe dans la CI de nuit (suite de WFS-123) + croissance lente de la mémoire du banc d'endurance (RG-09) | E7 (+ accroches `src/core/log.h` 10 lignes, `CMakeLists.txt` 2 lignes) | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic, agent WFS-126), 2026-11-05 | WFS-29 ✅, WFS-114 ✅, WFS-123 ✅ | `feature/waforspeed-126` → PR #109 |  |  |
| P3 Moyenne | WFS-127 | Showroom sonore : chaque voiture montrée au garage fait entendre son moteur (démarreur, coup de gaz, ralenti) | E6 (+ accroches ≤ 6 lignes `src/ui/garage.*` accesseur, `CMakeLists.txt`, `desktop_main.cpp`) | ✅ Fusionné (par confiance, PR #107) | Claude (agent 3, session 011cpqZd), 2026-11-05 | WFS-10 ✅, WFS-13 ✅, WFS-96 ✅, WFS-113 ✅ | `feature/waforspeed-127` |  |  |
| P3 Moyenne | WFS-131 | Sirène de police réaliste : vraie sirène continue (wail / yelp), plus forte quand la police approche, effet Doppler, jingles « ARRÊTÉ » / « ÉCHAPPÉ » | E6 (+ accroche E3 ≤ 10 lignes `src/game/poursuite_system.cpp`) | ✅ Fusionné (fusion par confiance, QA de nuit à faire), PR #133, commit b2362b9 | Claude (session 01Us6uCu), 2026-11-06 09:55 UTC | WFS-111 ✅, WFS-119 ✅, WFS-121 ✅ | `feature/waforspeed-131` | #133 | QA de nuit : `qa/Check.md` |
| P2 Haute | WFS-132 | Le jeu annonce une nouvelle version de l'APK au lancement (« Télécharger » en un appui) + boutons de volume sur le son du jeu | E7 | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic), 2026-11-05 | WFS-87 ✅, WFS-118 ✅, WFS-126 ✅ | `feature/waforspeed-132` | PR #114 (fusion par confiance, 894208c) |  |
| P2 Haute | WFS-133 | Course à 5, 2e passage : choix de la difficulté (Facile / Normale / Difficile) + ordinateurs combatifs (rattrapage, aspiration, nitro) | E3 (+ accroche E0 ≤ 10 lignes `src/core/configgame.*`) | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic), 2026-11-05 | WFS-110 ✅, WFS-125 ✅, WFS-128 ✅, WFS-130 ✅ | `feature/waforspeed-133` | #115 |  |
| P1 Urgent | WFS-134 | [PRIORITÉ ABSOLUE] Voitures en course, 3e passage : **plus jamais de voitures en cubes, même en Basse (RG-11, en premier)**, rendu réel affiché dans les Réglages, aucune baisse cachée de résolution (chauffe, rétrogradation), Basse à 75 %, modèle complet et filtrage anisotrope, ombre de contact + ANO-67 (X5, Model 3), ANO-60 (fantôme translucide), ANO-66 (reflet de l'enseigne) | E4 (exception : 2e ticket E4 à côté de WFS-109, blocs séparés ; + accroche E8 `reglages.cpp`) | 🔧 En cours (ajout 2026-11-05 21:01 : RG-11 durcie, aucun cube à aucune distance ; RG-12 transition progressive du LOD ; RG-13 captures par distance avant / après ; RG-14 **pas de fusion par confiance**, verdict `qa-lead` avant fusion ; ANO-66 passée à WFS-142) | ✅ Fusionné (QA de nuit à faire), PR #119, commit 1bf0514 | WFS-104 ✅, WFS-123 ✅, WFS-126 ✅ | `feature/waforspeed-134` |  |  |
| P2 Haute | WFS-135 | Lot rapide anomalies : textes faux du menu et du terrain, vue de caméra « Arrière », panneau du score lisible, captures `parcours-*` publiées, test du relais stable (ANO-55, 58, 61, 63, 64) | E7 (+ accroches ≤ 10 lignes E3 / E5 / E8) | ✅ Fusionné (QA de nuit à faire) | Claude (session dev expert, 0183bv2X), 2026-11-05 20:01 UTC | — | `feature/waforspeed-135` | #116 |  |
| P2 Haute | WFS-136 | Tracé des cartes, 2e version (catalyseur) : virages serrés, épingles, montagne, ronds-points, carrefours à feux (trafic arrêté au rouge), autoroutes, tunnels, ponts ; records mis de côté par version de tracé | E3 | ✅ Fusionné (QA de nuit à faire) | Claude (session dev expert, 0183bv2X), 2026-11-05 20:37 UTC | WFS-133 ✅ | `feature/waforspeed-136` | #128 | ❌ REFUSÉ (QA de nuit 2026-11-06, ANO-88 bloquante : CI de main rouge) → correction WFS-169 ; [rapport](qa/rapports/WFS-136.md) |
| P2 Haute | WFS-137 | Ville : rond-point à fontaine, carrefours à feux et piétons, autoroute urbaine, tunnel, pont, casino, centre commercial, cafés, bars, garages, pigeons | E2 | 🔧 En cours (exception PO : 2e ticket E2 à côté de WFS-140, fichiers séparés : `ville.cpp`, `ville_quartiers.*` contre `circuit.cpp`) | Claude (agent 4, session 011cpqZan), 2026-11-06 12:00 UTC | WFS-136 ✅, WFS-94 ✅ | `feature/waforspeed-137` |  |  |
| P2 Haute | WFS-138 | Campagne : col de montagne en épingles, animaux de ferme, village (rond-point, feu, café, bar, garage), autoroute et péage + vignes continues (ANO-65) | E2 | ⏳ Prêt (débloqué le 2026-11-06 : WFS-136 ✅, WFS-94 ✅ ; E2 occupée par WFS-140) |  | WFS-136 ✅, WFS-94 ✅ | `feature/waforspeed-138` |  |  |
| P2 Haute | WFS-139 | Corniche : route en falaise et tunnel, beach parties, jeux de plage, bateaux et jet-skis, bars de plage, casino de station, mouettes et dauphins | E2 | ⏳ Prêt (débloqué le 2026-11-06 : WFS-136 ✅, WFS-94 ✅ ; E2 occupée par WFS-140) |  | WFS-136 ✅, WFS-94 ✅ | `feature/waforspeed-139` |  |  |
| P2 Haute | WFS-140 | Grand Prix : épingle, chicanes, raidillon, feux de départ, tribunes pleines, paddock et garages des équipes, bars d'accueil, fan zone | E2 | 🔧 En cours — Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-06, branche feature/waforspeed-140 (réattribué par le PO le 2026-11-06 12:00 UTC : aucun commit de travail de Claude (session dev expert, 0183bv2X) depuis l'attribution de 08:21, ni au Journal depuis 08:18 ; détail au Journal) | Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-06 12:00 UTC (avant : Claude (session dev expert, 0183bv2X), 08:21 UTC, libéré) | WFS-136 ✅, WFS-94 ✅ | `feature/waforspeed-140` |  |  |
| P2 Haute | WFS-141 | Lot anomalies Poursuite et écrans : gyrophares, bandeau d'annonce, fins Course à 5 / Poursuite au style Checkpoint, panneau des modes, salon hors ligne (ANO-54, 56, 57, 59, 62) | E3 (exception PO : 2e ticket E3 actif à côté de WFS-136, fichiers disjoints ; + accroches E8 `salon.cpp`, `menu.cpp`) | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic), 2026-11-06 06:48 UTC | WFS-133 ✅, WFS-131 (souple : l. 228-236 de `poursuite_system.cpp` seulement tant que WFS-131 n'est pas fusionné) | `feature/waforspeed-141` | #130 |  |
| P1 Urgent | WFS-142 | [CRITIQUE] Route de nuit : fin de la chaussée miroir — asphalte jamais métallique, sec par défaut (mouillé seulement sous la pluie), reflet de la scène pondéré et flou, Fresnel plafonné, bloom et flaques de lumière revus + ANO-66 ; captures avant / après, **pas de fusion par confiance** | E4 (exception : 2e ticket E4 actif à côté de WFS-134 ; WFS-109 fusionné le 2026-11-05 20:59 ; blocs séparés, RG-11) | ✅ Fusionné par confiance (PR #120, eeec455, 2026-11-05 ; QA de nuit : `qa/Check.md`) | Claude (session 0183bv2V), 2026-11-05 21:01 UTC | WFS-09 ✅, WFS-97 ✅, WFS-100 ✅ | `feature/waforspeed-142` | #120 | #120 | ✅ VALIDÉ (QA de nuit 2026-11-06, [rapport](qa/rapports/WFS-142.md)) ; décision PO : RG-04 gardée, RG-08 mesure d'information ; résolu quand l'utilisateur l'a confirmé sur son téléphone (RG-10) 
| P1 Urgent | WFS-143 | [CI ROUGE] Reporter dans le code les valeurs de `configgame.json` de l'utilisateur (e23fa51 : score de poursuite 1000, caméra Poursuite 10 / -4.5 / zoom 64) et relire les records de poursuite > 100 (`sauvegarde.cpp` l. 150) | E0 | ✅ Terminé (contenu fusionné hors ticket par la PR #121, d1fa824 ; d07f704, 6b49595 ; CI de main verte au run 679) |  | — | `feature/waforspeed-143` | #121 |  |
| P2 Haute | WFS-144 | Correction : poursuite en Basse trop chère en appels de dessin (+149 → ≤ 60 par rapport à Checkpoint : matériaux fusionnés, feux de police groupés ou police `-lod` en Basse, jamais de cubes) + Basse conforme à WFS-134 (MSAA ×2, aniso ×4) | E4 (+ accroche E3 `poursuite_system.cpp`) | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic), 2026-11-06 00:19 UTC | WFS-134 ✅ | `feature/waforspeed-144` | PR #124 | QA de nuit : `qa/Check.md` |
| P1 Urgent | WFS-145 | [CI ROUGE] Job `endurance` de `main` en échec depuis WFS-94 (run 680) : Poursuite bloquée à 9,87 km à 0 km/h, 3 écarts au parcours, mémoire +10,7 % (seuil 10 %) | E7 (+ `src/world/**` E2, accroche ≤ 10 lignes E3 annoncée) | 🔧 En cours | Claude (session dev 5bx), 2026-11-06 00:56 UTC | WFS-94 ✅ | `feature/waforspeed-145` |  |  |
| P1 Urgent | WFS-146 | Lot anomalies INTERFACE, 2e passage (1/2) : textes, « — », couleurs, biseaux, commandes tactiles, SANS changer l'échelle (ANO-16, 20, 32, 35, 39, 41, 43, 44, 45) — reprend les morceaux sûrs de WFS-102 | E8 (+ E5 `text.cpp`, `toasts.cpp`, `hud.cpp`, `shaders/ui*`) | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic), 2026-11-06 02:38 UTC | — | `feature/waforspeed-146` | #125 |  |
| P2 Haute | WFS-147 | Lot anomalies INTERFACE, 2e passage (2/2) : tailles lisibles sur téléphone, écran par écran, sans échelle globale, verdict QA avant fusion (ANO-17, 18, 19, 38, 42, 47) | E8 (+ E5 `src/ui/**`) | ✅ Fusionné (QA VALIDÉ avant fusion) | Claude (session trafic), 2026-11-06 04:11 UTC | WFS-146 ✅ | `feature/waforspeed-147` | #126 | VALIDÉ |
| P2 Haute | WFS-148 | [RÉALISME 0.1] Référentiel du réel et banc de fidélité : données sourcées, 12 vues de référence, mesures automatiques, état zéro | E0 (+ nouveaux `references/reel/**`, `tests/fidelite_*`, `src/render/bench_realisme.*`) | ✅ Terminé (fusion par confiance, QA de nuit à faire) | expert-realisme-3d (session 0183bv2V), 2026-11-06 01:13 UTC | — | `feature/waforspeed-148` | #127 | à recetter (`qa/Check.md`) |
| P2 Haute | WFS-149 | [RÉALISME 0.2] Socle : textures compressées KTX2 (ASTC / ETC2) + mipmaps, tangentes MikkTSpace, shaders découpés par domaine | E0 | 🔍 PR #132, CI en cours | Claude (session trafic), 2026-11-06 09:40 UTC (4e ticket de l'agent sur ordre de l'utilisateur « avancer vite sur plusieurs sujets » ; WFS-141, 170 actifs, 158 en pause) | WFS-148 ✅ | `feature/waforspeed-149` | #132 |  |
| P2 Haute | WFS-150 | [RÉALISME 0.3] Socle : passes de rendu enfichables, pré-passe, capture d'environnement, niveau Ultra, coût GPU par passe | E0 | ⏳ En attente |  | WFS-149 validée | `feature/waforspeed-150` |  |  |
| P2 Haute | WFS-151 | [RÉALISME 1.1] Lumière : exposition physique (EV100, lux), IBL préfiltrée, sondes locales, étalonnage charte grise / ColorChecker | E13 | ⏳ En attente |  | WFS-150 validée | `feature/waforspeed-151` |  |  |
| P2 Haute | WFS-152 | [RÉALISME 1.2] Lumière : ciel physique, soleil et lune calculés (NOAA), 6 moments + heure réelle, brouillard de distance | E13 (+ accroche E8 `terrain.cpp`) | ⏳ En attente |  | WFS-151 validée | `feature/waforspeed-152` |  |  |
| P2 Haute | WFS-153 | [RÉALISME 1.3] Lumière : ombres en cascades à pénombre physique, ombres des lumières la nuit, occlusion ambiante, contact | E13 | ⏳ En attente |  | WFS-151 validée (après WFS-152, même épic) | `feature/waforspeed-153` |  |  |
| P2 Haute | WFS-154 | [RÉALISME 1.4] Lumière : reflets d'écran (SSR), bloom par énergie, flou de mouvement par objet, TAA, phares de face | E13 | ⏳ En attente |  | WFS-153 validée | `feature/waforspeed-154` |  |  |
| P2 Haute | WFS-155 | [RÉALISME 2.R1] Routes : revêtements réels scannés et calibrés par carte, usure, bordures, sans répétition visible | E9 (+ accroche `track.cpp`) | ⏳ En attente |  | WFS-151 validée | `feature/waforspeed-155` |  |  |
| P2 Haute | WFS-156 | [RÉALISME 2.R2] Routes : marquages réglementaires usés et rétroréfléchissants, fissures, nids-de-poule, plaques, traces | E9 | ⏳ En attente |  | WFS-155 validée | `feature/waforspeed-156` |  |  |
| P2 Haute | WFS-157 | [RÉALISME 2.R3] Routes : états de surface (sec, humide, mouillé, flaques, sable, feuilles), projections d'eau | E9 (+ accroche `meteo.cpp`) | ⏳ En attente |  | WFS-156, WFS-154 validées | `feature/waforspeed-157` |  |  |
| P2 Haute | WFS-158 | [RÉALISME 2.V1] Véhicules : dimensions exactes des 20 voitures (fiches constructeur ±1 %), assise, pièces manquantes, LOD 4 niveaux | E10 | 🔧 En cours — **rendu à Claude (session trafic) le 2026-11-06 11:07 UTC (PO)** : son travail local (6c78a60, 5d9e379 : mesure, corrections, `assise.json`, LOD allégés, indice 57,4 % → 93,1 %) est à **pousser dans l'heure** sur `feature/waforspeed-158` après `git merge origin/feature/waforspeed-158` (ST-1 de la session 01Us6uCu, e507961, déjà dessus) ; ST-2 et suite après la fusion de WFS-149 ; décisions RG-04 / RG-05 au Journal 11:07:42 | Claude (session trafic), 2026-11-06 11:07 UTC (ST-1 : Claude (session 01Us6uCu), libéré) | WFS-148 ✅ (fusionné, décision PO : suffit), WFS-144 ✅ | `feature/waforspeed-158` |  |  |
| P2 Haute | WFS-159 | [RÉALISME 2.V2] Véhicules : peinture multicouche (flocons, vernis), verre physique, métaux, pneus, phares à optique | E10 | ⏳ En attente |  | WFS-158, WFS-151 validées | `feature/waforspeed-159` |  |  |
| P2 Haute | WFS-160 | [RÉALISME 2.V3] Véhicules : suspension visible (roulis, plongée), roues, salissures, gouttes, dégâts légers | E10 (+ lecture E3) | ⏳ En attente |  | WFS-159 validée | `feature/waforspeed-160` |  |  |
| P2 Haute | WFS-161 | [RÉALISME 3.E1] Environnements : kit d'architecture (6 styles), fenêtres en profondeur, intérieurs la nuit, matériaux scannés | E12 (+ accroches cartes) | ⏳ En attente |  | WFS-151 validée | `feature/waforspeed-161` |  |  |
| P2 Haute | WFS-162 | [RÉALISME 3.E2] Environnements : signalisation réglementaire, feux tricolores, lampadaires réels, glissières, ponts, tunnels | E12 | ⏳ En attente |  | WFS-161 validée | `feature/waforspeed-162` |  |  |
| P2 Haute | WFS-163 | [RÉALISME 3.E3] Environnements : végétation par espèce, herbe au vent, terrain mélangé, montagnes, mer à vagues physiques | E12 | ⏳ En attente |  | WFS-162, WFS-153 validées | `feature/waforspeed-163` |  |  |
| P2 Haute | WFS-164 | [RÉALISME 4.P1] Personnages : humains réalistes, 12 morphotypes, vêtements par lieu, peau, cheveux, LOD | E11 | ⏳ En attente |  | WFS-151 validée | `feature/waforspeed-164` |  |  |
| P2 Haute | WFS-165 | [RÉALISME 4.P2] Personnages : animations en capture de mouvement, mélanges, pieds au sol (IK), regard | E11 | ⏳ En attente |  | WFS-164 validée | `feature/waforspeed-165` |  |  |
| P2 Haute | WFS-166 | [RÉALISME 4.P3] Personnages : comportements (trajets, feux, groupes, réactions, densité par lieu et heure, spectateurs) | E11 (+ lecture E3) | ⏳ En attente |  | WFS-165 validée | `feature/waforspeed-166` |  |  |
| P2 Haute | WFS-167 | [RÉALISME 5] Harmonisation et validation finale par carte : rapport complet ≥ 99 %, recette, confirmation de l'utilisateur | E13 | ⏳ En attente |  | WFS-148 à WFS-166 validées | `feature/waforspeed-167` |  |  |
| P1 Urgent | WFS-168 | [URGENT] Retour en arrière de la taille des boutons d'avant WFS-147 (seulement la taille des boutons, le reste de WFS-147 gardé) ; ensuite plus personne ne touche à la taille des boutons jusqu'au test du QA avec émulateurs (ordre de l'utilisateur) | E8 (+ E5 `src/ui/**`) | ✅ Fusionné (QA de nuit à faire) — PR #129, ce97402 | Claude (session trafic), 2026-11-06 08:13 UTC (exception PO : P1 passe devant son WFS-141) | — | `feature/waforspeed-168` |  |  |
| P1 Urgent | WFS-169 | [URGENT] Correction : CI de `main` rouge depuis WFS-136 — contrôle des records du banc aligné sur RG-07 (`best_v1`) (ANO-88) | E3 (exception PO : 2e ticket E3 actif à côté de WFS-141, `benchRecords` seulement) | ✅ Fusionné (QA de nuit à faire) — fusion par confiance, commit eadeae8 | Claude (session trafic), 2026-11-06 10:41 UTC | — | `feature/waforspeed-169` | #134 |  |
| P1 Urgent | WFS-170 | [URGENT] Correction des voitures : peinture de la 911, Giulia (bande grise, vitres), lunette de la S3, carrosseries froissées (Model 3, X5, 208, Compass), feux arrière rouges des 20 voitures (ANO-71 à 77) | E4 (+ coordination WFS-158 : `model_loader.cpp`, blocs séparés) | 🔍 PR ouverte, CI de branche en cours (fusion après WFS-169 sur main) | Claude (session trafic), 2026-11-06 09:12 UTC (WFS-158 en pause, même agent) | — (captures `vitrine-*` sur la CI après WFS-169) | `feature/waforspeed-170` | #131 |  |
| P3 Moyenne | WFS-171 | Correction : lueur des feux au sol la nuit (disque net) et feux de recul de la Golf hors des blocs (ANO-70, ANO-78) | E4 | ⏳ En attente |  | WFS-170 | `feature/waforspeed-171` |  |  |
| P3 Moyenne | WFS-172 | Correction : captures du banc qui ne prouvent pas leur critère — `garage-course`, `poursuite-camion`, `poursuite-police-basse`, `trace-feux-rouge` (ANO-80, ANO-87, ANO-89) | E3 | 🔧 En cours | Claude (session trafic), 2026-11-06 11:25 UTC (WFS-170, 149 en revue ; ordre de l'utilisateur « avancer vite ») | WFS-169 ✅ | `feature/waforspeed-172` |  |  |
| P2 Haute | WFS-173 | Mini-carte du parcours au HUD (idée du PO validée par l'utilisateur le 2026-11-06) : route autour du joueur en Checkpoint et en Poursuite (checkpoint, police, barrages), parcours entier en Course à 5 et en Duel (adversaires, partie parcourue, arrivée) ; ce n'est pas un bouton, aucun bouton ne change | E5 (nouveaux `src/ui/minicarte.*`, `tests/test_minicarte.cpp` ; accroches annoncées : E3 ≤ 10 lignes `course5.h` / `course5_system.cpp` / `poursuite.h` / `poursuite_system.cpp`, 1 ligne `hud.cpp` (`benchHud`), 1 ligne `CMakeLists.txt`, 1 ligne `build.yml`) | 🔧 En cours | ✅ Fusionné (fusion par confiance, QA de nuit à faire), PR #135, commit 56f5d47 | — (WFS-27 ✅, WFS-52 ✅, WFS-110 ✅, WFS-112 ✅, WFS-119 ✅) | `feature/waforspeed-173` |  | #135 | QA de nuit : `qa/Check.md` 
| P3 Moyenne | WFS-174 | Sons de la route : vent et roulement des pneus selon la vitesse, bande rugueuse au bord, herbe / gravier hors route, chuintement sur chaussée mouillée | E6 (+ accroche ≤ 3 lignes `CMakeLists.txt`) | 🔧 En cours | Claude, agent 3 (session 011cpqZd), 2026-11-06 12:01 UTC | — (WFS-28 ✅, WFS-76 ✅, WFS-121 ✅, WFS-131 ✅) | `feature/waforspeed-174` |  |  |

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

**WFS-52 — HUD dans les marges de sécurité (encoche)** · E5 · priorité 0.435 · ✅ Terminé (PR #30 fusionnée, VALIDÉ) — Claude (session trafic), 2026-11-03 (règles : `regles-de-gestion/WFS-52.md`)
- [ ] `encoche-hud` : aucun panneau du HUD dans la bande ; sans encoche, captures identiques

**WFS-56 — [MVP] Simulateur : scénario « parcours MVP » par appuis** · E7 · priorité 0.441 · ✅ Terminé (PR #28 fusionnée par l'utilisateur, QA après fusion) — Claude (session 01UY1Vv1), 2026-11-03 (règles : `regles-de-gestion/WFS-56.md`)
- [ ] `simulateur.yml` avec `scenario=mvp` sur `feature/waforspeed-56` : 10 captures nommées (RG-02), `01-chargement` et `02-menu` corrects, appuis et coordonnées dans `deroulement.txt`, `RÉSULTAT TECHNIQUE : OK`
- [ ] `scenario=conduite` (défaut) : mêmes 7 captures qu'avant ; aucun changement de `src/` ni `android/`

**WFS-58 — [MVP] Correction simulateur : scénario piloté par l'état du jeu (SIM-MVP-2, 3, 4)** · E7 · priorité P4 Basse · ✅ Fusionné le 2026-11-03 sur ordre de l'utilisateur (QA de nuit : `qa/Check.md`) — Claude (session trafic), 2026-11-03 (règles : `regles-de-gestion/WFS-58.md`)
- [ ] scénario `mvp` : chaque étape attend sa ligne du logcat (délai max), capture après présentation d'images, `ÉCHEC` au lieu d'un faux `OK` si une étape manque
- [ ] `01-chargement` = barre du jeu sans barres système, `02-menu` = menu ; écran d'émulateur réduit (`wm size`) ; `scenario=conduite` inchangé

**WFS-59 — [MVP] Point d'extension : mode GPU logiciel (SIM-MVP-1)** · E0 · priorité P1 Urgent · ✅ Fusionné le 2026-11-03 (PR #31, QA de nuit : `qa/Check.md`) — agent 3, 2026-11-03 (règles : `regles-de-gestion/WFS-59.md`)
- [ ] GPU logiciel ou extra `qa_simu` : dt max 0,5 s, rendu 25 %, sans ombres ; logcat `QA ips` ≥ 1, course qui avance sur les captures du simulateur
- [ ] banc PC : captures identiques à `main`, `0 écart(s)`

**WFS-64 — Diagnostic : gel intermittent juste après le chargement sur le simulateur** · E0 · priorité P4 Basse · ⏸ Suspendu tant que le gel ne se reproduit pas (règles : `regles-de-gestion/WFS-64.md`)
- [ ] gel reproduit puis expliqué par les lignes de diagnostic (mise à jour ou rendu) ; 3 essais `scenario=mvp` de suite sans gel
- [ ] banc PC : captures identiques à `main`, `0 écart(s)`

**WFS-50 — [MVP] Vérification simulateur du MVP** · E7 · priorité P4 Basse · 🔎 Fusionné, PR #47 (REFUSÉ au 1er passage, [rapport](qa/rapports/simulateur-MVP.md) ; échec du 2e passage sur `main` b01ae36 : barre de navigation de l'émulateur affichée, gaz appuyé sous la fenêtre du jeu, corrigé dans `tools/qa/emulateur.sh` ; parcours complet OK au run 41747781094 ; relancé sur `main`, recette `qa-lead` de nuit) (règles : `regles-de-gestion/WFS-50.md`)
- [ ] `simulateur.yml` (`epic=MVP`) : chargement → menu → Course solo → décompte → course + HUD → fin → Rejouer puis Menu ; paysage plein écran, aucun plantage
- [ ] rapport `qa/rapports/simulateur-MVP.md` VALIDÉ par `qa-lead`

**WFS-49 — Banc : captures du chargement 2400 sans planche de texte (ANO-10)** · E7 · priorité 0.47 · ✅ Terminé (Claude, session trafic, PR #22) (règles : `regles-de-gestion/WFS-49.md`)
- [ ] `chargement-*-2400` sans panneau « DÉMO DU TEXTE » ; `texte-essai-2400` inchangée

**WFS-48 — Texte en italique (ANO-04)** · E5 · priorité P2 Haute · ✅ Fusionné le 2026-11-03 sur ordre de l'utilisateur (QA de nuit : `qa/Check.md`) — Claude (agent 5), 2026-11-03 (règles : `regles-de-gestion/WFS-48.md`)
- [ ] décompte, écran de fin et HUD en italique (12°) ; `texte-essai-*` : ligne droite + ligne italique ; autres captures inchangées

**WFS-46 — Compteur de vitesse en arc** · E5 · priorité P2 Haute · ⏳ Prêt (règles : `regles-de-gestion/WFS-46.md`) — ↪ fusionné dans WFS-11
- [ ] `hud-arc-120`, `hud-arc-320` (zone rouge) en 1280x720 et 2400x1080 ; banc `0 écart(s)`

**WFS-44 — Point d'extension : scène dédiée par écran** · E0 · priorité 0.45 · ✅ Terminé (PR #15) (règles : `regles-de-gestion/WFS-44.md`)
- [ ] Captures du banc identiques à `main` (aucune scène dédiée par défaut), tests à 0 écart
- [ ] Capture `scene-dediee-vide` : fond uni `#07070c`, ni route, ni ville, ni trafic, ni voiture du joueur, ni commandes ; cube repère vu en FOV 36°
- [ ] Doc du point d'extension dans le skill `waforspeed-natif`

**WFS-45 — Lumières : projecteurs (spots), hémisphère, brouillard linéaire** · E4 · priorité P2 Haute · ✅ Fusionné le 2026-11-03 sur ordre de l'utilisateur (QA de nuit : `qa/Check.md`) — Claude (agent 4, session 011cpqZan), 2026-11-03 (règles : `regles-de-gestion/WFS-45.md`)
- [ ] Captures existantes identiques à `main` (0 spot par défaut)
- [ ] Capture `spots-sol` : 3 taches orange / cyan / blanche aux bords adoucis, hémisphère bleutée, brouillard linéaire, voiture éclairée
- [ ] Coût de la passe de scène ≤ +0,3 ms avec 3 spots (journal du banc)

**WFS-11 — Garage complet : plateau, 20 voitures, 16 peintures, fiche et compteur en arc** · E5 · priorité P2 Haute · ✅ Fusionné le 2026-11-04 (PR #43, QA de nuit : `qa/Check.md` ; étape 4, 16 voitures, à reprendre) — agent 3 (session 011cpqZd), 2026-11-03, branche `feature/waforspeed-11` (règles : `regles-de-gestion/WFS-11.md` + WFS-40, WFS-13, WFS-46)
- [ ] `garage-*` : plateau, anneau néon, projecteurs, 3 angles ; `garage-golf` / `garage-m4` : changement de voiture, nom et index

**WFS-39 — Pause en course et retour au menu** · E8 · priorité P2 Haute · ✅ Fusionné le 2026-11-03 sur ordre de l'utilisateur (QA de nuit : `qa/Check.md`) — Claude (session trafic), 2026-11-03 (règles : `regles-de-gestion/WFS-39.md`)
- [ ] `pause-bouton`, `pause-0` (3 boutons), `pause-reprise` (chrono conservé), `pause-quitter` (menu)

**WFS-40 — Garage : fiche de la voiture et peintures** · E5 · priorité P2 Haute · ⏳ En attente (règles : `regles-de-gestion/WFS-40.md`) — ↪ fusionné dans WFS-11
- [ ] `garage-fiche` (fiche + crédit), `garage-peinture-0/1`, course avec la voiture et la peinture choisies

**WFS-41 — Menus complets : terrain, moment, réglages** · E8 · priorité P2 Haute · ✅ Fusionné le 2026-11-04 (PR #45, par confiance ; QA de nuit : `qa/Check.md`) — Claude (agent 5), 2026-11-03, branche `feature/waforspeed-41` (règles : `regles-de-gestion/WFS-41.md`)
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


**WFS-09 — Nuit complète : phares, éclairage public, route mouillée et reflets** · E4 · priorité P3 Moyenne · ✅ Fusionné (QA de nuit à faire) le 2026-11-04 par confiance (PR #59) — Claude (session trafic) (règles : `regles-de-gestion/WFS-09.md`, `regles-de-gestion/WFS-16.md`)
- [ ] Phares réalistes (leçons de la version web)
- [ ] Lampadaires, mode nuit par carte



**WFS-14 — Cartes : socle multi-cartes et Corniche (plage)** · E2 · priorité 12 · ✅ Terminé (règles : `regles-de-gestion/WFS-14.md`, redécoupé)
- [ ] `carte-plage-jour` / `carte-plage-nuit` : océan, palmiers, parasols, sable, ambiance de la plage ; autres captures identiques
- [ ] test de fidélité du tracé des 3 nouvelles cartes (0 écart) ; ≤ 300 appels de dessin

**WFS-55 — Correction : PI en double dans le tracé** · E3 · priorité 12.05 · ✅ Terminé (PR #26) (règles : `regles-de-gestion/WFS-55.md`)
- [ ] `test_cartes` en tolérance 1e-9 ; captures renouvelées listées, sans différence visible

**WFS-53 — Cartes Campagne et Grand Prix + finitions des arbres** · E2 · priorité P2 Haute · ✅ Fusionné (QA de nuit à faire), fusion par confiance (PR #61, 2026-11-04) — Claude (session trafic) (règles : `regles-de-gestion/WFS-53.md`)
- [ ] `carte-campagne-jour` / `-nuit` : collines, champs, haies, fermes, éoliennes ; ≤ 300 appels de dessin

**WFS-54 — Carte Grand Prix (circuit)** · E2 · priorité P3 Moyenne · ✅ Fusionné avec WFS-53 (PR #61) (règles : `regles-de-gestion/WFS-54.md`) — ↪ fusionné dans WFS-53
- [ ] `carte-circuit-jour` / `-nuit` : vibreurs, tribunes, murs de pneus, gravier ; ≤ 300 appels de dessin

**WFS-13 — Les 16 autres voitures (rouvert après WFS-11)** · E5 · priorité P2 Haute · ✅ Fusionné (fusion par confiance, QA de nuit à faire) — agent 3 (session 011cpqZd), 2026-11-04, branche `feature/waforspeed-13` (règles : `regles-de-gestion/WFS-13.md`, section « Réouverture du 2026-11-04 »)
- [ ] `vitrine-<id>-0` pour les 16 voitures : bon sens, 4 roues au sol, peinture, feux arrière non blancs
- [ ] garage : 20 voitures ; `garage-<id>` sur 2 nouvelles ; course avec une nouvelle voiture sans plantage
- [ ] tailles des `.wfm` et de l'APK au Journal ; autres captures inchangées

**WFS-15 — Qualité, performances et post-traitement (réglages téléphone, LOD, bloom, TAA, flou)** · E4 · priorité P2 Haute · ✅ Fusionné le 2026-11-04 par confiance (PR #56, QA de nuit : `qa/Check.md`) — Claude (agent 4, session 011cpqZan), 2026-11-04, branche `feature/waforspeed-15` (règles : `regles-de-gestion/WFS-15.md`, section « Recadrage du PO (2026-11-04) »)
- [ ] Niveaux bas / moyen / haut, choix automatique

**WFS-28 — Bruitages : chocs, nitro, crissements, ambiance** · E6 · priorité P3 Moyenne · ✅ Fusionné le 2026-11-03 sur ordre de l'utilisateur (QA de nuit : `qa/Check.md`) — agent 3 (session 011cpqZd), 2026-11-03 (règles : `regles-de-gestion/WFS-28.md`)
- [ ] journal du banc : chaque événement (accident, petit choc, frottement, quasi-contact, nitro, bips décompte / GO / checkpoint / fin, crissement) déclenche son son une seule fois
- [ ] pas de craquement au début / à la fin des sons ; coupure en pause ; captures du banc inchangées, `0 écart(s)`

**WFS-20 — Performances : LOD, tri des objets visibles en parallèle** · E4 · priorité P3 Moyenne · ⏳ Prêt — ↪ fusionné dans WFS-15
- [ ] 60 i/s visés, mesures dans le journal

**WFS-19 — Personnages : piétons animés (ville et plage)** · E2 · priorité P2 Haute · ✅ Fusionné (PR #42, QA de nuit à faire) (Claude, session trafic, 2026-11-03), branche `feature/waforspeed-19` (règles : `regles-de-gestion/WFS-19.md`)
- [ ] ville : piétons sur les trottoirs, poses variées ; plage : promeneurs ; deux instants : poses de marche différentes

**WFS-73 — Conducteurs dans les voitures** · E0 · priorité P3 Moyenne · ⏳ En attente de WFS-19 (règles : `regles-de-gestion/WFS-73.md`)
- [ ] conducteur face au volant chez le joueur et dans le trafic, bien placé sur deux modèles de hauteurs différentes ; banc `0 écart(s)`

**WFS-63 — Point d'extension : maillages animés par squelette (skinning)** · E0 · priorité P3 Moyenne · ✅ Terminé (PR #39, VALIDÉ) — Claude (session trafic), 2026-11-03 (règles : `regles-de-gestion/WFS-63.md`)
- [ ] capture `squelette-essai` : `homme` du jeu web debout, proportions justes ; `squelette-marche-0` / `-1` : deux poses de `Walk_Loop` différentes
- [ ] captures existantes identiques à `main`, banc `0 écart(s)` ; doc du point d'extension dans le skill `waforspeed-natif`

**WFS-43 — Arbres : finitions (cœur du houppier, ombres des arbres)** · E2 · priorité P4 Basse · ✅ Fusionné avec WFS-53 (PR #61) (règles : `regles-de-gestion/WFS-43.md`) — ↪ fusionné dans WFS-53
- [ ] `trafic-cam0` : plus de cœur visible au bord des houppiers ; ombre d'arbre découpée visible sur une capture au soleil
- [ ] ≤ 300 appels de dessin ; banc `0 écart(s)`

**WFS-65 — Effets de conduite et d'accident (étincelles, fumée, lignes de vitesse, vignette, flash)** · E4 · priorité P3 Moyenne · ✅ Fusionné le 2026-11-04 par confiance (PR #46, QA de nuit : `qa/Check.md`) — Claude (agent 4, session 011cpqZan), 2026-11-04, exception 2e ticket E4 (fichiers séparés de WFS-09) (règles : `regles-de-gestion/WFS-65.md`)
- [ ] voir les critères du fichier de règles

**WFS-66 — Caméras et vue cockpit (3 vues, volant, champ de vision selon la vitesse)** · E3 · priorité P2 Haute · ⏳ Prêt (règles : `regles-de-gestion/WFS-66.md`)
- [ ] voir les critères du fichier de règles

**WFS-67 — Trafic vivant (clignotants, feux, accidents contournés, variété des modèles)** · E3 · priorité P3 Moyenne · ⏳ En attente (règles : `regles-de-gestion/WFS-67.md`)
- [ ] voir les critères du fichier de règles

**WFS-68 — Réglages : qualité, caméra, son, commandes** · E8 · priorité P3 Moyenne · ⏳ En attente (règles : `regles-de-gestion/WFS-68.md`) — ↪ fusionné dans WFS-41
- [ ] voir les critères du fichier de règles

**WFS-69 — [STABLE] CI : banc en jobs parallèles, fin des builds inutiles** · E7 · priorité P1 Urgent · ✅ Fusionné le 2026-11-03 (PR #37, QA de nuit : `qa/Check.md`) — agent 3 (session 011cpqZd), 2026-11-03 (règles : `regles-de-gestion/WFS-69.md`)
- [ ] doc seule = aucun build ; un seul build par commit ; `main` jamais annulé

**WFS-70 — Ressenti de conduite** · E3 · priorité P2 Haute · ✅ Fusionné (par confiance, QA de nuit à faire), PR [#51](https://github.com/wlfaleh/waforspeed_mobile/pull/51) (règles : `regles-de-gestion/WFS-70.md`)
- [x] choc = secousse + ralenti ; 2 modes de direction ; réduire les mouvements

**WFS-71 — Ville vivante** · E2 · priorité P3 Moyenne · ✅ Fusionné (fusion par confiance, QA de nuit à faire) — agent 3 (session 011cpqZd), 2026-11-04, branche `feature/waforspeed-71` (règles : `regles-de-gestion/WFS-71.md`, section « Zone et partage avec WFS-53 »)
- [ ] terrasses, portique, enseignes visibles

**WFS-72 — Point d'extension : apparence des personnages** · E0 · priorité P2 Haute · ✅ Fusionné (PR #40, QA de nuit à faire) — Claude (session trafic), 2026-11-03, branche `feature/waforspeed-72` (règles : `regles-de-gestion/WFS-72.md`)
- [ ] `squelette-tenue` : homme et femme habillés, peau / cheveux / haut / bas / chaussures distincts ; captures existantes identiques, banc `0 écart(s)`

**WFS-84 — [URGENT] Nuit : feux des voitures naturels** · E4 · priorité P1 Urgent · ✅ Fusionné le 2026-11-04 (PR #70, par confiance ; QA de nuit : `qa/Check.md`) — Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04, branche `feature/waforspeed-84` (règles : `regles-de-gestion/WFS-84.md`)
- [ ] petits feux rouges nets + lueur douce, pas de gros halos (coordonné avec WFS-81)

**WFS-85 — [URGENT] Caméra de poursuite plus haute** · E3 · priorité P1 Urgent · ✅ Fusionné le 2026-11-04 (PR #71, CI verte et captures regardées ; QA de nuit : `qa/Check.md`) — Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04, branche `feature/waforspeed-85` (règles : `regles-de-gestion/WFS-85.md`)
- [ ] trafic devant visible depuis la vue Poursuite ; ancienne vue basse conservée en option

**WFS-86 — [URGENT] Voitures en course aussi belles qu'au garage** · E4 · priorité P1 Urgent · ✅ Fusionné le 2026-11-04 par confiance (PR #72, QA de nuit : `qa/Check.md`) — Claude (agent 4, session 011cpqZan), 2026-11-04, branche `feature/waforspeed-86` (règles : `regles-de-gestion/WFS-86.md`)
- [ ] course = garage en netteté et reflets ; cause au Journal

**WFS-90 — Conduite libre : demi-tour, marche arrière, autre sens** · E3 · priorité P2 Haute · ✅ Fusionné le 2026-11-04 par confiance (PR #78, QA de nuit : `qa/Check.md`) — Claude (agent 4, session 011cpqZan), 2026-11-04, branche `feature/waforspeed-90` (règles : `regles-de-gestion/WFS-90.md`). Zone (exception E3, fichiers séparés de WFS-89) : `src/game/player.*`, `src/game/course*`, `src/game/camera_vue.*`, `src/game/direction.*`, `src/game/roues.*`, nouveau `tests/test_conduite_libre.cpp` ; accroches ≤ 10 lignes annoncées : `CMakeLists.txt`, `src/game/traffic_system.cpp`, `src/ui/hud.cpp`, bouton « Demi-tour » ; interdit : `src/game/duel*`, `reseau_*`, `binarypack*`, `tests/test_binarypack*`, `src/ui/ecrans/salon.*` (WFS-89), `src/render/**` (WFS-88)
- [ ] demi-tour possible, caméra qui suit, chrono et checkpoints seulement dans le bon sens

**WFS-104 — [PRIORITÉ ABSOLUE] Voiture en course = qualité du garage** · E4 · priorité P1 Urgent · ✅ Terminé (PR #82, fusion par confiance ; QA de nuit 2026-11-04 : **VALIDÉ**, [rapport](qa/rapports/WFS-104.md)) — Claude (agent 3, session 011cpqZd), 2026-11-04, branche `feature/waforspeed-104` (règles : `regles-de-gestion/WFS-104.md`)
- [x] course = garage en netteté et reflets (Haute et Moyenne, captures vérifiées par la QA) ; confirmation sur téléphone par l'utilisateur

**WFS-105 — [PRIORITAIRE] Caméra de poursuite ≈ 40° + vue Proche** · E3 · priorité P1 Urgent · ✅ Fusionné le 2026-11-04 (PR #85, facefd2, fusion par confiance, QA de nuit : `qa/Check.md`). Valeurs : Poursuite 9 m × 6,3 m, point visé −6,3 m, 66° (35° à l'arrêt, ≈ 28° en course ; 40° écarté, l'horizon sort de l'image) ; Proche 6 m × 2,67 m, point visé −2,1 m, 60° (24°) — Copilot (session 1ef8d4d6), 2026-11-04, branche `feature/waforspeed-105` (règles : `regles-de-gestion/WFS-105.md`). Zone : `src/game/camera_vue.*`, `src/game/camera_banc.*`, assertions caméra de `tests/test_systems.cpp` ; interdit : `src/render/**`, `shaders/**`, `src/game/traffic_system.cpp`, `course_system.*` (WFS-101), `commandes_tactiles.*`, `direction.*`, `player.*` (WFS-108), `src/game/game.*`, `src/ui/**`, `src/platform/**`, `.github/**`, iOS
- [ ] au moins 4 voitures devant visibles ; angle documenté
- [ ] `camera-proche` : avant de la voiture et rue devant (≥ 15 m) visibles, caméra inclinée ≈ 20-25° ; Capot et Cockpit inchangés

**WFS-106 — [STABLE] CI : APK de main en ligne ≈ 5 min après chaque fusion + banc découpé** · E7 · priorité P1 Urgent · ✅ Terminé (PR #88, fusionnée le 2026-11-05 à 01:34:25 après 4 CI de branche vertes, commit de fusion 0a88f80 ; recette QA de nuit à faire, `qa/Check.md` ; le critère « deux fusions de suite » se vérifie sur `main`, voir le Journal) — Copilot (session 6be876ef), 2026-11-04, branche `feature/waforspeed-106` (règles : `regles-de-gestion/WFS-106.md`, RG-01 à RG-11 ; élargi par le PO le 2026-11-04 : délai de publication, quota de l'API (403 « rate limit exceeded for installation » : ≈ 470 appels par publication pour 1 000 par heure), durée du banc ; mêmes fichiers, une seule CI). Zone : `.github/workflows/build.yml`, `tools/qa/publier_apercu.sh` (modes « APK seul » / « captures seules », empreintes), `src/platform/desktop_main.cpp` (aiguillage `WFS_BANC` seulement) ; doc ≤ 10 lignes `.claude/skills/waforspeed-natif/references/construction-et-tests.md`. Interdit : `ios.yml`, `ios/**`, `simulateur.yml`, `android/**`, fonctions `bench*`, tout code de jeu, `shaders/**`. Rien à relancer : l'APK de `df708ac` (WFS-104) est en ligne depuis 23:15:14 (construction 559). Jusqu'à la fusion, une publication de `main` rouge sur 403 se relance une heure après le premier 403 (`gh run rerun <run> --failed`). Fusion après CI de branche verte (pas d'essai local possible).
- [ ] deux fusions de suite sur `main` : `WaForSpeed.apk` de `apercu` remplacé ≤ 8 min après chaque fusion, Release finale = APK et titre du dernier commit ; runs et heures au Journal
- [ ] même liste de captures dans `apercu` avant / après (aucune perdue ni renommée), notes = commit des captures
- [ ] CI de la branche : plus long job de banc ≤ 11 min, durée totale notée au Journal (cible ≈ 12 min contre ≈ 21-25)
- [ ] délais maximaux (`timeout-minutes`, délai par appel `gh`) ; une publication de captures ne retient jamais un APK ; `apercu-waforspeed-106` publiée (APK + captures)
- [ ] quota : plus d'APK en échec à cause du quota (attente de la remise à zéro visible, réserve de 200 appels), captures envoyées seulement si elles ont changé ; nombre d'appels d'une publication de `main` noté au Journal (avant ≈ 470)

**WFS-107 — [URGENT] Bug animation « feu blanc » au braquage** · E4 (exception : prend la place de WFS-101, mêmes fichiers) · priorité P1 Urgent · ✅ Terminé (fusion par confiance, PR #87, `e9cc93e`, 2026-11-05 ; recette QA de nuit à faire, `qa/Check.md`) — Copilot (session dc0cdd3f), 2026-11-04 (réservé par le PO, démarré à la fusion de WFS-101), branche `feature/waforspeed-107` (règles : `regles-de-gestion/WFS-107.md`). **Cause trouvée** (banc, 2026-11-04) : fumée des pneus de WFS-65, ≈ 4,6 × trop claire (presque blanche) par la conversion `hdr()` de `effets_systeme.cpp`, émise en lumière donc lumineuse la nuit. **Corrigé** : couleur du jeu web (`fumeeHdr` = `versHdr(0,32 × fondu)`, ≈ 4,3 × moins claire), matériau `metal = 1` (émissif seul), bouffées rondes (texture `TEXS.radial`), taille du jeu web (≈ 1,33 m), aucune émission si « réduire les mouvements » ; captures `braquage-{gauche,droite}-{30,120,250}-{jour,nuit}` en vue Proche (aucune fumée à 30 km/h). Zone : `src/render/effets_systeme.*`, `src/render/effets_particules.*`, `src/render/bench_effets.*`, `tests/test_effets.cpp` ; accroche ≤ 10 lignes `src/platform/desktop_main.cpp` (captures), annoncée au Journal. Interdit : `renderer.*`, `shaders/**` (WFS-100), `traffic_system.cpp` (WFS-109), iOS
- [ ] captures `braquage-gauche` et `braquage-droite` à 30, 120 et 250 km/h, de jour et de nuit : aucune lueur blanche ; fumée grise, ronde et translucide comme au jeu web, jamais sous 72 km/h au simple braquage
- [ ] « réduire les mouvements » : aucune fumée ni étincelle (jeu web) ; aspect des étincelles et du nitro de WFS-101 inchangé ; tests CPU verts ; l'utilisateur confirme sur téléphone

**WFS-108 — [URGENT] Commandes tactiles plus grandes, frein / marche arrière, gyroscope** · E3 (exception : 2e ticket E3, zones disjointes de WFS-105, demande explicite de l'utilisateur) · priorité P1 Urgent · ✅ Fusionné le 2026-11-04 (PR #83, e6ea49b, fusion par confiance ; QA de nuit : **VALIDÉ**, [rapport](qa/rapports/WFS-108.md)) — Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04, branche `feature/waforspeed-108` (règles : `regles-de-gestion/WFS-108.md`). Zone : `src/game/commandes_tactiles.*`, `src/game/direction.*`, `src/game/player.*` (frein / marche arrière), `src/ui/ecrans/reglages.*` (mode de direction, taille des boutons, sensibilité / zone morte du gyroscope), `src/game/sauvegarde.*` (champs ajoutés, rétrocompatibles), nouveau fichier `src/platform/gyroscope_android.cpp` (+ `gyroscope.h`, banc PC : valeurs d'essai), tests CPU (`tests/test_systems.cpp` hors assertions caméra, ou nouveau fichier de test). Accroches ≤ 10 lignes chacune, annoncées au Journal : `src/ui/hud.cpp` (indication « R »), `src/platform/android_main.cpp` (démarrer / lire le capteur), `CMakeLists.txt` (nouveau fichier source), `src/game/game.cpp` (réétalonnage au départ de course). Interdit : `src/game/camera_*` (WFS-105), `src/game/traffic_system.cpp` et `course_system.*` (WFS-101), `src/render/**`, `shaders/**`, autres écrans de `src/ui/**`, `.github/**`, iOS (gel). Décision RG-02 : marche arrière par **maintien du frein** (comme le jeu web, `js/game/update.js`), pas de bouton « R » séparé.
- [ ] boutons GAZ / FREIN / NITRO / flèches ≥ 48 dp (GAZ / FREIN 64 à 80 dp), sans chevauchement ni encoche (captures du HUD tactile)
- [ ] test CPU : frein maintenu à l'arrêt → marche arrière, « R » au HUD ; gaz → avant
- [ ] mode gyroscope (seul ou combiné) : valeurs d'essai → braquage ; réglages mémorisés ; l'utilisateur confirme sur téléphone

**WFS-109 — [URGENT] Feux de freinage réalistes + halos orange des clignotants (ANO-48)** · E4 (exception : 2e ticket E4 actif, blocs séparés ; + E3 `traffic_system.cpp`) · priorité P1 Urgent · ✅ Fusionné (QA de nuit à faire), PR #117, commit 4469c5e, 2026-11-05, fusion par confiance — Claude (session trafic, agent reprise), 2026-11-05 (repris de Copilot sur demande de l'utilisateur), ex-Copilot (session dc0cdd3f) ; branche `feature/waforspeed-109` reprise (aucun commit de travail, seulement 6e09c9f d'attribution ; reste à faire : section « Reprise du 2026-11-05 » des règles) (règles : `regles-de-gestion/WFS-109.md`, RG-10 à RG-14 ajoutées le 2026-11-05). **Constat** : le jour, aucun feu du modèle ne s'allume au freinage (joueur : rien ; trafic : carrés de halo de 1,1 m, lumière × 7) ; halos orange des clignotants / feux de détresse (1,5 m, lumière × 7) : très grande tache orange sur `accident-fumee` (ANO-48, signalée par Copilot dc0cdd3f). Zone : bloc des feux de `src/render/renderer.cpp` (`draw`, lignes `feuxVoiture` / `MAT_TAIL` ; `namespace feux` et `feuxVoiture` de WFS-84), `src/game/traffic_system.cpp` (`feux`, halos, ligne `feuxProchaineVoiture`), `tests/test_trafic_vivant.cpp`, nouveaux `src/render/feux_modele.cpp/.h` (découpe des feux arrière, RG-13) ; accroches ≤ 10 lignes annoncées : `src/render/renderer.h` (`FeuxVoiture`, champ `ModelPart::feuArriere`), `src/render/model_loader.cpp` (appel de la découpe avant l'envoi des tampons et boîte du feu dans le bloc WFS-09 ; pas les matériaux de WFS-100), `CMakeLists.txt` (1 ligne de test, facultatif), `src/render/nuit.cpp` (bloc des feux du joueur seulement, aussi le jour ; le reste du fichier, réglé par WFS-97 ✅, hors périmètre : RG-11), `src/platform/desktop_main.cpp` (captures) ; `#include "effets_particules.h"` permis (`effets::versHdr`, sans le modifier). Interdit : `shaders/**`, peinture et bloom de `renderer.cpp` (WFS-100), modifier `effets_*`, `player.*` (WFS-108), iOS
- [ ] captures `jour-frein` (joueur), `trafic-freinage` (trafic, de jour) et `nuit-frein` : le feu du modèle lui-même s'allume en rouge vif, lueur courte, plus de grand disque plaqué
- [ ] joueur et trafic identiques ; valeurs notées au Journal ; l'utilisateur valide sur téléphone
- [ ] `accident-fumee`, `trafic-clignotant` : clignotants et feux de détresse bien visibles, halo orange ≤ 1,5 m centré sur le feu, plus de grande tache orange (RG-10, ANO-48)
- [ ] (RG-13, RG-14) Golf et 911, joueur et trafic : le feu arrière du modèle s'allume en rouge au freinage, de jour comme de nuit. La nuit, les feux arrière sont rouges (plus blancs). ≤ 300 appels de dessin

**WFS-113 — [URGENT] Showroom : flèches qui sautent des voitures** · E5 · priorité P1 Urgent · ✅ Fusionné (fusion par confiance, QA de nuit à faire) — Claude (agent 3, session 011cpqZd), 2026-11-05, branche `feature/waforspeed-113` (règles : `regles-de-gestion/WFS-113.md`)
- [ ] une flèche = une voiture, sans saut ni erreur

**WFS-110 — Modes de course : 5 voitures, Checkpoint 60 s, duel sans trafic** · E3 · priorité P2 Haute · ✅ Fusionné par confiance (PR #91, 2026-11-05) — Claude (agent 3, session 011cpqZd), branche `feature/waforspeed-110` (règles : `regles-de-gestion/WFS-110.md`). Zone : `src/game/**` sauf `game.*` (nouveaux fichiers de la course à 5 et des ordinateurs, `course_system.*`, duel), `tests/**` ; dans `src/game/traffic_system.cpp`, seulement l'arrêt du trafic par mode (jamais le bloc des feux de WFS-109) ; accroches ≤ 10 lignes annoncées au Journal : `src/ui/ecrans/**` (choix du mode, WFS-102 en cours : conflits résolus en gardant les deux), `src/game/game.*`, `CMakeLists.txt` (tests), `src/platform/desktop_main.cpp` (captures). Interdit : `src/render/**`, `shaders/**` (WFS-100, WFS-109), `src/world/**` (WFS-94), `src/ui/**` hors accroche, iOS
- [ ] grille de 5 voitures sans trafic ; Checkpoint à 60 s

**WFS-111 — Mode Poursuite police (+ étape 0 : retrait du contournement RG-14 de WFS-91)** · E3 (exception : en parallèle de WFS-115, fichiers séparés) · priorité P2 Haute · ✅ Fusionné (QA de nuit à faire), PR #98 (fusion eab18f9) et PR #100 (fusion cab0027 : carte Poursuite disponible, parcours complet au doigt, 0 écart), par confiance — Claude (session trafic, 2e agent), 2026-11-05, branche `feature/waforspeed-111` (règles : `regles-de-gestion/WFS-111.md`, section « Attribution du 2026-11-05 », RG-10 à RG-12). **Zone** : nouveaux `src/game/poursuite*.cpp/.h` ; `src/game/course_system.*` et `traffic_system.cpp` par accroches ≤ 10 lignes (hors bloc des feux, WFS-109) ; `tests/**` (nouveau test) ; `src/game/modes_ecran.cpp` **une seule ligne** (`dispo` de la carte Poursuite), après fusion de WFS-115 ; étape 0 : `src/platform/android_main.cpp` (`termGraphics`, l. 82-84), `src/platform/endurance_banc.cpp` (l. ≈ 90), `tools/qa/validation_connues.txt`. **Interdit** : le reste de `modes_ecran.cpp` et la navigation de `garage.cpp` (WFS-115), `src/ui/**` (WFS-102), `src/render/**` et `shaders/**` (WFS-109), `src/world/**` (WFS-94), `.github/**`, iOS (gel)
- [ ] police, trafic dense, embouteillages, surprises, score 100

**WFS-112 — Duel entre amis par Internet** · E3 · priorité P2 Haute · ✅ Fusionné (QA de nuit à faire), PR #101 (fusion par confiance, 2026-11-05) — Claude (session trafic, 3e agent), branche `feature/waforspeed-112` (règles : `regles-de-gestion/WFS-112.md`, section « Attribution du 2026-11-05 », RG-09 à RG-12). **Zone** : `src/game/duel*`, `salon.*`, `reseau_*`, `src/platform/reseau_android.cpp`, nouveau `serveur/`, `tests/**`, `AndroidManifest.xml` (lien profond) et `JeuActivity.java` (partage), `src/ui/ecrans/salon.*` (conflits avec WFS-102 : garder les deux), `modes_ecran.cpp` ≤ 10 lignes ; accroches ≤ 10 lignes : `CMakeLists.txt`, `game.*`, `desktop_main.cpp`. **Interdit** : `build.gradle`, keystore, `.github/**` (WFS-118), `src/render/**`, `shaders/**`, bloc des feux de `traffic_system.cpp` (WFS-109), reste de `src/ui/**` (WFS-102), `src/world/**` (WFS-94), iOS (gel)
- [ ] code + lien d'invitation, serveur de relais, reconnexion

**WFS-114 — CI : banc d'endurance ≤ 15 min, seulement sur `main`, la nuit et à la demande** · E7 · P3 Moyenne · ✅ Fusionné par confiance (PR #93, 2026-11-05) — Claude (agent 3, session 011cpqZd), branche `feature/waforspeed-114` (règles : `regles-de-gestion/WFS-114.md`). Ticket facile demandé par l'utilisateur : job `endurance` (23 min aujourd'hui) sauté sur les branches, lancé sur `main`, en `workflow_dispatch` et la nuit ; ≤ 15 min (cache de compilation, moins d'images rendues sans baisser les seuils de WFS-91). Zone : `.github/workflows/build.yml` (job `endurance`, `schedule`, une ligne `if:` par autre job), `tools/qa/endurance.sh`, `tools/qa/validation_connues.txt`, `src/platform/endurance_banc.*`. Interdit : `src/render/**`, `shaders/**`, `src/game/**`, `src/ui/**`, `src/world/**`, `android_main.cpp`, iOS. `VUID-vkCmdDraw-None-08114` : rangé dans WFS-100 (RG-W2).

**WFS-115 — [URGENT] Menus : « Continuer » ramène au menu** · E8 (exception : en parallèle de WFS-102, fichiers séparés) · priorité P1 Urgent · ✅ Fusionné par confiance (PR #97, commit a739a7d, 2026-11-05 ; QA de nuit à faire) — Claude (session trafic), 2026-11-05, branche `feature/waforspeed-115` (règles : `regles-de-gestion/WFS-115.md`). **Zone** : `src/game/modes_ecran.cpp` ; `src/ui/garage.cpp` **seulement** la navigation (`allerA`, drapeau « vient du parcours de course », boutons « Choisir » / « Retour ») ; `tests/**` (test CPU de parcours) ; captures `parcours-*` par accroche ≤ 10 lignes dans `desktop_main.cpp` (hors aiguillage `WFS_BANC`) ; accroche ≤ 10 lignes annoncée au Journal dans `src/game/game.h` si le drapeau de parcours doit y vivre. **Interdit** : `src/ui/ecrans/**` et les lignes d'échelle / libellés de `garage.cpp` (WFS-102), `src/render/**` et `shaders/**` (WFS-100, WFS-109), `src/world/**` (WFS-94), `.github/**`, iOS (gel)
- [ ] menu → mode → terrain → course sans retour imprévu

**WFS-116 — Bouton / geste « Retour » du téléphone** · E7 · priorité P2 Haute · ✅ Fusionné par confiance (PR #95, 2026-11-05) — Claude (agent 3, session 011cpqZd), branche `feature/waforspeed-116` (règles : `regles-de-gestion/WFS-116.md`). Constat du PO : `AKEYCODE_BACK` hors manette non géré → `NativeActivity` se ferme (course perdue). Zone : nouveau `src/platform/retour_systeme.*`, `android_main.cpp` (`onInput`, ≤ 10 lignes), `manette_android.cpp`, `JeuActivity.java` (`onBackPressed` → `moveTaskToBack`), nouveau `tests/test_retour_systeme.cpp` (+ 1 ligne `CMakeLists.txt`) ; accroche ≤ 10 lignes dans `manette_jeu.cpp` / `manette.h` (réutiliser `toucher` et la zone `retour`, sans les lignes d'échelle de WFS-102). Interdit : `src/ui/**`, `src/game/**` (WFS-115), `src/render/**`, `shaders/**`, `src/world/**`, `.github/**`, iOS

**WFS-117 — CI : tous les tests CPU lancés automatiquement** · E7 · priorité P2 Haute · ✅ Fusionné par confiance (PR #96, 2026-11-05) — Claude (agent 3, session 011cpqZd), branche `feature/waforspeed-117` (règles : `regles-de-gestion/WFS-117.md`). Constat du PO : `test_retour_systeme` (WFS-116) et `test_commandes` (WFS-108) sont construits mais jamais lancés par `build.yml`. **Zone** : nouveau `tools/qa/tests_cpu.sh`, `.github/workflows/build.yml` (étape « Test de la piste » seulement), `tests/**` seulement pour réparer un test oublié (≤ 10 lignes, RG-05). **Interdit** : `CMakeLists.txt`, le reste de `build.yml` (groupes du banc, APK, endurance), `src/**`, `shaders/**`, iOS (gel)
- [ ] test CPU de parcours avec « Retour » (RG-06) au journal ; en course Retour → pause ; au menu → arrière-plan sans fermeture

**WFS-118 — [URGENT] APK : même signature à chaque build** · E7 · priorité P1 Urgent · ✅ Fusionné le 2026-11-05 par confiance (PR #99, QA de nuit : `qa/Check.md`) — Claude (session trafic), branche `feature/waforspeed-118` (règles : `regles-de-gestion/WFS-118.md`). **Zone** : `android/app/build.gradle` (`signingConfigs`), nouveaux `android/waforspeed-test.keystore` et `android/keystore.properties`, `.github/workflows/build.yml` (étapes de signature / vérification `apksigner` du job APK et notes de la Release `apercu` seulement). **Interdit** : `src/**` (dont `android_main.cpp` et `endurance_banc.*`, accroches de WFS-111), `shaders/**`, le reste de `build.yml` (groupes du banc, tests, endurance), partie AAB / secret `WFS_CLE_ANDROID` (RG-03), iOS (gel)
- [ ] même empreinte de certificat sur deux builds ; installation par-dessus sans désinstaller

**WFS-119 — Poursuite police, 2e passage** · E3 (exception : 2e ticket E3 actif, fichiers séparés de WFS-112) · priorité P2 Haute · ✅ Fusionné le 2026-11-05 par confiance (PR #106, QA de nuit : `qa/Check.md`) — Claude (session trafic, agent WFS-119), branche `feature/waforspeed-119` (règles : `regles-de-gestion/WFS-119.md`). **Zone** : `src/game/poursuite*`, `tests/test_poursuite.cpp` (et son appel), `src/game/sauvegarde.*` si nécessaire pour RG-03 ; accroches ≤ 10 lignes annoncées au Journal : `src/game/traffic_system.cpp` (densité / silhouette en poursuite, **hors bloc des feux** de WFS-109), `src/ui/hud.cpp` (≤ 3 lignes, RG-02 ; zone WFS-102 : garder les deux), `src/game/modes_ecran.cpp` (≤ 4 lignes, RG-03 ; WFS-112 y a aussi ses lignes : garder les deux), `src/game/course5_system.cpp` (≤ 3 lignes, RG-08 / ANO-53 : `onTouch`, consommer le relâcher avant le test de fin), `src/platform/desktop_main.cpp` (captures dans le groupe `course5`), `CMakeLists.txt`. **Interdit** : `src/game/duel*`, `salon.*`, `reseau_*`, `serveur/`, `AndroidManifest.xml`, `JeuActivity.java` (WFS-112) ; `src/render/**`, `shaders/**` (WFS-109) ; le reste de `src/ui/**` (WFS-102) ; `src/world/**` (WFS-94) ; `.github/**`, `android/**` ; iOS (gel)
- [ ] captures `poursuite-camion`, `poursuite-ralentisseur` ; un seul score à l'écran en poursuite ; test CPU (6 surprises, ralentisseur, mode gardé) ; Journal : écart d'appels de dessin ≤ 60 en Basse et parcours complet RG-06 à 0 écart

**WFS-120 — Sons de conduite : pétarades, turbo, rupteur** · E6 · P3 Moyenne · ✅ Fusionné par confiance (PR #102, 2026-11-05) — Claude (agent 3, session 011cpqZd), branche `feature/waforspeed-120` (règles : `regles-de-gestion/WFS-120.md`). **Zone** : `src/audio/**`, `tests/test_audio.cpp` ; accroche ≤ 3 lignes annoncée : `CMakeLists.txt` (ligne `test_audio`). **Interdit** : `src/game/**` (WFS-119), `src/ui/**` (WFS-102), `src/render/**`, `shaders/**` (WFS-109), `src/world/**` (WFS-94), `.github/**`, `android/**`, iOS (gel)
**WFS-121 — Sons du trafic : passages (Doppler), klaxon, rumeur** · E6 · P3 Moyenne · ✅ Fusionné le 2026-11-05 par confiance (PR #103, QA de nuit : `qa/Check.md`) — Claude (session trafic), branche `feature/waforspeed-121` (règles : `regles-de-gestion/WFS-121.md`, RG-01 à RG-08). **Zone** : `src/audio/**` (nouveau `trafic_sons.*` conseillé), `tests/test_audio.cpp` ; accroche ≤ 3 lignes annoncée : `CMakeLists.txt` ; `#include` en lecture seule de `src/game/traffic_system.h`, `traffic.h`, `player.h`. **Interdit** : écrire dans `src/game/**` (WFS-119, WFS-109), `src/ui/**` (WFS-102), `src/render/**`, `shaders/**` (WFS-109), `src/world/**` (WFS-94), `.github/**`, `android/**`, iOS (gel)
**WFS-122 — Duel entre amis, suite : derniers adversaires, salon à 4** · E3 (+ `salon.*`) · P3 Moyenne · ⏳ En attente de la fin de WFS-119 (E3) et de WFS-102 (`salon.*`) (règles : `regles-de-gestion/WFS-122.md`). Suites de WFS-112 ; le libellé faux du menu (« même trafic ») est rangé dans WFS-102 (RG-15b) ; la mise en ligne du relais reste une action de l'utilisateur (`tache utilisateur.md`).
- [ ] APK : pétarades au lever de pied à haut régime, pschitt à chaque rapport, rupteur en 5e à fond ; rien en Model 3 ni dans les menus
- [ ] banc : `SFX turbo` (≥ 3), `SFX petarade` (≥ 1) au journal ; `test_audio` étendu ; parcours complet accueil → course → fin → Menu au Journal (RG-08)

**WFS-123 — Tenue sur téléphone : chauffe, batterie, 60 images/s stables** · E7 · P2 Haute · ✅ Fusionné le 2026-11-05 par confiance (PR #105, QA de nuit : `qa/Check.md`) — Claude (session trafic), branche `feature/waforspeed-123` (règles : `regles-de-gestion/WFS-123.md`, RG-01 à RG-10). **Zone** : `src/platform/**` (nouveaux `chauffe.h`, `chauffe_android.cpp`, simulation bureau `WFS_CHAUFFE`), `JeuActivity.java`, nouveau `tests/test_chauffe.cpp` ; accroches ≤ 10 lignes annoncées : `src/render/qualite_systeme.cpp`, `src/render/qualite_auto.h`, `CMakeLists.txt` (≤ 3). **Interdit** : `src/game/**` (WFS-119), `src/ui/**` dont `toasts.*` / `hud.cpp` (WFS-102 : appeler l'API seulement), `src/world/**` (WFS-94), le reste de `src/render/**` et `shaders/**` (WFS-109), `src/audio/**`, `.github/**`, iOS (gel). Critères : chauffe simulée 2 → une rétrogradation + bandeau « Graphismes rapides », rétablie au retour à 0 ; chauffe 1 → résolution bloquée ; 30 images/s aux menus, 60 en course ; `test_chauffe` vert ; parcours complet RG-10 au journal ; logcat du téléphone `CHAUFFE` / `setFrameRate 60`, aucun plantage sous API 30.

**WFS-124 — Sons de course, 2e passage : tic du chrono, combo, combo perdu** · E6 · P3 Moyenne · ✅ Fusionné par confiance (PR #104, 2026-11-05) — Claude (agent 3, session 011cpqZd), 2026-11-05, branche `feature/waforspeed-124` (règles : `regles-de-gestion/WFS-124.md`, RG-01 à RG-08). **Zone** : `src/audio/**` (`sfx.*`, nouveau `sfx_course.h`), `tests/test_audio.cpp` ; accroche ≤ 3 lignes annoncée : `CMakeLists.txt` ; `#include` en lecture seule de `src/game/course.h`. **Interdit** : écrire dans `src/game/**` (WFS-119), `src/ui/**` (WFS-102), `src/render/**`, `shaders/**` (WFS-109), `src/world/**` (WFS-94), `src/platform/**`, `.github/**`, `android/**` (WFS-123), iOS (gel). Critères : tic sous 8 s en mode Checkpoint (urgent sur 3, 2, 1), rien en duel / course à 5 / poursuite ni après un checkpoint qui remonte le chrono ; bip de combo qui monte (combo 2 à 6) ; « combo perdu » sur choc seulement ; `test_audio` étendu ; journal du banc `SFX chrono-tic` / `chrono-urgent` et `SFX combo-2` ; parcours complet RG-08 au Journal.
**WFS-127 — Showroom sonore : moteur de la voiture affichée au garage** · E6 · P3 Moyenne · ✅ Fusionné par confiance (PR #107, 2026-11-05) — Claude (agent 3, session 011cpqZd), 2026-11-05, branche `feature/waforspeed-127` (règles : `regles-de-gestion/WFS-127.md`, RG-01 à RG-10). **Zone** : `src/audio/**` (`audio_system.cpp`, `sfx.*`, nouveau `garage_son.h`), `tests/test_audio.cpp` ; accroches ≤ 6 lignes annoncées au Journal : `src/ui/garage.h/.cpp` (accesseur `garageVoitureAffichee()` seulement), `CMakeLists.txt`, `desktop_main.cpp` (parcours). **Interdit** : navigation / échelle / dessin de `garage.cpp`, `src/ui/ecrans/**` (WFS-102), `src/game/**` (WFS-125), `src/render/**`, `shaders/**` (WFS-109), `src/world/**` (WFS-94), le reste de `src/platform/**`, `.github/**`, `android/**` (WFS-126), iOS (gel). Critères : au garage, démarreur + coup de gaz + ralenti de la voiture affichée à chaque flèche, sans superposition ; course avec le moteur choisi, rien de coincé ; `test_audio` étendu ; journal `MOTEUR garage <id>` × 3 et `SFX demarreur` ; parcours complet RG-10 au Journal.

**WFS-128 — Configuration du jeu par fichier `configgame.json`** · E0 · priorité P2 Haute · ✅ Fusionné par confiance (PR #110, 2026-11-05) — Claude, agent 3 (session 011cpqZd), 2026-11-05, branche `feature/waforspeed-128` (règles : `regles-de-gestion/WFS-128.md`, section « Attribution du 2026-11-05 » : zone, accroches RG-13 à RG-16, étapes)
- [ ] changer une valeur dans le fichier change le jeu sans toucher au code ; fichier absent = comportement actuel

**WFS-129 — Android prêt pour Google Play 2026 (SDK 35, 16 Ko, bord à bord) + endurance fiable** · E7 · priorité P2 Haute · ✅ Fusionné par confiance (PR #111, 2026-11-05, QA de nuit à faire) — Claude (session trafic), 2026-11-05, branche `feature/waforspeed-129` (règles : `regles-de-gestion/WFS-129.md`, section « Attribution du 2026-11-05 »). Zone : `android/**` (hors bloc des fichiers copiés de WFS-128), jobs `android` et `endurance` de `build.yml`, `src/platform/endurance_banc.*`, `tools/qa/**`, `store/checklist.md` ; accroche ≤ 10 lignes `desktop_main.cpp`. Interdit : `src/core/**`, `src/game/**`, `CMakeLists.txt`, `android_main.cpp`, `configgame.json` (WFS-128), `src/ui/**` (WFS-102), `src/world/**` (WFS-94), `src/render/**`, `shaders/**` (WFS-109), `src/audio/**`, signature de WFS-118, iOS (gel)
- [ ] `compileSdk` / `targetSdk` 35, APK et AAB signés (empreinte WFS-118 inchangée, installation par-dessus)
- [ ] étape CI « pages de 16 Ko » verte (segments LOAD ≥ 2**14, `zipalign -c -P 16`) ; plein écran, rien sous l'encoche
- [ ] endurance : seuil mesuré au début de cycle, cycles Course à 5 et Poursuite, 3 lancements verts de suite, ≤ 15 min
- [ ] parcours complet RG-08 au Journal ; Retour du téléphone comme WFS-116 ; ligne `qa/Check.md`

**WFS-130 — Correction : mémoire de la Course à 5** · E3 · priorité P1 Urgent · ✅ Fusionné (QA de nuit à faire), PR #113 — Claude (session trafic), 2026-11-05, branche `feature/waforspeed-130` (règles : `regles-de-gestion/WFS-130.md`). Zone : `src/game/course5_system.*`, `tests/**` ; accroche E7 : `tools/qa/validation_connues.txt` (retrait de la ligne). Interdit : `src/game/traffic_system.cpp` (WFS-109), `course_system.*`, `duel_system.*`, `commandes_tactiles.*`, `camera_vue.*` (WFS-102), `src/ui/**` (WFS-102), `src/world/**` (WFS-94), `src/render/**`, `shaders/**` (WFS-109), `game.*`, `src/core/**`, `.github/**`, iOS (gel)
- [ ] 10 Courses à 5 de suite au banc PC : RSS et images Vulkan stables (≤ +10 %), chiffres au Journal
- [ ] job `endurance` vert sans `memoire-course5-connue`, 0 erreur de validation
- [ ] parcours complet RG-06 au Journal (4 ordinateurs visibles à chaque départ) ; tests CPU OK ; ligne `qa/Check.md`

**WFS-131 — Sirène de police réaliste (wail / yelp, volume selon la distance, Doppler, jingles de fin)** · E6 · P3 Moyenne · ✅ Fusionné (fusion par confiance, QA de nuit à faire), PR #133, commit b2362b9 — Claude (session 01Us6uCu), 2026-11-06 09:55 UTC (réattribué, règle 9 : aucune branche poussée par Claude, agent 3, session 011cpqZd, attribué le 2026-11-05 17:59), branche `feature/waforspeed-131` (règles : `regles-de-gestion/WFS-131.md`). Zone : `src/audio/**` (nouveau `sirene_son.h`), `tests/test_audio.cpp` ; accroches ≤ 10 lignes annoncées au Journal : `src/game/poursuite_system.cpp` (ligne de la sirène l. 184-186 et ligne du jingle à `fini_`, RG-07), `CMakeLists.txt` ≤ 3 lignes. Interdit : le reste de `src/game/**` (WFS-130, WFS-109, WFS-102), `poursuite.cpp/.h`, `src/ui/**`, `src/render/**`, `shaders/**`, `src/world/**`, `src/platform/**`, `.github/**`, iOS (gel)
- [x] Sirène continue qui enfle / s'éteint selon la distance (15 m → 0,45 ; 150 m → 0), yelp sous 20 m, Doppler borné ± 12 %
- [x] Jingle ARRÊTÉ ou ÉCHAPPÉ à la fin, plus de sirène ensuite ni dans les autres modes
- [x] `test_audio` étendu (RG-10), `sirene.wav` au banc, parcours complet RG-11 au Journal

**WFS-132 — Nouvelle version de l'APK annoncée au lancement + boutons de volume** · E7 · P2 Haute · ✅ Fusionné (QA de nuit à faire), PR #114, commit 894208c — Claude (session trafic), 2026-11-05, branche `feature/waforspeed-132` (règles : `regles-de-gestion/WFS-132.md`). Zone : `android/**` (`JeuActivity.java`, nouveau `MiseAJour.java`, `android/app/src/test/**`, `build.gradle` : dépendance JUnit), `.github/workflows/build.yml` (étape `testReleaseUnitTest` du job `android`, ≤ 3 lignes). Interdit : `src/**`, `shaders/**`, `assets/**`, `tools/qa/publier_apercu.sh` (format du titre à garder), `ios/**` et `ios.yml` (gel)
- [ ] `MiseAJourTest` (8 cas RG-08) vert au job `android` ; banc `parcours` 0 écart ; déclencheur `wfs_maj_test` → boîte « Nouvelle version disponible » (Télécharger / Plus tard) ; sans réseau : une ligne `MISEAJOUR`, aucune boîte ; boutons de volume = son du jeu

**WFS-133 — Course à 5, 2e passage : difficulté au choix et ordinateurs combatifs** · E3 · P2 Haute · ✅ Fusionné (QA de nuit à faire), PR #115, commit 2c9580b — Claude (session trafic), 2026-11-05, branche `feature/waforspeed-133` (règles : `regles-de-gestion/WFS-133.md`). Zone : `src/game/course5.*`, `src/game/course5_system.cpp`, `src/game/modes_ecran.cpp`, `src/game/sauvegarde.*` (clé `diff5`), `tests/test_course5.cpp` (et test de sauvegarde) ; accroches ≤ 10 lignes annoncées : `src/core/configgame.*`, `configgame.json`, `configgame.README.md`. Interdit : `src/game/traffic_system.cpp` (WFS-109), `src/game/poursuite_system.cpp` et `src/audio/**` (WFS-131), `course_system.*`, `duel_system.*`, `camera_vue.*`, `commandes_tactiles.*`, `src/ui/**` (WFS-102), `player.*`, `src/render/**`, `shaders/**`, `src/world/**` (WFS-94), `src/platform/**`, `android/**`, `.github/**`, iOS (gel)
- [ ] `modes-choix` : 3 puces FACILE / NORMALE / DIFFICILE sous les distances, NORMALE par défaut, choix mémorisé (`diff5`)
- [ ] test_course5 : Difficile plus rapide que Facile (même graine), rattrapage, aspiration, nitro bornée, 0 choc entre ordinateurs, tous arrivent
- [ ] parcours complet RG-10 au Journal (Rejouer garde la difficulté), 0 écart ; ligne `qa/Check.md`

**WFS-134 — Voitures en course, 3e passage : qualité prouvée sur le téléphone** · E4 (exception, à côté de WFS-109) · P1 Urgent · ✅ Fusionné (QA de nuit à faire) — PR #119, commit 1bf0514, 2026-11-05 (**ajout du 2026-11-05 21:01** : RG-11 durcie, plus aucun cube à aucune distance ; RG-12 transition progressive complet → `-lod` (hystérésis ±10 %, fondu 8 m) ; RG-13 captures `voiture-distance-{10,25,50,60,80,120,180}` et `voiture-lod-passage`, avant / après ; RG-14 pas de fusion par confiance, verdict `qa-lead` avant fusion ; ANO-66 (RG-08) transférée à WFS-142) — Claude (session trafic), 2026-11-05, branche `feature/waforspeed-134` (règles : `regles-de-gestion/WFS-134.md`). Zone : `src/render/qualite_*`, `vk.*`, `model_loader.cpp` (hors accroche de découpe des feux de WFS-109), `renderer.cpp` (hors bloc des feux), `shaders/scene*`, `src/render/nuit.cpp` (reflets, ANO-66) ; accroches ≤ 10 lignes : `src/ui/ecrans/reglages.cpp` (ligne « Rendu »), `course_system.cpp` (dessin du fantôme), `desktop_main.cpp`. **Ajout du 2026-11-05 19:36 (RG-11, voitures en cubes, priorité absolue)** : `src/game/traffic_system.cpp` **seulement** le choix silhouette / modèle (l. 129 et 143, `dessineLoin`) et `src/game/poursuite_system.cpp` (choix `complet` de la police l. 221-222 et du camion l. 253). Interdit : bloc des feux, `feux_modele.*`, le reste de `traffic_system.cpp` (WFS-109), `src/game/modes_ecran.cpp`, `course5*`, `sauvegarde.*` (WFS-133), `src/world/**`, iOS (gel)
- [ ] `voiture-course-proche`, `voiture-course-trafic`, `voiture-course-basse` aussi nettes que `garage-golf` ; ligne « Rendu : … » dans `reglages-menu`
- [ ] `vitrine-x5-0`, `vitrine-model3-0` sans beige (ANO-67) ; `records-fantome` translucide (ANO-60) ; reflet de l'enseigne flou (ANO-66) ; parcours complet 0 écart

**WFS-135 — Lot rapide anomalies : textes, caméra « Arrière », score, captures du parcours, test du relais** · E7 (+ accroches) · P2 Haute · ✅ Fusionné (PR #116, fusion par confiance, QA de nuit à faire) — Claude (session dev expert, 0183bv2X), 2026-11-05 20:01 UTC, branche `feature/waforspeed-135` (règles : `regles-de-gestion/WFS-135.md`). Zone : `.github/workflows/build.yml` ; accroches ≤ 10 lignes : `menu.cpp`, `terrain.cpp`, `camera_vue.h`, `reglages.cpp`, `hud.cpp`, `tests/test_relais.cpp`. Interdit : `modes_ecran.cpp`, `course5*`, `sauvegarde.*` (WFS-133), `src/render/**`, `src/world/**`, `poursuite_system.cpp`, iOS (gel)
- [ ] 5 captures `parcours-*` dans la Release `apercu` ; textes du menu et du terrain justes ; « ARRIÈRE » ; score lisible ; test_relais 10/10 ; parcours 0 écart

**WFS-136 — Tracé des cartes, 2e version (catalyseur des 4 cartes)** · E3 · P2 Haute · ✅ Fusionné (PR #128, fusion par confiance, Grand Prix seul au tracé 2 : A7) — Claude (session dev expert, 0183bv2X), 2026-11-05 20:37 UTC, branche `feature/waforspeed-136` (WFS-133 ✅ ; règles : `regles-de-gestion/WFS-136.md`). Zone : `src/game/track.*`, `trace_cartes.*` (nouveaux), `traffic.cpp`, `sauvegarde.*`, `reseau_relais.*`, `tests/**` ; accroches : `traffic_system.cpp` (hors bloc des feux), `CMakeLists.txt`, `desktop_main.cpp`. Interdit : `src/world/**`, `src/render/**`, `src/ui/**`, iOS (gel)
- [ ] `trace-rond-point`, `trace-feux-rouge`, `trace-epingle`, `trace-autoroute` ; test_trace vert ; anciens records gardés (`*_v1`) ; parcours 0 écart

**WFS-137 à WFS-140 — Décor des 4 cartes (Ville, Campagne, Corniche, Grand Prix)** · E2 · P2 Haute · WFS-140 🔧 En cours — Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-06, branche feature/waforspeed-140 (réattribué le 2026-11-06 12:00 UTC ; avant : Claude, session dev expert, 0183bv2X, 08:21 UTC, sans travail poussé ; choisi en premier car le Grand Prix est la seule carte déjà au tracé 2 en jeu) ; WFS-137 🔧 En cours (Claude, agent 4, session 011cpqZan, 2026-11-06 12:00 UTC, branche `feature/waforspeed-137`, exception PO : 2e ticket E2 actif à côté de WFS-140, fichiers de la Ville seulement) ; WFS-138, 139 ⏳ Prêt, débloqués le 2026-11-06 (une carte à la fois : E2 = un ticket actif ; règles : `regles-de-gestion/WFS-137.md` à `WFS-140.md`). Zone : le fichier de la carte (`ville.cpp` + `ville_quartiers.*`, `campagne.cpp`, `plage.cpp`, `circuit.cpp`), `assets/world/**`, `cartes_banc.cpp`. Interdit : `src/game/**`, les 3 autres cartes, `src/render/**`, iOS (gel)
- [ ] captures `carte-<carte>-*` de jour et de nuit : chaque lieu des règles visible ; rien sur la chaussée ; parcours 0 écart

**WFS-141 — Lot anomalies Poursuite et écrans (ANO-54, 56, 57, 59, 62)** · E3 (+ accroches E8) · P2 Haute · ✅ Fusionné (QA de nuit à faire), PR #130 (926604d) ; **décision PO 2026-11-06 : RG-02 (bandeau à 5 % de la hauteur) et RG-03 (tailles de panneau, boutons et textes des fins) ⏸ en attente du test QA avec émulateurs** (ordre de l'utilisateur 63dd043, tailles d'interface gelées), ni faites ni abandonnées : un ticket les reprendra après ce test (ANO-57, ANO-59) ; dépassements d'accroche (`salon.cpp` 31 lignes, `poursuite_system.cpp` hors l. 228-236) acceptés, annoncés au Journal — Claude (session trafic), 2026-11-06 06:48 UTC, branche `feature/waforspeed-141` (exception PO : 2e ticket E3 à côté de WFS-136, fichiers disjoints ; WFS-131 non fusionné : dans `poursuite_system.cpp`, seulement l. 228-236 ; tests dans `tests/test_poursuite.cpp` / `tests/test_course5.cpp` seulement ; interdit en plus : `trace_cartes.*`, `traffic*.cpp`, `sauvegarde.*`, `reseau_relais.*` (WFS-136) ; règles : `regles-de-gestion/WFS-141.md`). Zone : `poursuite_system.cpp`, `poursuite.cpp`, `course5_system.cpp`, `modes_ecran.cpp`, `course_system.*` (fin), `tests/**` ; accroches : `salon.cpp`, `menu.cpp`, `desktop_main.cpp`. Interdit : `src/render/**`, `src/world/**`, `src/audio/**`, `track.*`, iOS (gel)

**WFS-142 — [CRITIQUE] Route de nuit : fin de la chaussée miroir** · E4 (exception : 2e ticket E4 actif à côté de WFS-134, WFS-109 fusionné ; blocs séparés) · P1 Urgent · 🔧 En cours — Claude (session 0183bv2V), 2026-11-05 21:01 UTC, branche `feature/waforspeed-142` (règles : `regles-de-gestion/WFS-142.md`, cause racine fichier:ligne, RG-01 à RG-12 ; preuve : `qa/retours/2026-11-05-route-nuit-miroir.jpg`). **Zone** : dans `shaders/scene.frag` et `shaders/scene_skin.frag` **seulement** le bloc `F_ROAD` / `wetRoad`, le `Fe` / `env` du sol et le bloc du reflet ; `shaders/common.glsl` ; `shaders/post.frag` (seuil du bloom) ; `src/render/renderer.cpp` **seulement** le bloc des reflets (l. 575-680, 950-952) ; `src/render/nuit.cpp` **seulement** `mouille()`, la ligne `fp.mouille / flaques / reflets` et les flaques de lumière (l. 156-175) ; `src/render/bench_nuit.*` ; `#include "meteo.h"` en lecture seule ; accroches ≤ 10 lignes annoncées : `desktop_main.cpp`, `configgame.*`, `configgame.json`. **Interdit** : bloc des feux de `renderer.cpp`, `feux_modele.*`, bloc des feux du joueur de `nuit.cpp`, `traffic_system.cpp` (WFS-109) ; le reste de `scene*.frag`, `qualite_*`, `vk.*`, `model_loader.cpp` (WFS-134) ; `src/world/**` (WFS-94), `src/game/**`, `src/ui/**`, iOS (gel). **Critères** : captures avant / après `route-nuit-ville-sec`, `route-nuit-ville-pluie`, `route-nuit-corniche`, `route-jour-sec`, `route-jour-pluie`, `route-nuit-ville-pluie-basse` ; rapport de luminance de la chaussée avec / sans reflet ≤ 1,10 au sec, 1,15 à 1,8 sous la pluie ; aucun immeuble lisible dans la route ; ANO-66 ; parcours complet à 0 écart. **Fusion** : pas de fusion par confiance ; CI verte + captures regardées + verdict `qa-lead` avant fusion ; résolu seulement après confirmation de l'utilisateur sur son téléphone.
- [ ] gyrophares petits et alternés ; bandeau hors de l'obstacle ; fins au gabarit Checkpoint, « 2e PLACE » ; panneau des modes opaque ; salon hors ligne lisible ; parcours RG-06 0 écart

**WFS-143 — [CI ROUGE] Valeurs de l'utilisateur dans le code + records de poursuite > 100** · E0 · P1 Urgent · ✅ Terminé (contenu fusionné par la PR #121 ; créé le 2026-11-05 21:28 UTC à la demande de Claude (session dev 5bx) ; règles : `regles-de-gestion/WFS-143.md`). Zone : `src/core/configgame.*`, `tests/test_configgame.cpp`, `tests/test_records.cpp` ; accroches ≤ 10 lignes annoncées : `sauvegarde.cpp` l. 150, `course_system.cpp` (banc des records), `poursuite*.cpp`. Interdit : `configgame.json` (valeurs de l'utilisateur), `src/render/**`, `shaders/**`, `src/world/**`, iOS (gel). Fusion par confiance permise.
- [ ] CI de `main` verte (`texte`, `vitrines`) ; `configgame.json` inchangé ; défauts du code = JSON ; poursuite à 1000 ; parcours des records 0 écart prouvé au Journal

**WFS-144 — Correction : appels de dessin de la poursuite en Basse + MSAA ×2 / aniso ×4 en Basse** · E4 · P2 Haute · ✅ Fusionné (QA de nuit à faire), PR #124, commit de fusion 588146a, 2026-11-06 — Claude (session trafic), 2026-11-06 00:19 UTC, branche `feature/waforspeed-144` (créé le 2026-11-05 à la demande de Claude (session trafic), suite de WFS-134 ; règles : `regles-de-gestion/WFS-144.md`). Zone : `src/render/**` (hors `ui_*`), `shaders/scene*`, `tests/test_qualite*` ; accroche ≤ 10 lignes : `poursuite_system.cpp` (choix du modèle de la police). Interdit : reste de `src/game/**`, `src/world/**`, `src/ui/**`, iOS (gel). Fusion par confiance permise si le banc `course5` local tient la cible.
- [ ] Banc `course5` Basse : poursuite − Checkpoint ≤ 60 ; aucune police en cubes ; « Rendu » Basse = MSAA ×2, aniso ×4 ; parcours complet 0 écart au Journal

**WFS-145 — [CI ROUGE] Job `endurance` de `main` en échec depuis WFS-94** · E7 · P1 Urgent · 🔧 En cours — Claude (session dev 5bx), 2026-11-06 00:56 UTC, branche `feature/waforspeed-145` (règles : `regles-de-gestion/WFS-145.md`). Zone : `src/platform/endurance_banc.*`, `tools/qa/**`, `src/world/**` (placement / mémoire du décor de WFS-94), `tests/**` ; accroche ≤ 10 lignes annoncée si la cause est dans `src/game/**`. Interdit : affaiblir le banc (150 s, seuil 10 %, `validation_connues.txt`), `src/render/**`, `src/ui/**`, iOS (gel).
- [ ] cause au Journal (fichier:ligne) ; `endurance` vert : Poursuite finie, 0 écart sur 10 cycles, mémoire ≤ 9 % ; CI de `main` verte

**WFS-146 — Lot anomalies INTERFACE, 2e passage (1/2) : textes et couleurs sans échelle** · E8 (+ E5) · P1 Urgent · ✅ Fusionné (PR #125, commit c60d75b, après CI de branche verte, QA de nuit à faire) — Claude (session trafic), 2026-11-06 02:38 UTC, branche `feature/waforspeed-146` (règles : `regles-de-gestion/WFS-146.md`). Zone : `src/ui/ecrans/**`, `src/ui/text.cpp`, `toasts.cpp`, `hud.cpp`, `shaders/ui*`. Interdit : toute échelle / taille / mise en page, `src/ui/echelle.*`, `src/render/**`, `src/game/**`.
- [ ] « — » au menu ; boutons sans lignes ; sélection cyan ; duel en parallélogrammes ; salon lisible ; `garage-*-2400` identiques à `main` ; parcours 0 écart

**WFS-147 — Lot anomalies INTERFACE, 2e passage (2/2) : tailles écran par écran** · E8 (+ E5) · P2 Haute · ✅ Fusionné (PR #126, commit eb8152a, `qa-lead` VALIDÉ au tour 2/2, `qa/rapports/WFS-147.md`) — Claude (session trafic), 2026-11-06 04:11 UTC, branche `feature/waforspeed-147` (WFS-146 ✅ ; règles : `regles-de-gestion/WFS-147.md`). Pas de fusion sans verdict `qa-lead`.
- [ ] écrans lisibles en 2400 un par un ; garage propre ; QA VALIDÉ

**WFS-60 — Roues animées : roues séparées, rotation et braquage (absorbe WFS-61, WFS-62)** · E5 (+ API E0) · priorité P2 Haute · ✅ Fusionné le 2026-11-04 par confiance (PR #48, QA de nuit : `qa/Check.md`) (règles : `regles-de-gestion/WFS-60.md`, `WFS-61.md`, `WFS-62.md`)
- [ ] 4 roues distinctes (centre, rayon) par voiture ; voiture à l'arrêt identique à avant
- [ ] Rotation et braquage par roue dans l'API du rendu ; sans appel, rien ne change
- [ ] Roues immobiles à l'arrêt, qui tournent en roulant, avant braquées en virage (joueur et trafic proche)

**WFS-61 — Roues animées : transformation par roue et animation (rotation, braquage)** · E0 · ↪ fusionné dans WFS-60 (règles : `regles-de-gestion/WFS-61.md`)
- [ ] Rotation et braquage par roue dans l'API du rendu ; sans appel, rien ne change

**WFS-62 — Roues animées** · E3 · priorité P4 Basse · ↪ fusionné dans WFS-60 (via WFS-61) (règles : `regles-de-gestion/WFS-62.md`)
- [ ] Roues immobiles à l'arrêt, qui tournent en roulant, avant braquées en virage

**WFS-16 — Route mouillée avec reflets** · E4 · priorité P4 Basse · ⏳ En attente — ↪ fusionné dans WFS-09
- [ ] Reflets plausibles, coût maîtrisé

**WFS-18 — Météo : pluie, flaques** · E2 · priorité P4 Basse · ✅ Fusionné (QA de nuit à faire), PR #66 — Claude (session trafic, agent WFS-18), 2026-11-04, branche `feature/waforspeed-18` (règles : `regles-de-gestion/WFS-18.md`)
- [ ] Pluie animée, route mouillée

**WFS-17 — Post-traitement : bloom, TAA, flou de mouvement** · E4 · priorité P4 Basse · ⏳ En attente (règles : `regles-de-gestion/WFS-17.md`) — ↪ fusionné dans WFS-15
- [ ] Activable selon la qualité
- [ ] `vitrine-golf-0`, `vitrine-p911-1` : plus de lignes pointillées sur les jointures de carrosserie (reste de WFS-32, crénelage de géométrie lissé par le TAA)

**WFS-24 — Sauvegarde et progression** · E3 · priorité P3 Moyenne · ✅ Fusionné (QA de nuit à faire), PR #54 — Claude (session trafic), 2026-11-04, branche `feature/waforspeed-24` (règles : `regles-de-gestion/WFS-24.md`)
- [x] Meilleurs scores, voitures et réglages conservés

**WFS-21 — Mode duel en ligne** · E3 · priorité P2 Haute · ✅ Fusionné (PR #38, QA de nuit à faire) — Claude (agent 4, session 011cpqZan), 2026-11-03, branche `feature/waforspeed-21` (règles : `regles-de-gestion/WFS-21.md`)
- [ ] Compatible avec le principe du duel web

**WFS-23 — Manettes de jeu** · E7 · priorité P4 Basse · ⏳ En attente — ↪ fusionné dans WFS-22
- [ ] Manettes Bluetooth Android

**WFS-22 — Plateformes : iOS, manettes et préparation des stores** · E7 · priorité P4 Basse · ✅ Fusionné (QA de nuit à faire), PR #60 et #62 — Claude (session trafic), 2026-11-04, branche `feature/waforspeed-22` (règles : `regles-de-gestion/WFS-22.md`)
- [ ] Construction iOS, test TestFlight

**WFS-25 — Préparation des stores** · E7 · priorité P4 Basse · ⏳ En attente — ↪ fusionné dans WFS-22
- [ ] Icônes, signature, fiches

**WFS-74 — Changement de carte avec écran de chargement, course figée hors de l'écran Course** · E0 · P2 Haute · ✅ Fusionné le 2026-11-04 (PR #50, par confiance ; QA de nuit : `qa/Check.md`) (règles : `regles-de-gestion/WFS-74.md`)
- [ ] `terrain-chargement` (barre partielle + étape) puis `terrain-arrivee` (nouvelle carte, décompte, chrono et score à zéro, voiture du garage gardée)
- [ ] `terrain-fige-1/2` : chrono et score identiques au journal derrière l'écran Terrain ; pause → reprise sans remise à zéro ; autres captures inchangées

**WFS-75 — Réglages : commandes gaucher et vibrations** · E8 · P3 Moyenne · ✅ Fusionné (QA de nuit à faire), PR #65, fusion par confiance — Claude (session trafic, agent WFS-75), 2026-11-04, branche `feature/waforspeed-75` (règles : `regles-de-gestion/WFS-75.md`)
- [ ] `reglages-commandes` (Commandes + Vibrations) ; `course-gaucher` (volant à droite, pédales et nitro à gauche) ; vibrations coupées → `vibration ignorée (réglage)`

**WFS-76 — Ambiances sonores par carte et musique (menu et course)** · E6 · P3 Moyenne · ✅ Fusionné le 2026-11-04 (PR #55, par confiance ; QA de nuit : `qa/Check.md`) — Claude (agent 5, session 011cpqZaDiS2u), 2026-11-04, branche `feature/waforspeed-76` (règles : `regles-de-gestion/WFS-76.md`)
- [ ] journal `MUSIQUE menu|course` et `AMBIANCE <carte>|nuit` aux bons moments ; WAV + PNG du banc sans coupure (crête < 1,0) ; capture `reglages-musique` ; téléphone : musique au menu, ambiance ville ≠ plage, pas de craquement

**WFS-77 — Les 20 voitures sans défaut visible** · E5 · P2 Haute · ✅ Fusionné (PR #57, 0f77ae4, sur ordre de l'utilisateur ; QA de nuit à faire) — Claude (agent 5, session 011cpqZaDiS2u), 2026-11-04, branche `feature/waforspeed-77` (règles : `regles-de-gestion/WFS-77.md`)
- [ ] `vitrine-giulia-0` / `vitrine-elantra-0` : couleur unie, aucun camouflage ; la teinte suit la peinture choisie au garage
- [ ] journal : « gt86.wfm : 4 roues séparées », plus aucune « roues non trouvées » ; `vitrines-planche` : 20 voitures propres (sens, 4 roues au sol, Turquoise) ; `.wfm` < 9 Mo

**WFS-78 — iOS : surface Metal et démarrage jusqu'au menu** · E7 · P4 Basse · ✅ Fusionné (QA de nuit à faire), PR #64, 2ac79d4 — surface Metal créée (swapchain 2622x1206) ; « QA pret » pas encore atteint sur le simulateur (blocage dans `Renderer::init`, suite à donner par un ticket E4) — Claude (session trafic, agent WFS-78), 2026-11-04 (règles : `regles-de-gestion/WFS-78.md`)
- [ ] run `ios.yml` vert : durées des étapes puis `QA pret` ; `ios-menu.png` = menu, `ios-course.png` = course ; surface Metal, plus de recopie PNG ; CI Android et banc inchangés

**WFS-79 — Son iOS** · E6 · P4 Basse · ✅ Fusionné (QA de nuit à faire), PR #63 — Claude (session trafic, agent WFS-79), 2026-11-04 (règles : `regles-de-gestion/WFS-79.md`)
- [ ] journal du simulateur `audio iOS : 48000 Hz` puis `n rappels` > 0 ; son Android et WAV du banc inchangés

**WFS-80 — Manette au focus + réglages Direction et Réduire les mouvements** · E8 · P3 Moyenne · ✅ Fusionné (QA de nuit à faire), PR #74 — Claude (session trafic), 2026-11-04, branche `feature/waforspeed-80` (règles : `regles-de-gestion/WFS-80.md`). Zone : `src/ui/ecrans/reglages.*` ; accroches ≤ 10 lignes annoncées dans `src/ui/garage.cpp` et, si besoin, `src/platform/manette_jeu.*`.
- [ ] `reglages` : lignes « Direction » (Glisser / Flèches, clé `steer`) et « Réduire les mouvements » (Non / Oui) visibles ; le choix s'applique tout de suite (`reglagesRessenti()`) et survit au relancement
- [ ] `manette-garage-peinture`, `manette-reglages` : cadre cyan, A applique, B retour ; captures tactiles inchangées

**WFS-81 — Nuit corniche et circuit** · E2 · P3 Moyenne · ✅ Fusionné (QA de nuit à faire), PR #75 — Claude (session trafic, agent WFS-81), 2026-11-04, branche `feature/waforspeed-81` (règles : `regles-de-gestion/WFS-81.md`)
- [ ] `nuit-plage-*` : poteaux sous chaque lumière ; `nuit-circuit-*` : lumière sous chaque mât ; trafic qui freine la nuit : un seul halo rouge par feu

**WFS-82 — [STABLE] Performances téléphone : masquage** · E4 · P2 Haute · ✅ Fusionné le 2026-11-04 par confiance (PR #67, QA de nuit : `qa/Check.md`) — Claude (agent 4, session 011cpqZan), 2026-11-04, branche `feature/waforspeed-82` (règles : `regles-de-gestion/WFS-82.md`)
- [ ] banc nuit ville : Basse ≤ 300 appels de dessin (**mesuré 331 : cible assouplie par le PO le 2026-11-04**, instanciation des décors reportée), Moyenne ≤ 450 (337, atteint), Haute sans hausse ; avant/après au Journal
- [ ] captures `qualite-*` et partie1 : rien ne manque à moins de 150 m ; phares et feux du trafic restent attachés à leur voiture
- [ ] test CPU `masquage` vert ; aucun plantage, aucune régression du banc

**WFS-83 — Duel jouable sur le même Wi-Fi** · E3 · P2 Haute · ✅ Fusionné le 2026-11-04 (PR #68, par confiance ; QA de nuit : `qa/Check.md`) — Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04, branche `feature/waforspeed-83` (règles : `regles-de-gestion/WFS-83.md`)
- [ ] test CPU : deux `TransportUdp` en boucle locale (annonce, rejoindre, prêts, même graine, horloge commune, message hostile ignoré, retrait après 3 s)
- [ ] banc : `menu-duel`, `salon-accueil`, `salon-hote`, `salon-pret`, `duel-reseau-course` (1280x720 et `-2400`)
- [ ] APK : « Duel en ligne » ouvre le Salon ; sans Wi-Fi, note orange, aucun plantage

**WFS-87 — [URGENT] CI : APK de `main` toujours publié** · E7 · P1 Urgent · ✅ Fusionné par l'utilisateur (PR #73, 3b62e5f ; CI de main verte, run 41860181020) — Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04, branche `feature/waforspeed-87` (règles : `regles-de-gestion/WFS-87.md`). Zone : `.github/workflows/build.yml` (job `publication`), `tools/qa/publier_apercu.sh` ; ne pas toucher `ios.yml`, `ios/**` (WFS-78).
- [ ] CI de `main` après fusion : job `publication` vert, `releases/download/apercu/WaForSpeed.apk` répond
- [ ] Release `apercu` sur le dernier commit, captures à la même adresse ; jamais de moment sans APK (plus de suppression avant création)
- [ ] branche `feature/*` : `apercu-waforspeed-87` publiée

**WFS-88 — iOS : `Renderer::init` ne se bloque plus** · E4 · P4 Basse · ⏸ Suspendu (GEL iOS) ; PR #76 à regarder et fusionner par WFS-103 (ordre de l'utilisateur, 2026-11-04) — Claude (session trafic), 2026-11-04, branche `feature/waforspeed-88` (règles : `regles-de-gestion/WFS-88.md`). Zone : `src/render/**` (+ accroche ≤ 10 lignes `ios/**` / `ios.yml` annoncée)
- [ ] `ios.yml` : « QA pret » atteint, `ios-menu.png` montre le menu
- [ ] plus de trou de 60 s après l'étape vulkan ; CI Android verte, banc PC inchangé

**WFS-89 — Duel par Internet (code à 4 caractères, compatible jeu web)** · E3 · P2 Haute · ✅ Fusionné (PR #77, fb50d09, revue WFS-103 ; WebRTC réel → WFS-92) — Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04, branche `feature/waforspeed-89` (règles : `regles-de-gestion/WFS-89.md`). Zone : `src/game/duel*`, `src/game/reseau_peerjs.*`, `src/game/binarypack.*`, `tests/**` ; accroches annoncées : `src/ui/ecrans/salon.*`, `CMakeLists.txt` ≤ 10 lignes, manifeste Android ≤ 2 lignes
- [ ] `salon-code`, `salon-code-saisi` : bloc « Rejoindre avec un code », 4 cases, clavier à l'écran, « Rejoindre » actif à 4 caractères
- [ ] `salon-internet` (relais simulé : 2 pilotes) et `salon-hors-ligne` (note orange, pas de plantage)
- [ ] `test_binarypack` vert (octets de référence PeerJS) ; APK : duel entre deux réseaux différents et avec le jeu web (vérifié par l'utilisateur)

**WFS-91 — [STABLE] Endurance : parcours long, validation Vulkan, retours d'arrière-plan** · E7 · P2 Haute · ✅ Fusionné par confiance (PR #92, 2026-11-05) — Claude (agent 3, session 011cpqZd), version allégée (section « Attribution du 2026-11-05 » des règles : 8 cycles, ≥ 10 min simulées, ≥ 3 retours, aucune correction hors E7). **+ ANO-52 (bloquante, 2026-11-05)** : correction immédiate du plantage au retour d'arrière-plan dans `android_main.cpp` (`Renderer` neuf après `shutdown`, RG-14 à RG-16) ; correction durable du renderer dans WFS-100 (RG-W1). Historique : remis en ⏳ Prêt le 2026-11-04 (aucune branche poussée depuis l'attribution à Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), ordre de l'utilisateur ; « à démarrer dès WFS-89 fusionné » ne valait que pour l'agent 5, occupé par WFS-89 : aucune dépendance, zone disjointe de WFS-89), branche `feature/waforspeed-91` (règles : `regles-de-gestion/WFS-91.md`). Zone : `src/platform/**` (nouveau `endurance_banc.*`, `desktop_main.cpp`), `.github/workflows/build.yml` (job `endurance`), `tools/qa/**` ; corrections ailleurs : accroches ≤ 10 lignes annoncées, jamais dans les zones de tickets en cours
- [ ] job `endurance` vert sur `main` (≤ 15 min), artefact `captures-endurance`
- [ ] `endurance-journal.txt` : ≥ 12 cycles, 4 cartes, jour et nuit, ≥ 4 retours d'arrière-plan, RSS final ≤ cycle 2 + 10 %, 0 erreur de validation (hors erreurs connues avec ticket)
- [ ] `endurance-retour-*.png` : image correcte après chaque retour ; une capture de fin de course par carte ; autres jobs verts, APK publié
- [ ] (ajout PO, ANO-29) Release `apercu` jamais incomplète : publication `cancel-in-progress: false`, anciens fichiers supprimés après l'envoi complet des nouveaux (RG-A1, RG-A2)

**WFS-92 — Duel par Internet : pair PeerJS réel** · E3 · P2 Haute · ⏸ En attente de la décision de l'utilisateur sur l'ajout de libdatachannel 0.22.6 + Mbed TLS 3.6.2 dans `third_party/` (≈ 14 Mo, recommandation PO : autoriser) — branche `feature/waforspeed-92` (règles : `regles-de-gestion/WFS-92.md`). Zone : `src/game/reseau_peerjs.*`, `third_party/**` (nouvelles bibliothèques) ; accroches : `CMakeLists.txt`, `android/**` ≤ 10 lignes
- [ ] APK : duel entre deux réseaux différents et contre le jeu web (vérifié par l'utilisateur)
- [ ] sans Internet : note orange, aucun plantage ; CI verte, APK publié

**WFS-93 — Ville : 6 quartiers + finitions des cartes** · E2 · P2 Haute · ✅ Terminé (PR #89, fusion par confiance le 2026-11-05, commit de fusion 1df6dc0 ; QA de nuit à faire, `qa/Check.md`) — Copilot (session 0e5c192d), 2026-11-04 (après remise en ⏳ Prêt le 2026-11-04 : aucune branche poussée depuis l'attribution à Claude (session trafic), ordre de l'utilisateur), branche `feature/waforspeed-93` (règles : `regles-de-gestion/WFS-93.md`). Zone : `src/world/**` (nouveau `ville_quartiers.cpp`, `ville.cpp`, `circuit.cpp`, `campagne.cpp`, `cartes_banc.cpp`) ; interdit : `src/render/**` et `shaders/**` (WFS-100, WFS-101), `src/platform/**` et `.github/**` (WFS-106 : captures dans le groupe `cartes` existant), `src/game/**` (WFS-105, WFS-108), `src/ui/**`, fichiers du duel, iOS
- [ ] `carte-ville-residentiel`, `-parc`, `-port` : objets du quartier (abribus ; grille, bancs, fontaine ; entrepôts, conteneurs, grues) ; `course-*` sans régression : `course-cam0-*` centre-ville (immeubles replacés par les tirages du web), `course-cam1-*` quartier résidentiel (précision du PO, `WFS-93.md`)
- [ ] `carte-circuit-jour` / `-nuit` : texte d'AD_TEXTS lisible sur les panneaux ; `carte-campagne-vignes` : rangs sans marches ; ≤ 300 appels de dessin

**WFS-94 — Corniche : quartiers marina, rochers, village + ville de nuit (fin d'ANO-37) + finitions de WFS-93 (ANO-49, ANO-51)** · E2 · P2 Haute · ✅ Fusionné (QA de nuit à faire), PR #123, commit a722e6c, 2026-11-06, fusion par confiance — Claude (session trafic, agent reprise), 2026-11-05 (repris de Copilot sur demande de l'utilisateur), ex-Copilot (session 0e5c192d), démarre à la fusion de WFS-109 ; branche `feature/waforspeed-94` reprise (seule l'étape 0, PR #76, a été faite ; aucun commit de code sur la branche, d42c3f0 d'attribution : section « Reprise du 2026-11-05 » des règles) (règles : `regles-de-gestion/WFS-94.md` : RG-A2 à RG-A5, puis section « Attribution du 2026-11-05 », RG-06 et RG-07). **Étape 0** : regarder et fusionner la PR #76 (WFS-88, iOS ; confiée par le PO, RG-10 de WFS-103), sauf si elle est déjà fusionnée. Zone : `src/world/**` (`plage.cpp`, nouveaux `src/world/plage_*.cpp`, `cartes_banc.cpp`, banc de `commerces.cpp` (ANO-40), `ville.cpp` (façades et fenêtres de nuit, trottoir), `ville_quartiers.*` (trottoir, ANO-49), `enseignes.*` / `circuit.cpp` (ANO-51), `decor_carte.*` pour les primitives) ; `#include "effets_particules.h"` permis, sans le modifier ; interdit : le reste de `src/render/**`, `shaders/**` (WFS-100, WFS-109), `src/game/**` dont `WALK_W` (accroches de WFS-102), `src/ui/**` (WFS-102), `src/platform/**` et `.github/**` (WFS-106), noms et API de `zones.h`, fichiers du duel, iOS (hormis la fusion de #76)
- [ ] `carte-plage-marina`, `-rochers`, `-village` : bateaux, bornes, villas, rochers, glissières visibles ; ≤ 300 appels de dessin
- [ ] (ajout PO, ANO-40) `vie-portique` à 25 m (« CHECKPOINT 1 » lisible), `vie-plage` à 15 m (serviettes et baigneurs visibles)
- [ ] (ajout PO, ANO-37 fin) `course-nuit-ville-*` : façades sombres jusqu'au fond de rue, même dans l'axe des phares ; environ une fenêtre sur trois allumée (quads émissifs, RG-A4) ; captures de jour inchangées
- [ ] (ANO-49) `carte-ville-residentiel`, `-parc`, `-port`, `course-cam0-*`, `vie-terrasse` : trottoir gris continu de la chaussée jusqu'aux façades (RW + 6 m), plus de bande d'herbe ; terrasses posées sur le trottoir
- [ ] (ANO-51) `carte-circuit-jour`, `carte-circuit-nuit` : mots entiers sur les panneaux lointains (plus de lettres en morceaux) ; enseignes de la ville toujours nettes de près
- [ ] #76 fusionnée (ou déjà fusionnée) avec sa ligne `qa/Check.md` ; tests CPU verts

**WFS-95 — Texte et bandeaux** · E5 · P2 Haute · ⏳ Prêt — remis en ⏳ Prêt le 2026-11-04 (aucune branche poussée depuis l'attribution à Claude (session trafic), ordre de l'utilisateur), branche `feature/waforspeed-95` (règles : `regles-de-gestion/WFS-95.md`). Zone : `src/ui/text.*`, `src/ui/toasts.*`, `shaders/ui*`, `assets/fonts/**` ; interdit : `src/ui/garage.*`, `src/ui/ecrans/**`, `src/render/**`, `src/platform/**` — ↪ fusionné dans WFS-102
- [ ] menu : « — » sous RECORD (plus de « ? »), « TA VOITURE » lisible ; capture `toast-quartier` : bandeau du nom du quartier à chaque changement de zone

**WFS-96 — Menus : Showroom, quartiers du terrain, focus manette, voile, sons** · E8 · P2 Haute · ✅ Fusionné (PR #79, 6863763, revue WFS-103) — Claude (session trafic), 2026-11-04, branche `feature/waforspeed-96` (règles : `regles-de-gestion/WFS-96.md`). Zone : `src/ui/ecrans/**` sauf `salon.*` ; accroches ≤ 10 lignes annoncées : `src/ui/garage.cpp`, `src/platform/manette_jeu.cpp`, `src/audio/sfx.*` ; interdit : `desktop_main.cpp`, `.github/**`, `src/render/**`, `src/ui/text.*`, `toasts.*`
- [ ] `menu-showroom`, `showroom-choisir` : Showroom ouvre le garage, « Choisir » ramène au menu ; `terrain-0` : « Quartiers » et noms ; voile du menu sans rayures ; focus manette sur le 1er choix ; « SFX garage » / « SFX carte » au journal

**WFS-97 — Lot anomalies NUIT (circuit + ville, ANO-37)** · E4 (exception : en parallèle de WFS-100, fichier séparé) · P1 Urgent · ✅ Terminé (fusion par confiance, PR #90, `dcfa0ff`, 2026-11-05 ; recette QA de nuit à faire, `qa/Check.md`) — Copilot (session dc0cdd3f), 2026-11-05 (après remise en ⏳ Prêt le 2026-11-04 : aucune branche poussée depuis l'attribution à Claude (session trafic)), branche `feature/waforspeed-97` (règles : `regles-de-gestion/WFS-97.md`, précisions RG-04 à RG-07 du 2026-11-05). Zone : `src/render/nuit.cpp` (sauf le bloc « feux du joueur », réservé à WFS-109) ; `#include "effets_particules.h"` permis (`effets::versHdr`, sans modifier ce fichier) ; accroche ≤ 10 lignes dans `desktop_main.cpp` seulement si une capture de nuit manque ; **accroche d'une ligne autorisée le 2026-11-05 dans `src/world/decor_carte.cpp` l. 320** (ordre des indices de la chaussée du circuit, normale vers le haut : RG-08). Interdit : le reste de `src/world/**` (WFS-93), `src/render/renderer.*` et `shaders/**` (WFS-100), le reste de `src/platform/**`, `src/game/**`, iOS
- [ ] `carte-circuit-nuit`, `course-nuit-circuit-0` : taches froides visibles sous les mâts ; ville et plage de nuit sans surexposition
- [ ] `course-nuit-ville-*` : façades sombres (plus de lavande / rose), néons, flaques et reflets qui ressortent (ANO-37, RG-04) ; captures de jour inchangées, **sauf l'asphalte du circuit enfin éclairé par le soleil** (RG-09, voulu, valeurs avant / après dans `qa/Check.md`) ; valeurs notées au Journal ; tests CPU verts

**WFS-98 — Correction : commandes tactiles, HUD cockpit, caméra du demi-tour** · E3 (+ E0 `game.cpp`, E5 `hud.*`) · P1 Urgent · ✅ Fusionné le 2026-11-04 par confiance (PR #80, QA de nuit : `qa/Check.md`) — Copilot (session 0e5c192d), 2026-11-04 (réattribué : aucune branche poussée depuis l'attribution à Claude (session trafic) à 18:42:05, ordre de l'utilisateur), branche `feature/waforspeed-98` (règles : `regles-de-gestion/WFS-98.md`). Zone : `src/game/game.cpp` (commandes, caméra de poursuite), `direction.*`, `camera_vue.*`, `camera_banc.*` (si besoin), `conduite_libre_banc.*`, nouveau `commandes_tactiles.*`, `src/ui/hud.*`
- [ ] `hud-course`, `encoche-hud-2400` : pédales « GAZ » orange et « FREIN » rouge, nitro rond à anneau vert acide « NITRO », volant dessiné ; boutons du HUD sans rayures ; `hud-nitro` : gaz appuyé (remplissage orange) ; `course-gaucher` : commandes en miroir, gaz appuyé (RG-05 bis)
- [ ] `camera-cockpit` : un seul compteur, plus de disque cyan ; `conduite-demitour` : Golf entière au tiers bas, en travers ; tests CPU verts

**WFS-99 — Correction : garage lisible** · E5 · P1 Urgent · ✅ Terminé (PR #81, fusion par confiance ; QA de nuit : **PASSABLE AVEC ANOMALIES MINEURES**, ANO-47 → WFS-102, [rapport](qa/rapports/WFS-99.md)) — Copilot (session 0e5c192d), 2026-11-04 (après remise en ⏳ Prêt le 2026-11-04 : aucune branche poussée depuis l'attribution à Claude (session trafic), ordre de l'utilisateur), branche `feature/waforspeed-99` (règles : `regles-de-gestion/WFS-99.md`). Zone : `src/ui/garage.*`, `planche_voitures.*`, `src/render/bench_spots.*`
- [ ] `spots-sol`, `garage-0`, `garage-2` : 3 taches de lumière au sol, 911 noire détachée du fond (liseré cyan)
- [ ] `garage-*` : aucun néon sur le texte, voile à gauche, voiture de 3/4 avant, panneau sans escalier

**WFS-100 — Correction : carrosseries propres et bloom sans fantômes** · E4 · P1 Urgent · ✅ Fusionné (PR #94, 9d50529) — Claude (session trafic), 2026-11-04 (réattribué après remise en ⏳ Prêt le 2026-11-04), branche `feature/waforspeed-100` (règles : `regles-de-gestion/WFS-100.md`). Zone : `src/render/renderer.cpp`, `vk.*`, `model_loader.*`, `qualite_*`, `shaders/scene*`, `post*`, `common.glsl`
- [ ] `vitrine-*`, `garage-peinture*`, `trafic-clignotant` : aucun trait brillant dans le vide ; Giulia, X5, Model 3 en peinture unie
- [ ] `qualite-haute`, `qualite-moyenne`, `course-nuit-*` : halo rond et doux, aucune copie fantôme ni traînée

**WFS-101 — Correction : étincelles, nitro, trafic du banc, écran de fin** · E4 (+ E3) · P1 Urgent · ✅ Terminé (fusion par confiance, PR #84, `a256432` ; QA de nuit : **VALIDÉ**, [rapport](qa/rapports/WFS-101.md)) — Copilot (session dc0cdd3f), 2026-11-04 (après remise en ⏳ Prêt le 2026-11-04 : aucune branche poussée depuis l'attribution à Claude (session trafic), ordre de l'utilisateur), branche `feature/waforspeed-101` (règles : `regles-de-gestion/WFS-101.md`). Zone : `src/render/effets_*`, `bench_effets.*`, `src/game/traffic_system.cpp` (`benchTraficVivant`), `src/game/course_system.*`
- [x] `choc-etincelles` : gerbe orange et blanche (prise de côté) ; `nitro-lignes` : traits en étoile + vignette cyan sombre
- [x] `trafic-clignotant`, `trafic-freinage` : voiture à 8-25 m ; `fin-record` : badge et record espacés

**WFS-102 — Lot anomalies INTERFACE (absorbe WFS-95 et ANO-47)** · E8 (+ E5 hors navigation du garage, accroches E0 / E7) · P1 Urgent · ✅ Fusionné (PR #118, commit a6303f5, QA de nuit à faire) — Claude (session dev 5bx), 2026-11-05 19:58 UTC (ex-Copilot session 1ef8d4d6, remis en Prêt à 19:15 faute de branche ; **pousser la branche dans les 2 h**), branche `feature/waforspeed-102` (règles : `regles-de-gestion/WFS-102.md` + `WFS-95.md`, section « Attribution du 2026-11-05 »). Dépendances toutes ✅ (WFS-96, 89, 98, 99, 101, 105, 108 ; WFS-95 absorbé, WFS-91 levé). **Zone** : `src/ui/ecrans/**` ; nouveau `src/ui/echelle.*` ; `src/ui/text.*`, `src/ui/toasts.*`, `shaders/ui*`, `assets/fonts/**` (WFS-95) ; `src/ui/hud.*` (échelle, libellé NITRO) ; `src/ui/garage.cpp` **seulement** les lignes d'échelle et les petits libellés (ANO-47), la navigation (flèches, gestes, index) étant à WFS-113 ; `src/game/camera_vue.cpp` (biseau, échelle), `src/game/course_system.cpp` (échelle, bandeau du quartier ≤ 10 lignes), `src/game/duel_system.cpp` (écran de résultat), `src/game/commandes_tactiles.*` (zones de toucher), `src/platform/manette_jeu.cpp` (échelle). **Densité** : celle de WFS-108 (`densiteCommandes()`), jamais relue. Accroches ≤ 10 lignes annoncées au Journal : `src/game/game.h` (densité), `src/game/game.cpp` (`drawControls`, RG-15), `src/platform/android_main.cpp` (`CMakeLists.txt` inutile : `GLOB_RECURSE` prend `src/ui/*.cpp`). **Interdit** : `src/render/**` (WFS-100, WFS-107 : laisser le `/ 720` de `effets_systeme.cpp`), `shaders/**` hors `ui*`, `src/world/**` (WFS-93 : `zones.h` en lecture seule, laisser `meteo.cpp`), `.github/**` et l'aiguillage `WFS_BANC` de `desktop_main.cpp` (WFS-106), navigation de `garage.cpp` et `src/ui/appui.h` (WFS-113, fusionné), iOS (gel)
> ⛔ **2026-11-05 : ANNULÉ sur `main`** (revert 741c998 de la PR #118, sur ordre de l'utilisateur). Sur son téléphone (2400x1080), l'échelle en dp grossissait tout le garage : statistiques et boutons par-dessus la voiture, titre sur la voiture. Le banc PC (1280x720, densité 1) ne le montrait pas. Ticket à reprendre par le PO, avec captures 2400x1080 regardées avant fusion.
- [ ] `*-2400` (`menu-1` = menu en 2400, `reglages-menu-2400`, `salon-*-2400`, `hud-2400`, `terrain-*-2400`, `garage-*-2400`) : boutons ≥ 44 dp (≥ 115 px), texte ≥ 12 dp (petites étiquettes ≥ 10 dp) ; captures 1280x720 à la même échelle (k = 1) ; aucun bouton rayé sur aucun écran
- [ ] `menu-0` : « — » sous RECORD, aide selon le réglage Direction ; texte courant lu sans confusion a / o ni virgule / point (`terrain-0`, `menu-0`, `salon-*`) ; `garage-sf90` « HYBRIDE » et `nitro-lignes` « NITRO » complets (ANO-47) ; nouvelle capture `toast-quartier`
- [ ] `terrain-*` : description sur fond sombre, sélection cyan, illustration sans bandes ; `salon-*` : carte centrale, aide sous le code, touche effacer lisible ; `duel-resultat` : biseaux et « 2e PLACE » ; `chargement-*` : reflets orange et cyan ; `chargement-erreur` : « Réessayer »
- [ ] clavier du code ≥ 44 dp ; aucune commande tactile pendant le fondu (`chargement-fondu-2400`) ni à la fin de course (`course-fin`) ; tests CPU verts ; parcours chargement → menu → course → fin inchangé

**WFS-103 — Revue et fusion des PR en attente** · E7 (transverse) · P1 Urgent · ✅ Terminé (#77, #79, #76 fusionnées ; CI de main verte, run 42006825688) — Claude (session trafic), 2026-11-04 (règles : `regles-de-gestion/WFS-103.md`). Zone : fichiers des PR revues (résolution de conflits seulement), `TACHES.md`, `qa/Check.md`, `journal.md`, `anno.md` ; aucune nouvelle fonctionnalité, rien d'iOS hormis la fusion de #76. Ordre : #77 (WFS-89) puis #79 (WFS-96, non réattribué), puis #76 (WFS-88). **Décision du PO (2026-11-05)** : WFS-103 n'est pas mis en pause malgré le gel iOS (ordre explicite de l'utilisateur pour #76, seule exception) ; Claude (session trafic) sans activité depuis le 2026-11-04 22:29, échéance RG-10 (01:13:25) dépassée : **#76 confiée à Copilot (session 0e5c192d)**, étape 0 de WFS-94 (relecture faite par le PO à 22:59, fusion sans conflit avec `main` 6e09c9f vérifiée par `git merge-tree`). WFS-88 reste ⏸ (aucune suite iOS).
- [ ] #77 et #79 fusionnées, ou blocage précis écrit au Journal (PR laissée ouverte)
- [ ] #76 (WFS-88, iOS) fusionnée : synchronisée avec `main`, compilation et tests CPU, ligne `qa/Check.md` (fusion par confiance), **avant** le code de WFS-100 dans `renderer.cpp` / `vk.cpp`
- [ ] CI de `main` verte après la dernière fusion ; `TACHES.md`, `qa/Check.md`, Journal à jour pour WFS-89 et WFS-96
- [ ] **Règle permanente** : aucune PR ouverte plus de 2 h sans décision (fusionnée, bloquée au Journal, ou gel iOS)

**WFS-148 — Référentiel du réel et banc de fidélité : données sourcées, 12 vues de référence, mesures automatiques, état zéro** · E0 · P2 Haute · ✅ Terminé (PR #127, commit 0d923d5, fusion par confiance ; état zéro au Journal) — expert-realisme-3d (session 0183bv2V), 2026-11-06 01:13 UTC, branche `feature/waforspeed-148` · dépend de : — (règles : `regles-de-gestion/WFS-148.md` ; charte : `regles-de-gestion/DA-realisme.md`)
- [ ] critères et mesures de fidélité du fichier de règles ; indice ≥ 99 % dans `realisme-fidelite.txt` (avant / après)
- [ ] captures `realisme-*` avant / après regardées ; budget de performance tenu ; tests CPU et parcours complet verts

**WFS-149 — Socle : textures compressées KTX2 (ASTC / ETC2) + mipmaps, tangentes MikkTSpace, shaders découpés par domaine** · E0 · P2 Haute · 🔍 PR #132 (CI en cours) — Claude (session trafic), 2026-11-06 09:40 UTC, branche `feature/waforspeed-149` · dépend de : WFS-148 ✅ (règles : `regles-de-gestion/WFS-149.md` ; charte : `regles-de-gestion/DA-realisme.md`)
- [ ] critères et mesures de fidélité du fichier de règles ; indice ≥ 99 % dans `realisme-fidelite.txt` (avant / après)
- [ ] captures `realisme-*` avant / après regardées ; budget de performance tenu ; tests CPU et parcours complet verts

**WFS-150 — Socle : passes de rendu enfichables, pré-passe, capture d'environnement, niveau Ultra, coût GPU par passe** · E0 · P2 Haute · ⏳ En attente · dépend de : WFS-149 validée (règles : `regles-de-gestion/WFS-150.md` ; charte : `regles-de-gestion/DA-realisme.md`)
- [ ] critères et mesures de fidélité du fichier de règles ; indice ≥ 99 % dans `realisme-fidelite.txt` (avant / après)
- [ ] captures `realisme-*` avant / après regardées ; budget de performance tenu ; tests CPU et parcours complet verts

**WFS-151 — Lumière : exposition physique (EV100, lux), IBL préfiltrée, sondes locales, étalonnage charte grise / ColorChecker** · E13 · P2 Haute · ⏳ En attente · dépend de : WFS-150 validée (règles : `regles-de-gestion/WFS-151.md` ; charte : `regles-de-gestion/DA-realisme.md`)
- [ ] critères et mesures de fidélité du fichier de règles ; indice ≥ 99 % dans `realisme-fidelite.txt` (avant / après)
- [ ] captures `realisme-*` avant / après regardées ; budget de performance tenu ; tests CPU et parcours complet verts

**WFS-152 — Lumière : ciel physique, soleil et lune calculés (NOAA), 6 moments + heure réelle, brouillard de distance** · E13 · P2 Haute · ⏳ En attente · dépend de : WFS-151 validée (règles : `regles-de-gestion/WFS-152.md` ; charte : `regles-de-gestion/DA-realisme.md`)
- [ ] critères et mesures de fidélité du fichier de règles ; indice ≥ 99 % dans `realisme-fidelite.txt` (avant / après)
- [ ] captures `realisme-*` avant / après regardées ; budget de performance tenu ; tests CPU et parcours complet verts

**WFS-153 — Lumière : ombres en cascades à pénombre physique, ombres des lumières la nuit, occlusion ambiante, contact** · E13 · P2 Haute · ⏳ En attente · dépend de : WFS-151 validée (après WFS-152, même épic) (règles : `regles-de-gestion/WFS-153.md` ; charte : `regles-de-gestion/DA-realisme.md`)
- [ ] critères et mesures de fidélité du fichier de règles ; indice ≥ 99 % dans `realisme-fidelite.txt` (avant / après)
- [ ] captures `realisme-*` avant / après regardées ; budget de performance tenu ; tests CPU et parcours complet verts

**WFS-154 — Lumière : reflets d'écran (SSR), bloom par énergie, flou de mouvement par objet, TAA, phares de face** · E13 · P2 Haute · ⏳ En attente · dépend de : WFS-153 validée (règles : `regles-de-gestion/WFS-154.md` ; charte : `regles-de-gestion/DA-realisme.md`)
- [ ] critères et mesures de fidélité du fichier de règles ; indice ≥ 99 % dans `realisme-fidelite.txt` (avant / après)
- [ ] captures `realisme-*` avant / après regardées ; budget de performance tenu ; tests CPU et parcours complet verts

**WFS-155 — Routes : revêtements réels scannés et calibrés par carte, usure, bordures, sans répétition visible** · E9 · P2 Haute · ⏳ En attente · dépend de : WFS-151 validée (règles : `regles-de-gestion/WFS-155.md` ; charte : `regles-de-gestion/DA-realisme.md`)
- [ ] critères et mesures de fidélité du fichier de règles ; indice ≥ 99 % dans `realisme-fidelite.txt` (avant / après)
- [ ] captures `realisme-*` avant / après regardées ; budget de performance tenu ; tests CPU et parcours complet verts

**WFS-156 — Routes : marquages réglementaires usés et rétroréfléchissants, fissures, nids-de-poule, plaques, traces** · E9 · P2 Haute · ⏳ En attente · dépend de : WFS-155 validée (règles : `regles-de-gestion/WFS-156.md` ; charte : `regles-de-gestion/DA-realisme.md`)
- [ ] critères et mesures de fidélité du fichier de règles ; indice ≥ 99 % dans `realisme-fidelite.txt` (avant / après)
- [ ] captures `realisme-*` avant / après regardées ; budget de performance tenu ; tests CPU et parcours complet verts

**WFS-157 — Routes : états de surface (sec, humide, mouillé, flaques, sable, feuilles), projections d'eau** · E9 · P2 Haute · ⏳ En attente · dépend de : WFS-156, WFS-154 validées (règles : `regles-de-gestion/WFS-157.md` ; charte : `regles-de-gestion/DA-realisme.md`)
- [ ] critères et mesures de fidélité du fichier de règles ; indice ≥ 99 % dans `realisme-fidelite.txt` (avant / après)
- [ ] captures `realisme-*` avant / après regardées ; budget de performance tenu ; tests CPU et parcours complet verts

**WFS-158 — Véhicules : dimensions exactes des 20 voitures (fiches constructeur ±1 %), assise, pièces manquantes, LOD 4 niveaux** · E10 · P2 Haute · 🔧 En cours — Claude (session trafic), 2026-11-06 11:07 UTC (rendu par le PO : le travail avancé du ticket n'existe que dans son bac à sable ; avant, Claude (session 01Us6uCu), 10:35-11:07 UTC, ST-1 poussé, e507961 ; avant encore, Claude (session trafic), 08:24), branche `feature/waforspeed-158` · dépend de : WFS-148 ✅, WFS-144 ✅. Zone : `assets/cars/**`, `tools/convert_car.mjs`, `src/render/vehicule_*`, `references/reel/voitures/**`, `tests/fidelite_voitures*` ; accroches ≤ 10 lignes annoncées : `model_loader.cpp` (LOD), `CMakeLists.txt` (test). Interdit : `src/ui/**` (WFS-168), `src/game/**` (WFS-141), `src/world/**` (WFS-140), `src/platform/**` (WFS-145), le reste de `src/render/**` et `shaders/**` (WFS-142), matériaux (WFS-159), iOS (gel). Fusion : CI verte + captures `garage-*` / `realisme-voiture-*` regardées avant fusion (règles : `regles-de-gestion/WFS-158.md` ; charte : `regles-de-gestion/DA-realisme.md`)
- [ ] critères et mesures de fidélité du fichier de règles ; indice ≥ 99 % dans `realisme-fidelite.txt` (avant / après)
- [ ] captures `realisme-*` avant / après regardées ; budget de performance tenu ; tests CPU et parcours complet verts

**WFS-159 — Véhicules : peinture multicouche (flocons, vernis), verre physique, métaux, pneus, phares à optique** · E10 · P2 Haute · ⏳ En attente · dépend de : WFS-158, WFS-151 validées (règles : `regles-de-gestion/WFS-159.md` ; charte : `regles-de-gestion/DA-realisme.md`)
- [ ] critères et mesures de fidélité du fichier de règles ; indice ≥ 99 % dans `realisme-fidelite.txt` (avant / après)
- [ ] captures `realisme-*` avant / après regardées ; budget de performance tenu ; tests CPU et parcours complet verts

**WFS-160 — Véhicules : suspension visible (roulis, plongée), roues, salissures, gouttes, dégâts légers** · E10 · P2 Haute · ⏳ En attente · dépend de : WFS-159 validée (règles : `regles-de-gestion/WFS-160.md` ; charte : `regles-de-gestion/DA-realisme.md`)
- [ ] critères et mesures de fidélité du fichier de règles ; indice ≥ 99 % dans `realisme-fidelite.txt` (avant / après)
- [ ] captures `realisme-*` avant / après regardées ; budget de performance tenu ; tests CPU et parcours complet verts

**WFS-161 — Environnements : kit d'architecture (6 styles), fenêtres en profondeur, intérieurs la nuit, matériaux scannés** · E12 · P2 Haute · ⏳ En attente · dépend de : WFS-151 validée (règles : `regles-de-gestion/WFS-161.md` ; charte : `regles-de-gestion/DA-realisme.md`)
- [ ] critères et mesures de fidélité du fichier de règles ; indice ≥ 99 % dans `realisme-fidelite.txt` (avant / après)
- [ ] captures `realisme-*` avant / après regardées ; budget de performance tenu ; tests CPU et parcours complet verts

**WFS-162 — Environnements : signalisation réglementaire, feux tricolores, lampadaires réels, glissières, ponts, tunnels** · E12 · P2 Haute · ⏳ En attente · dépend de : WFS-161 validée (règles : `regles-de-gestion/WFS-162.md` ; charte : `regles-de-gestion/DA-realisme.md`)
- [ ] critères et mesures de fidélité du fichier de règles ; indice ≥ 99 % dans `realisme-fidelite.txt` (avant / après)
- [ ] captures `realisme-*` avant / après regardées ; budget de performance tenu ; tests CPU et parcours complet verts

**WFS-163 — Environnements : végétation par espèce, herbe au vent, terrain mélangé, montagnes, mer à vagues physiques** · E12 · P2 Haute · ⏳ En attente · dépend de : WFS-162, WFS-153 validées (règles : `regles-de-gestion/WFS-163.md` ; charte : `regles-de-gestion/DA-realisme.md`)
- [ ] critères et mesures de fidélité du fichier de règles ; indice ≥ 99 % dans `realisme-fidelite.txt` (avant / après)
- [ ] captures `realisme-*` avant / après regardées ; budget de performance tenu ; tests CPU et parcours complet verts

**WFS-164 — Personnages : humains réalistes, 12 morphotypes, vêtements par lieu, peau, cheveux, LOD** · E11 · P2 Haute · ⏳ En attente · dépend de : WFS-151 validée (règles : `regles-de-gestion/WFS-164.md` ; charte : `regles-de-gestion/DA-realisme.md`)
- [ ] critères et mesures de fidélité du fichier de règles ; indice ≥ 99 % dans `realisme-fidelite.txt` (avant / après)
- [ ] captures `realisme-*` avant / après regardées ; budget de performance tenu ; tests CPU et parcours complet verts

**WFS-165 — Personnages : animations en capture de mouvement, mélanges, pieds au sol (IK), regard** · E11 · P2 Haute · ⏳ En attente · dépend de : WFS-164 validée (règles : `regles-de-gestion/WFS-165.md` ; charte : `regles-de-gestion/DA-realisme.md`)
- [ ] critères et mesures de fidélité du fichier de règles ; indice ≥ 99 % dans `realisme-fidelite.txt` (avant / après)
- [ ] captures `realisme-*` avant / après regardées ; budget de performance tenu ; tests CPU et parcours complet verts

**WFS-166 — Personnages : comportements (trajets, feux, groupes, réactions, densité par lieu et heure, spectateurs)** · E11 · P2 Haute · ⏳ En attente · dépend de : WFS-165 validée (règles : `regles-de-gestion/WFS-166.md` ; charte : `regles-de-gestion/DA-realisme.md`)
- [ ] critères et mesures de fidélité du fichier de règles ; indice ≥ 99 % dans `realisme-fidelite.txt` (avant / après)
- [ ] captures `realisme-*` avant / après regardées ; budget de performance tenu ; tests CPU et parcours complet verts

**WFS-167 — Harmonisation et validation finale par carte : rapport complet ≥ 99 %, recette, confirmation de l'utilisateur** · E13 · P2 Haute · ⏳ En attente · dépend de : WFS-148 à WFS-166 validées (règles : `regles-de-gestion/WFS-167.md` ; charte : `regles-de-gestion/DA-realisme.md`)
- [ ] critères et mesures de fidélité du fichier de règles ; indice ≥ 99 % dans `realisme-fidelite.txt` (avant / après)
- [ ] captures `realisme-*` avant / après regardées ; budget de performance tenu ; tests CPU et parcours complet verts

**WFS-168 — [URGENT] Retour en arrière de la taille des boutons (ordre de l'utilisateur)** · E8 (+ E5) · P1 Urgent · ✅ Fusionné (QA de nuit à faire), PR #129 (commit de fusion ce97402), 2026-11-06 — Claude (session trafic), 2026-11-06 08:13 UTC, branche `feature/waforspeed-168` (exception PO : 2e ticket de l'agent, P1 urgent ; WFS-141 continue mais ne touche pas `salon.cpp` / `menu.cpp` avant la fusion de WFS-168) (règles : `regles-de-gestion/WFS-168.md`). Zone : `src/ui/ecrans/{reglages,salon,terrain,chargement,menu}.cpp`, seulement les valeurs de taille des boutons. Interdit : revert entier de WFS-147, textes, garage, HUD, commandes tactiles de course, toute autre suppression de code.
- [ ] boutons, choix et touches à leur taille d'avant WFS-147 (`349f7d0`) sur les captures 1280 et 2400 de ces 5 écrans
- [ ] le reste de WFS-147 identique (textes, ombre et ciel du terrain, étiquette du garage) ; tests CPU et parcours complet verts

**WFS-169 — [URGENT] Correction : CI de `main` rouge depuis WFS-136 (ANO-88)** · E3 · P1 Urgent · ✅ Fusionné (QA de nuit à faire), PR #134, commit eadeae8 — Claude (session trafic), 2026-11-06 10:41 UTC (réattribué par le PO : aucune branche poussée par Claude (session dev expert, 0183bv2X), CI de main rouge), branche `feature/waforspeed-169` (règles : `regles-de-gestion/WFS-169.md`)
- [ ] `benchRecords` aligné sur RG-07 : `best == 0`, `best_v1 == 48210`, `m4` gardée, records vides ; migration `*_v1` inchangée
- [ ] job `banc-essai (vitrines)` vert sur `main`, `vitrine-*` et `trace-*` publiées ; tests CPU verts ; parcours complet 0 écart ; ligne `qa/Check.md`

**WFS-170 — [URGENT] Correction des voitures (ANO-71 à 77)** · E4 · P1 Urgent · 🔍 PR #131 (fusion après WFS-169) — Claude (session trafic), 2026-11-06 09:12 UTC, branche `feature/waforspeed-170` (règles : `regles-de-gestion/WFS-170.md`)
- [ ] `garage-suivant`, `garage-suivant-2400`, `vitrine-p911-0` : 911 dans la couleur choisie ; `vitrine-giulia-0` sans bande grise, vitres teintées ; `vitrine-s3-0` lunette sans camouflage
- [ ] `vitrine-model3-0`, `vitrine-x5-0`, `vitrine-p208-0`, `vitrine-compass-0` sans facettes ni trace beige ; feux arrière rouges sur les 20 `vitrine-<id>-feux` (V60 comprise)
- [ ] 15 voitures déjà bonnes identiques ; aucun `.wfm` modifié ; tests CPU verts ; parcours garage → course 0 écart ; ligne `qa/Check.md`

**WFS-171 — Correction : lueur des feux au sol et feux de recul de la Golf (ANO-70, ANO-78)** · E4 · P3 Moyenne · ⏳ En attente de WFS-170 (règles : `regles-de-gestion/WFS-171.md`)
- [ ] `feux-frein-nuit`, `course-nuit-ville-stop`, `course-nuit-ville-recul` : plus de disque net au sol ; `feux-recul` : recul dans les blocs arrière

**WFS-172 — Correction : captures du banc (ANO-80, ANO-87, ANO-89)** · E3 · P3 Moyenne · 🔧 En cours — Claude (session trafic), 2026-11-06 11:25 UTC, branche `feature/waforspeed-172` · dépend de : WFS-169 ✅ (règles : `regles-de-gestion/WFS-172.md`)
- [ ] `garage-course(-2400)` en course avec la voiture choisie ; `poursuite-camion` camion visible ; `poursuite-police-basse` publiée ; `trace-feux-rouge` ≥ 2 voitures arrêtées

**WFS-173 — Mini-carte du parcours au HUD** · E5 · P2 Haute · ✅ Fusionné (fusion par confiance, QA de nuit à faire), PR #135, commit 56f5d47 — Claude (session 01Us6uCu), 2026-11-06 11:20 UTC, branche `feature/waforspeed-173` (règles : `regles-de-gestion/WFS-173.md`). Zone : nouveaux `src/ui/minicarte.h` / `src/ui/minicarte.cpp` (système `MiniCarte`, ordre 422), `tests/test_minicarte.cpp` ; accroches annoncées au Journal : E3 ≤ 10 lignes au total (`course5.h`, `course5_system.cpp`, `poursuite.h`, `poursuite_system.cpp`, accesseurs en lecture seule), 1 ligne `hud.cpp` (fin de `benchHud`), 1 ligne `CMakeLists.txt` (test), 1 ligne `.github/workflows/build.yml` (copie `minicarte-*-2400`). Interdit : taille ou place de tout bouton (ordre permanent du 2026-11-06), dessin existant de `hud.cpp`, `pause.cpp`, `camera_vue.cpp`, `commandes_tactiles.*`, `direction.*`, place des bandeaux, `src/ui/ecrans/**`, règles de jeu, `src/render/**`, iOS (gel), tout retrait de code.
- [x] `hud-course`, `hud-2400`, `minicarte-checkpoint(-2400)` : carte GPS en haut à droite sous le score, flèche orange, checkpoint cyan devant ; `hud-combo`, `toast-pile`, `encoche-hud(-2400)`, `camera-*-2400` : rien de recouvert
- [x] `course5-course`, `minicarte-ligne(-2400)` : parcours entier, partie parcourue cyan, 4 adversaires, damier d'arrivée ; `duel-course` : adversaire ; `poursuite-police`, `poursuite-hud` : police rouge / bleu, carte à 14 k du haut
- [x] aucun autre élément de l'interface déplacé ni redimensionné (comparaison avec `main`) ; pas de carte en pause, sur les écrans de fin, menus, garage
- [x] ≤ 4 000 sommets et ≤ 128 points au journal du banc ; tests CPU verts (`test_minicarte`) ; bancs `parcours` et `course5` à 0 écart, preuve au Journal ; ligne `qa/Check.md`

**WFS-174 — Sons de la route : vent, roulement, bord de route, hors route, chaussée mouillée** · E6 · P3 Moyenne · 🔧 En cours — Claude, agent 3 (session 011cpqZd), 2026-11-06 12:01 UTC, branche `feature/waforspeed-174` (règles : `regles-de-gestion/WFS-174.md`). Zone : `src/audio/**` (nouveau `route_sons.h` conseillé), `tests/test_audio.cpp` ; accroche ≤ 3 lignes annoncée : `CMakeLists.txt` ; lecture seule : `player.h`, `meteo.h`, `system.h`. Interdit : écrire dans `src/game/**` (WFS-172), `src/ui/**` (WFS-173), `src/ui/ecrans/**`, `src/render/**`, `shaders/**`, `src/world/**` (WFS-140), `src/platform/**` (WFS-145), `.github/**`, `android/**`, iOS (gel).
- [ ] tests CPU : vent nul < 40 km/h et 0,30 au maximum, roulement nul à l'arrêt, bande à `|x| ≥ edge`, herbe à `|x| > 1`, eau seulement sous la pluie, silence hors course et en pause, crête < 1,0
- [ ] banc audio : vent audible au plein gaz, volumes au journal (`SFX route …`), aucune saturation ; parcours complet (accueil → course → pause → fin → menu) au Journal, bancs `parcours` / `course5` à 0 écart ; aucune capture changée

**Action de l'utilisateur (pas un ticket)** : TestFlight et vraie signature iOS / Play Store : compte Apple Developer et secrets de signature à fournir (procédure : `store/signature.md`).

> WFS-01 à WFS-05 (E1) : critères détaillés et rapports dans `qa/rapports/` et dans le Journal (`journal.md`).

## Anomalies et journal (fichiers à part)
- **Anomalies mineures** (QA → PO) : `anno.md`.
- **Journal** du projet (une entrée par étape, nouvelles entrées à la fin) : `journal.md`. **Chaque entrée commence par la date et l’heure UTC complètes, jusqu’à la seconde** : `### AAAA-MM-JJ hh:mm:ss — titre (auteur)`, heure obtenue avec `date -u "+%Y-%m-%d %H:%M:%S"` au moment d’écrire, jamais devinée ni recopiée (une date seule, sans heure, est refusée).
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
