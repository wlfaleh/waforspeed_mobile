# WaForSpeed Mobile — tableau des tâches

** RÈGLE IMPORTANT : il faut voir systématiquement le fichier tache utilisateur.md et répondre en cas de besoin à l'utilisateur

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

10. **Autonomie (règle de l’utilisateur)** : les agents **ne demandent pas de confirmation à l’utilisateur** pour suivre le processus. Ils enchaînent seuls : demander un ticket au PO, créer la branche, lire les règles, coder, construire, fusionner selon la règle en vigueur, mettre à jour `TACHES.md`, `journal.md`, `qa/Check.md`, puis demander le ticket suivant. Pas de « je lance ? », « je continue ? » en fin de message : on agit, puis on rend compte. L’utilisateur n’est sollicité que pour un **vrai blocage** : décision produit que ni les règles de gestion ni le PO ne tranchent, accès ou droits manquants, action irréversible hors processus (supprimer, réécrire l’historique…). Dans ce cas : une seule question précise, avec une recommandation. Une question de règles ou de priorité se pose au PO (`po-scrum-master`), pas à l’utilisateur.

**Définition de « Prêt »** : fichier `regles-de-gestion/WFS-XX.md` écrit par le PO (règles de gestion + éléments du jeu web à réutiliser), dépendances terminées, critères d'acceptation vérifiables sur captures, zone de code connue,
taille ≤ 1 journée (sinon le PO découpe).
**Définition de « Fini »** (sauf fusion par confiance, voir ci-dessus : la CI et la recette viennent après la fusion) : critères cochés, CI verte (banc d'essai, APK, émulateur), verdict QA `VALIDÉ` ou `PASSABLE AVEC ANOMALIES MINEURES` (anomalies mineures restantes inscrites au tableau des anomalies), synchronisé, fusionné,
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
notifications) → fin → Rejouer / Menu**, en plein écran Android. **Décision de l'utilisateur (2026-11-03) : le MVP n'attend plus le simulateur ; l'utilisateur teste lui-même l'APK de `main` sur téléphone** (WFS-50, WFS-58 et WFS-64 sans urgence). Tickets marqués **[MVP]** (le tableau est trié par numéro de ticket ; la colonne « Priorité » donne l'urgence), dans l'ordre :
WFS-35 → WFS-27 → WFS-37 → WFS-29 → WFS-38 → WFS-47 → WFS-57 (centrage fin, ANO-14) → WFS-56 (scénario simulateur) → WFS-58 + WFS-59 (corrections simulateur) → WFS-50 (vérification simulateur ; WFS-64 suspendu : le gel ne s'est pas reproduit, run 41719456821). Hors MVP : garage, terrain, pause,
duel, sauvegarde, compteur en arc, italique, nuit, météo, bruitages.

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
| P4 Basse | WFS-78 | iOS : affichage direct dans une surface Metal et démarrage jusqu'au menu | E7 (+ accroche E4 `vk.cpp`) | ✅ Fusionné (QA de nuit à faire), PR #64 — surface Metal OK, démarrage bloqué dans `Renderer::init` sur le simulateur | Claude (session trafic, agent WFS-78), 2026-11-04 | WFS-22 ✅ | `feature/waforspeed-78` |  |  |
| P4 Basse | WFS-79 | Son iOS (sortie Core Audio) | E6 (+ accroche E7 `ios.cmake`) | ✅ Fusionné (QA de nuit à faire) — PR #63 | Claude (session trafic, agent WFS-79), 2026-11-04 | WFS-22 ✅, WFS-76 ✅ | `feature/waforspeed-79` |  |  |
| P3 Moyenne | WFS-80 | Manette (peintures, réglages au focus) + réglages « Direction » et « Réduire les mouvements » | E8 (+ accroche E5 `garage.cpp`) | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic), 2026-11-04 | WFS-22 ✅, WFS-75 ✅ | `feature/waforspeed-80` | #74 |  |
| P3 Moyenne | WFS-81 | Nuit corniche et circuit : vrais lampadaires, lumières alignées, feux du trafic sans double halo | E2 (+ accroche E3 `traffic_system.cpp`) | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic, agent WFS-81), 2026-11-04 | WFS-09 ✅, WFS-53 ✅, WFS-67 ✅ | `feature/waforspeed-81` | #75 |  |
| P2 Haute | WFS-82 | [STABLE] Performances téléphone : masquage hors champ et au loin, moins d'appels de dessin (Basse ≤ 300) | E4 | ✅ Fusionné (fusion par confiance, QA de nuit à faire) — Basse 331 > 300 : cible assouplie par le PO (Moyenne 337 ≤ 450 atteinte) | Claude (agent 4, session 011cpqZan), 2026-11-04 | WFS-15 ✅, WFS-67 ✅ | `feature/waforspeed-82` | #67 |  |
| P2 Haute | WFS-83 | Duel jouable entre deux téléphones sur le même Wi-Fi : écran Salon, transport réseau local, menu « Duel en ligne » actif | E3 (+ `src/ui/ecrans/salon.*`, accroches E0 / E7 / E8) | ✅ Fusionné (par confiance, QA de nuit à faire) | Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04 | WFS-21 ✅, WFS-24 ✅, WFS-41 ✅, WFS-74 ✅ | `feature/waforspeed-83` | [#68](https://github.com/wlfaleh/waforspeed_mobile/pull/68) |  |
| P1 Urgent | WFS-84 | [URGENT] Nuit : feux des voitures naturels (petits feux nets, lueur douce, fini l'effet artificiel) | E4 | ✅ Fusionné (par confiance, QA de nuit à faire) | Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04 | WFS-09 ✅ | `feature/waforspeed-84` | [#70](https://github.com/wlfaleh/waforspeed_mobile/pull/70) |  |
| P1 Urgent | WFS-85 | [URGENT] Caméra de poursuite plus haute (voir les voitures devant) | E3 | ✅ Fusionné (QA de nuit à faire) | Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04 | WFS-66 ✅ | `feature/waforspeed-85` | [#71](https://github.com/wlfaleh/waforspeed_mobile/pull/71) |  |
| P1 Urgent | WFS-86 | [URGENT] Voitures en course aussi belles qu'au garage (qualité, échelle de rendu, anticrénelage, modèle complet) | E4 | ✅ Fusionné (fusion par confiance, QA de nuit à faire) | Claude (agent 4, session 011cpqZan), 2026-11-04 | — | `feature/waforspeed-86` | #72 |  |
| P2 Haute | WFS-90 | Conduite libre : demi-tour, marche arrière facile, rouler dans l'autre sens | E3 (exception : 2e ticket E3, fichiers séparés de WFS-89) | ✅ Fusionné (fusion par confiance, QA de nuit à faire) | Claude (agent 4, session 011cpqZan), 2026-11-04 | — | `feature/waforspeed-90` | #78 |  |
| P1 Urgent | WFS-87 | [URGENT] CI : l'APK de `main` toujours publié (Release `apercu` fiable, plus de trou ni d'échec 403) | E7 (exception : 2e ticket E7, fichiers séparés de WFS-78) | ✅ Fusionné par l'utilisateur (PR #73, QA de nuit à faire) | Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04 | WFS-69 ✅ | `feature/waforspeed-87` | [#73](https://github.com/wlfaleh/waforspeed_mobile/pull/73) |  |
| P4 Basse | WFS-88 | iOS : `Renderer::init` ne se bloque plus, démarrage jusqu'au menu | E4 | ⏸ Suspendu (GEL iOS, décision utilisateur 2026-11-04 : Android d'abord) ; PR #76 laissée ouverte (décision de l'utilisateur, 2026-11-04 22:29 UTC) | Claude (session trafic), 2026-11-04 | WFS-78 ✅ | `feature/waforspeed-88` |  |  |
| P2 Haute | WFS-89 | Duel par Internet : salon par code à 4 caractères, compatible avec le jeu web (PeerJS) | E3 (+ accroches E8 `salon.*`, E0 `CMakeLists.txt`, E7 manifeste) | ✅ Fusionné (PR #77, revue WFS-103, décision B : duel Wi-Fi local, saisie du code, « Sans Internet » ; vrai réseau PeerJS → WFS-92 ⏸ ; QA de nuit à faire) | Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04 | WFS-83 ✅, WFS-21 ✅ | `feature/waforspeed-89` |  |  |
| P2 Haute | WFS-91 | [STABLE] Endurance : parcours automatique long sans plantage ni fuite, validation Vulkan à zéro erreur, retours d'arrière-plan | E7 | ⏳ Prêt (libéré le 2026-11-04 : aucune branche poussée, ordre de l'utilisateur) |  | — | `feature/waforspeed-91` |  |  |
| P2 Haute | WFS-92 | Duel par Internet : pair PeerJS réel (WebSocket + WebRTC, libdatachannel + Mbed TLS dans `third_party/`) | E3 | ⏸ En attente (décision de l'utilisateur : ajout des bibliothèques tierces) |  | WFS-89 ✅ | `feature/waforspeed-92` |  |  |
| P2 Haute | WFS-93 | Ville : les 6 quartiers du jeu web (résidentiel, parc, port) + panneaux du circuit avec texte, vignes sans marches | E2 | ⏳ Prêt (libéré le 2026-11-04 : aucune branche poussée, ordre de l'utilisateur) |  | — | `feature/waforspeed-93` |  |  |
| P3 Moyenne | WFS-94 | Corniche : quartiers Port de plaisance, Côte rocheuse, Village de pêcheurs (villas, bateaux, rochers) | E2 | ⏳ En attente (après WFS-93, même épic) |  | WFS-93 | `feature/waforspeed-94` |  |  |
| P2 Haute | WFS-95 | Texte et bandeaux : caractères manquants (« — »), petits libellés lisibles, nom du quartier en course | E5 | ⏳ Prêt (libéré le 2026-11-04 : aucune branche poussée, ordre de l'utilisateur) |  | — | `feature/waforspeed-95` |  |  |
| P2 Haute | WFS-96 | Menus : Showroom et « Ta voiture » ouvrent le garage, quartiers sur la carte du terrain, focus manette, voile sans bandes, sons d'interface | E8 | ✅ Fusionné (PR #79, revue WFS-103 ; ANO-15 et ANO-34 corrigées ; QA de nuit à faire) | Claude (session trafic), 2026-11-04 | — | `feature/waforspeed-96` |  |  |
| P3 Moyenne | WFS-97 | Nuit du circuit : taches de lumière visibles sous les projecteurs | E4 | ⏳ Prêt (libéré le 2026-11-04 : aucune branche poussée, ordre de l'utilisateur) |  | — | `feature/waforspeed-97` |  |  |
| P1 Urgent | WFS-98 | Correction : commandes tactiles du jeu web (GAZ / FREIN / NITRO / volant), HUD de la vue cockpit, caméra qui suit le demi-tour (ANO-31, ANO-24, ANO-30) | E3 (exception : 2e ticket E3, fichiers séparés de WFS-89 ; + E0 `game.cpp`, E5 `hud.*`) | ✅ Fusionné (fusion par confiance, QA de nuit à faire) | Copilot (session 0e5c192d), 2026-11-04 | — | `feature/waforspeed-98` | #80 |  |
| P1 Urgent | WFS-99 | Correction : garage lisible (voile, néons à droite, taches de lumière au sol, voiture de 3/4 avant) (ANO-22, ANO-23, ANO-33) | E5 (exception : 2e ticket E5, fichiers séparés de WFS-95) | ✅ Fusionné (fusion par confiance, QA de nuit à faire) | Copilot (session 0e5c192d), 2026-11-04 | — | `feature/waforspeed-99` | #81 |  |
| P1 Urgent | WFS-100 | Correction : carrosseries sans reflets flottants ni taches, bloom sans images fantômes (ANO-21, ANO-28, ANO-27) | E4 (exception : 2e ticket E4, fichiers séparés de WFS-97) | 🔧 En cours | Claude (session trafic), 2026-11-04 | — | `feature/waforspeed-100` |  |  |
| P1 Urgent | WFS-101 | Correction : étincelles et lignes de nitro visibles, trafic du banc de près, écran de fin aéré (ANO-25, ANO-26, ANO-36) | E4 (exception : 3e ticket E4, fichiers séparés ; + E3 `traffic_system.cpp`, `course_system.*`) | 🔧 En cours | Copilot (session dc0cdd3f), 2026-11-04 | — | `feature/waforspeed-101` |  |  |
| P1 Urgent | WFS-102 | Correction : interface à l'échelle du téléphone (dp, cibles 44 dp), biseaux d'un seul tenant, écrans lisibles (ANO-19, ANO-18, ANO-20, ANO-32, ANO-35, ANO-38, ANO-39, ANO-41, ANO-42, ANO-43, ANO-44, ANO-45) | E8 (+ E5, accroches E0 / E7) | ⏳ En attente (WFS-95, WFS-96, WFS-91, WFS-89, WFS-98, WFS-99, WFS-101 : mêmes fichiers) |  | WFS-95, WFS-96, WFS-91, WFS-89, WFS-98, WFS-99, WFS-101 | `feature/waforspeed-102` |  |  |
| P1 Urgent | WFS-103 | Revue et fusion des PR en attente (#77 WFS-89, #79 WFS-96 ; #76 iOS gelée) | E7 (transverse : n'occupe pas l'épic, WFS-91 reste attribuable) | 🔍 #77 et #79 fusionnées, CI de main à confirmer ; #76 gelée (iOS) | Claude (session trafic), 2026-11-04 | — | `feature/waforspeed-103` (travail sur les branches des PR) | #77, #79 |  |
| P1 Urgent | WFS-104 | [PRIORITÉ ABSOLUE] Voiture en course : qualité identique au garage (2e passage, résolution 100 %, jamais sous le garage) | E4 (exception : 3e ticket E4, fichiers séparés de WFS-100 / WFS-101 : `qualite_*`, `masquage.*`) | ✅ Fusionné (fusion par confiance, QA de nuit à faire) | Claude (agent 3, session 011cpqZd), 2026-11-04 | WFS-86 ✅ | `feature/waforspeed-104` |  |  |
| P1 Urgent | WFS-105 | [PRIORITAIRE] Caméra de poursuite : angle d'environ 40° (voir les voitures devant) + vue Proche qui montre la route (RG-06) | E3 | 🔧 En cours | Copilot (session 1ef8d4d6), 2026-11-04 | WFS-85 ✅ | `feature/waforspeed-105` |  |  |
| P1 Urgent | WFS-106 | [STABLE] CI : publier l'APK de main (Release apercu) à chaque fusion | E7 | ⏳ Prêt |  | — | `feature/waforspeed-106` |  |  |
| P1 Urgent | WFS-107 | [URGENT] Bug : animation bizarre (« feu blanc ») quand on tourne à gauche ou à droite | E4 | ⏳ Prêt |  | — | `feature/waforspeed-107` |  |  |
| P1 Urgent | WFS-108 | [URGENT] Commandes tactiles : boutons plus grands, frein et marche arrière revus, option gyroscope | E3 | ⏳ Prêt |  | WFS-98 ✅ | `feature/waforspeed-108` |  |  |
| P1 Urgent | WFS-109 | [URGENT] Feux de freinage réalistes (2e passage) | E4 | ⏳ En attente (après WFS-107, même épic) |  | WFS-84 ✅ | `feature/waforspeed-109` |  |  |

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

**WFS-104 — [PRIORITÉ ABSOLUE] Voiture en course = qualité du garage** · E4 · priorité P1 Urgent · ⏳ Prêt (règles : `regles-de-gestion/WFS-104.md`)
- [ ] course = garage en netteté et reflets, confirmé sur téléphone

**WFS-105 — [PRIORITAIRE] Caméra de poursuite ≈ 40° + vue Proche** · E3 · priorité P1 Urgent · 🔧 En cours — Copilot (session 1ef8d4d6), 2026-11-04, branche `feature/waforspeed-105` (règles : `regles-de-gestion/WFS-105.md`). Zone : `src/game/camera_vue.*`, `src/game/camera_banc.*`, assertions caméra de `tests/test_systems.cpp` ; interdit : `src/render/**`, `shaders/**`, `src/game/traffic_system.cpp`, `course_system.*` (WFS-101), `commandes_tactiles.*`, `direction.*`, `player.*` (WFS-108), `src/game/game.*`, `src/ui/**`, `src/platform/**`, `.github/**`, iOS
- [ ] au moins 4 voitures devant visibles ; angle documenté
- [ ] `camera-proche` : avant de la voiture et rue devant (≥ 15 m) visibles, caméra inclinée ≈ 20-25° ; Capot et Cockpit inchangés

**WFS-106 — [STABLE] CI : publier l'APK de main** · E7 · priorité P1 Urgent · ⏳ Prêt (règles : `regles-de-gestion/WFS-106.md`)
- [ ] Release apercu = APK du dernier commit de main

**WFS-107 — [URGENT] Bug animation « feu blanc » au braquage** · E4 · priorité P1 Urgent · ⏳ Prêt (règles : `regles-de-gestion/WFS-107.md`)
- [ ] voir les critères du fichier de règles

**WFS-108 — [URGENT] Commandes tactiles plus grandes, frein / marche arrière, gyroscope** · E3 · priorité P1 Urgent · ⏳ Prêt (règles : `regles-de-gestion/WFS-108.md`)
- [ ] voir les critères du fichier de règles

**WFS-109 — [URGENT] Feux de freinage réalistes** · E4 · priorité P1 Urgent · ⏳ Prêt (règles : `regles-de-gestion/WFS-109.md`)
- [ ] voir les critères du fichier de règles

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

**WFS-88 — iOS : `Renderer::init` ne se bloque plus** · E4 · P4 Basse · 🔧 En cours — Claude (session trafic), 2026-11-04, branche `feature/waforspeed-88` (règles : `regles-de-gestion/WFS-88.md`). Zone : `src/render/**` (+ accroche ≤ 10 lignes `ios/**` / `ios.yml` annoncée)
- [ ] `ios.yml` : « QA pret » atteint, `ios-menu.png` montre le menu
- [ ] plus de trou de 60 s après l'étape vulkan ; CI Android verte, banc PC inchangé

**WFS-89 — Duel par Internet (code à 4 caractères, compatible jeu web)** · E3 · P2 Haute · ✅ Fusionné (PR #77, fb50d09, revue WFS-103 ; WebRTC réel → WFS-92) — Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04, branche `feature/waforspeed-89` (règles : `regles-de-gestion/WFS-89.md`). Zone : `src/game/duel*`, `src/game/reseau_peerjs.*`, `src/game/binarypack.*`, `tests/**` ; accroches annoncées : `src/ui/ecrans/salon.*`, `CMakeLists.txt` ≤ 10 lignes, manifeste Android ≤ 2 lignes
- [ ] `salon-code`, `salon-code-saisi` : bloc « Rejoindre avec un code », 4 cases, clavier à l'écran, « Rejoindre » actif à 4 caractères
- [ ] `salon-internet` (relais simulé : 2 pilotes) et `salon-hors-ligne` (note orange, pas de plantage)
- [ ] `test_binarypack` vert (octets de référence PeerJS) ; APK : duel entre deux réseaux différents et avec le jeu web (vérifié par l'utilisateur)

**WFS-91 — [STABLE] Endurance : parcours long, validation Vulkan, retours d'arrière-plan** · E7 · P2 Haute · ⏳ Prêt — remis en ⏳ Prêt le 2026-11-04 (aucune branche poussée depuis l'attribution à Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), ordre de l'utilisateur ; « à démarrer dès WFS-89 fusionné » ne valait que pour l'agent 5, occupé par WFS-89 : aucune dépendance, zone disjointe de WFS-89), branche `feature/waforspeed-91` (règles : `regles-de-gestion/WFS-91.md`). Zone : `src/platform/**` (nouveau `endurance_banc.*`, `desktop_main.cpp`), `.github/workflows/build.yml` (job `endurance`), `tools/qa/**` ; corrections ailleurs : accroches ≤ 10 lignes annoncées, jamais dans les zones de tickets en cours
- [ ] job `endurance` vert sur `main` (≤ 15 min), artefact `captures-endurance`
- [ ] `endurance-journal.txt` : ≥ 12 cycles, 4 cartes, jour et nuit, ≥ 4 retours d'arrière-plan, RSS final ≤ cycle 2 + 10 %, 0 erreur de validation (hors erreurs connues avec ticket)
- [ ] `endurance-retour-*.png` : image correcte après chaque retour ; une capture de fin de course par carte ; autres jobs verts, APK publié
- [ ] (ajout PO, ANO-29) Release `apercu` jamais incomplète : publication `cancel-in-progress: false`, anciens fichiers supprimés après l'envoi complet des nouveaux (RG-A1, RG-A2)

**WFS-92 — Duel par Internet : pair PeerJS réel** · E3 · P2 Haute · ⏸ En attente de la décision de l'utilisateur sur l'ajout de libdatachannel 0.22.6 + Mbed TLS 3.6.2 dans `third_party/` (≈ 14 Mo, recommandation PO : autoriser) — branche `feature/waforspeed-92` (règles : `regles-de-gestion/WFS-92.md`). Zone : `src/game/reseau_peerjs.*`, `third_party/**` (nouvelles bibliothèques) ; accroches : `CMakeLists.txt`, `android/**` ≤ 10 lignes
- [ ] APK : duel entre deux réseaux différents et contre le jeu web (vérifié par l'utilisateur)
- [ ] sans Internet : note orange, aucun plantage ; CI verte, APK publié

**WFS-93 — Ville : 6 quartiers + finitions des cartes** · E2 · P2 Haute · ⏳ Prêt — remis en ⏳ Prêt le 2026-11-04 (aucune branche poussée depuis l'attribution à Claude (session trafic), ordre de l'utilisateur), branche `feature/waforspeed-93` (règles : `regles-de-gestion/WFS-93.md`). Zone : `src/world/**` (nouveau `ville_quartiers.cpp`, `ville.cpp`, `circuit.cpp`, `campagne.cpp`, `cartes_banc.cpp`) ; interdit : `src/render/**`, `src/platform/**`, `.github/**`, fichiers du duel
- [ ] `carte-ville-residentiel`, `-parc`, `-port` : objets du quartier (abribus ; grille, bancs, fontaine ; entrepôts, conteneurs, grues) ; `course-*` inchangées
- [ ] `carte-circuit-jour` / `-nuit` : texte d'AD_TEXTS lisible sur les panneaux ; `carte-campagne-vignes` : rangs sans marches ; ≤ 300 appels de dessin

**WFS-94 — Corniche : quartiers marina, rochers, village** · E2 · P3 Moyenne · ⏳ En attente (après WFS-93) — branche `feature/waforspeed-94` (règles : `regles-de-gestion/WFS-94.md`). Zone : `src/world/plage.cpp`, `cartes_banc.cpp`, nouveaux `src/world/plage_*.cpp`
- [ ] `carte-plage-marina`, `-rochers`, `-village` : bateaux, bornes, villas, rochers, glissières visibles ; ≤ 300 appels de dessin
- [ ] (ajout PO, ANO-40) `vie-portique` à 25 m (« CHECKPOINT 1 » lisible), `vie-plage` à 15 m (serviettes et baigneurs visibles)

**WFS-95 — Texte et bandeaux** · E5 · P2 Haute · ⏳ Prêt — remis en ⏳ Prêt le 2026-11-04 (aucune branche poussée depuis l'attribution à Claude (session trafic), ordre de l'utilisateur), branche `feature/waforspeed-95` (règles : `regles-de-gestion/WFS-95.md`). Zone : `src/ui/text.*`, `src/ui/toasts.*`, `shaders/ui*`, `assets/fonts/**` ; interdit : `src/ui/garage.*`, `src/ui/ecrans/**`, `src/render/**`, `src/platform/**`
- [ ] menu : « — » sous RECORD (plus de « ? »), « TA VOITURE » lisible ; capture `toast-quartier` : bandeau du nom du quartier à chaque changement de zone

**WFS-96 — Menus : Showroom, quartiers du terrain, focus manette, voile, sons** · E8 · P2 Haute · ✅ Fusionné (PR #79, 6863763, revue WFS-103) — Claude (session trafic), 2026-11-04, branche `feature/waforspeed-96` (règles : `regles-de-gestion/WFS-96.md`). Zone : `src/ui/ecrans/**` sauf `salon.*` ; accroches ≤ 10 lignes annoncées : `src/ui/garage.cpp`, `src/platform/manette_jeu.cpp`, `src/audio/sfx.*` ; interdit : `desktop_main.cpp`, `.github/**`, `src/render/**`, `src/ui/text.*`, `toasts.*`
- [ ] `menu-showroom`, `showroom-choisir` : Showroom ouvre le garage, « Choisir » ramène au menu ; `terrain-0` : « Quartiers » et noms ; voile du menu sans rayures ; focus manette sur le 1er choix ; « SFX garage » / « SFX carte » au journal

**WFS-97 — Nuit du circuit : taches de lumière des projecteurs** · E4 · P3 Moyenne · ⏳ Prêt — remis en ⏳ Prêt le 2026-11-04 (aucune branche poussée depuis l'attribution à Claude (session trafic), ordre de l'utilisateur), branche `feature/waforspeed-97` (règles : `regles-de-gestion/WFS-97.md`). Zone : `src/render/nuit.cpp` ; interdit : `src/world/**`, `src/platform/**`, iOS
- [ ] `carte-circuit-nuit`, `course-nuit-circuit-0` : taches froides visibles sous les mâts ; ville et plage de nuit sans surexposition

**WFS-98 — Correction : commandes tactiles, HUD cockpit, caméra du demi-tour** · E3 (+ E0 `game.cpp`, E5 `hud.*`) · P1 Urgent · ✅ Fusionné le 2026-11-04 par confiance (PR #80, QA de nuit : `qa/Check.md`) — Copilot (session 0e5c192d), 2026-11-04 (réattribué : aucune branche poussée depuis l'attribution à Claude (session trafic) à 18:42:05, ordre de l'utilisateur), branche `feature/waforspeed-98` (règles : `regles-de-gestion/WFS-98.md`). Zone : `src/game/game.cpp` (commandes, caméra de poursuite), `direction.*`, `camera_vue.*`, `camera_banc.*` (si besoin), `conduite_libre_banc.*`, nouveau `commandes_tactiles.*`, `src/ui/hud.*`
- [ ] `hud-course`, `encoche-hud-2400` : pédales « GAZ » orange et « FREIN » rouge, nitro rond à anneau vert acide « NITRO », volant dessiné ; boutons du HUD sans rayures ; `hud-nitro` : gaz appuyé (remplissage orange) ; `course-gaucher` : commandes en miroir, gaz appuyé (RG-05 bis)
- [ ] `camera-cockpit` : un seul compteur, plus de disque cyan ; `conduite-demitour` : Golf entière au tiers bas, en travers ; tests CPU verts

**WFS-99 — Correction : garage lisible** · E5 · P1 Urgent · ✅ Fusionné le 2026-11-04 par confiance (PR #81, QA de nuit : `qa/Check.md`) — Copilot (session 0e5c192d), 2026-11-04 (après remise en ⏳ Prêt le 2026-11-04 : aucune branche poussée depuis l'attribution à Claude (session trafic), ordre de l'utilisateur), branche `feature/waforspeed-99` (règles : `regles-de-gestion/WFS-99.md`). Zone : `src/ui/garage.*`, `planche_voitures.*`, `src/render/bench_spots.*`
- [ ] `spots-sol`, `garage-0`, `garage-2` : 3 taches de lumière au sol, 911 noire détachée du fond (liseré cyan)
- [ ] `garage-*` : aucun néon sur le texte, voile à gauche, voiture de 3/4 avant, panneau sans escalier

**WFS-100 — Correction : carrosseries propres et bloom sans fantômes** · E4 · P1 Urgent · 🔧 En cours — Claude (session trafic), 2026-11-04 (réattribué après remise en ⏳ Prêt le 2026-11-04), branche `feature/waforspeed-100` (règles : `regles-de-gestion/WFS-100.md`). Zone : `src/render/renderer.cpp`, `vk.*`, `model_loader.*`, `qualite_*`, `shaders/scene*`, `post*`, `common.glsl`
- [ ] `vitrine-*`, `garage-peinture*`, `trafic-clignotant` : aucun trait brillant dans le vide ; Giulia, X5, Model 3 en peinture unie
- [ ] `qualite-haute`, `qualite-moyenne`, `course-nuit-*` : halo rond et doux, aucune copie fantôme ni traînée

**WFS-101 — Correction : étincelles, nitro, trafic du banc, écran de fin** · E4 (+ E3) · P1 Urgent · 🔧 En cours — Copilot (session dc0cdd3f), 2026-11-04 (après remise en ⏳ Prêt le 2026-11-04 : aucune branche poussée depuis l'attribution à Claude (session trafic), ordre de l'utilisateur), branche `feature/waforspeed-101` (règles : `regles-de-gestion/WFS-101.md`). Zone : `src/render/effets_*`, `bench_effets.*`, `src/game/traffic_system.cpp` (`benchTraficVivant`), `src/game/course_system.*`
- [ ] `choc-etincelles` : gerbe orange et blanche ; `nitro-lignes` : traits en étoile + vignette cyan sombre
- [ ] `trafic-clignotant`, `trafic-freinage` : voiture à 8-25 m ; `fin-record` : badge et record espacés

**WFS-102 — Correction : interface à l'échelle du téléphone, biseaux, écrans lisibles** · E8 (+ E5, accroches E0 / E7) · P1 Urgent · ⏳ En attente (WFS-95, WFS-96, WFS-91, WFS-89, WFS-98, WFS-99, WFS-101) — branche `feature/waforspeed-102` (règles : `regles-de-gestion/WFS-102.md`). Zone : `src/ui/ecrans/**`, nouveau `src/ui/echelle.*`, une ligne `ui::echelle` par fichier d'interface, `commandes_tactiles.*` (taille des zones), accroches `game.h` / `game.cpp` (`drawControls`) / `android_main.cpp`. Ajouts du 2026-11-04 (QA d210f62) : ANO-42 (bandes de l'illustration du terrain), ANO-43 (touche effacer), ANO-44 (aide sous le code), ANO-45 (commandes cachées hors course), cibles < 44 dp (clavier, GAZ, NITRO)
- [ ] `*-2400` : boutons ≥ 44 dp, libellés ≥ 12 dp ; 1280x720 inchangé ; aucun bouton rayé sur aucun écran
- [ ] `terrain-*` : description sur fond sombre, sélection cyan ; `menu-0` aide juste ; `salon-*` carte centrale ; `chargement-erreur` « Réessayer »
- [ ] clavier du code, GAZ, NITRO ≥ 44 dp ; touche effacer lisible ; aide sous le code ; illustration du terrain sans bandes ; aucune commande tactile pendant le fondu ni à la fin de course

**WFS-103 — Revue et fusion des PR en attente** · E7 (transverse) · P1 Urgent · 🔍 #77 (fb50d09) et #79 (6863763) fusionnées, CI de main à confirmer ; #76 gelée — Claude (session trafic), 2026-11-04 (règles : `regles-de-gestion/WFS-103.md`). Zone : fichiers des PR revues (résolution de conflits seulement), `TACHES.md`, `qa/Check.md`, `journal.md`, `anno.md` ; aucune nouvelle fonctionnalité, rien d'iOS (#76 gelée). Ordre : #77 (WFS-89) puis #79 (WFS-96, non réattribué)
- [ ] #77 et #79 fusionnées, ou blocage précis écrit au Journal (PR laissée ouverte)
- [ ] CI de `main` verte après la dernière fusion ; `TACHES.md`, `qa/Check.md`, Journal à jour pour WFS-89 et WFS-96
- [ ] **Règle permanente** : aucune PR ouverte plus de 2 h sans décision (fusionnée, bloquée au Journal, ou gel iOS)

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
