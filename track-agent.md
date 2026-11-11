# Suivi des agents — qui fait quoi

> **Règle de l'utilisateur (2026-11-06)** : ce fichier est **toujours synchronisé** avec `main`.
> - **Chaque agent développeur** met à jour **sa propre ligne** à chaque étape (ticket attribué, branche créée et poussée, PR ouverte,
>   fusion, CI de `main` verte, ticket fini, agent libre) et l'envoie sur `main` **dans le même commit** que `TACHES.md`.
> - **Le PO** remet tout le fichier à jour à chaque appel (attribution, point d'avancement) d'après `TACHES.md` et l'heure du dernier
>   envoi de chaque branche (`git log -1 --format=%ci origin/feature/waforspeed-XX`), et met à jour « Dernière mise à jour : 2026-11-10 09:49:00 UTC (po-scrum-master)
> - Mêmes règles que `TACHES.md` : partir de `main` (`git pull`), ne modifier que sa ligne, ne jamais copier ce fichier d'une autre branche.
> - **Colonne « Ticket en cours »** (règle de l'utilisateur, 2026-11-06) : seulement les tickets 🔧 En cours ou 🔀 En PR ; **vide** si l'agent n'en a
>   aucun (les tickets 🟣 fusionnés ou ✅ terminés n'y figurent pas ; la CI de `main` suivie s'écrit dans « État »).
> - **Nom de l'agent** : la colonne « Agent » donne le nom à mettre à la fin de chaque titre de commit, entre parenthèses (règle de l'utilisateur, 2026-11-06).
> - **Ce fichier suit les agents, pas les tickets** (règle de l'utilisateur, 2026-11-06) : une ligne par agent (ce qu'il fait, sa branche,
>   son dernier envoi, son état). La liste des tickets et leurs statuts restent dans `TACHES.md`.
> - **Après une fusion, l'agent est libre** (règle de l'utilisateur, 2026-11-06 23:20 UTC) : il ne « suit » plus la CI de `main` ; la colonne État ne le dit plus.
> - **Un seul sujet par session (règle de l'utilisateur, 2026-11-09 12:05 UTC)** : chaque session de développeur mène **un seul ticket** jusqu'à la fusion, puis **s'arrête** (État « ⏹️ session terminée ») ; elle ne reprend pas de ticket. Les sessions qui tiennent un ticket à 12:05 UTC le terminent, puis s'arrêtent. Nouveau sujet = nouvelle session, nouvelle ligne.
> - **Agent fini = ligne enlevée** (règle de l'utilisateur, 2026-11-09) : dès qu'un développeur a fusionné son ticket (session terminée), le PO **enlève sa ligne** de ce tableau ; l'historique reste dans `TACHES.md` et `journal.md`.
> - Alerte ⚠️ quand un ticket n'a rien reçu depuis plus de 2 h, ou n'a pas de branche 30 min après l'attribution ; 🔴 pour un ticket P1.

**Dernière mise à jour** : 2026-11-11 12:17:38 UTC (po-scrum-master)

> 🔴 **Alerte** : CI de `main` verte (#998, WFS-419) ; **WFS-324** (P1) en retour lead dev (PR #341) ; WFS-421 attribué à Claude, agent dev (01UY1Vv4ss) ; WFS-412 attribué à agent expert ci cd (session 01UY1Vv4jR6p) ; P1 prêts sans session : **WFS-423, 422** (performance de la Basse, `@dev`) ; WFS-424 attend 421 à 423 ; WFS-425 (P2) prêt ; WFS-405, 319, 407, 408, 409 en attente du jour de publication.

## Agents développeurs

| Agent (session) | Ticket en cours | Ce qu'il fait | Branche, dernier envoi (UTC) | État |
|---|---|---|---|---|
| Claude, agent dev3d (01UY1Vv5YT) | WFS-324 | rétroviseurs intérieur et extérieurs avec vrai reflet (vue Cockpit) | `feature/waforspeed-324`, 2026-11-10 23:10 | 🔁 PR #341 corrigée, lead dev rappelé |
| Claude, agent dev3d (01Us6uCxLS) |  | WFS-414 fusionné (#343, 5b069f79), 🟡 à valider par l'utilisateur | `feature/waforspeed-414`, dernier envoi 2026-11-11 02:07 UTC | ⏹️ session terminée (WFS-414 fusionné) |
| Claude, agent dev3d (01UY1Vv4rv) |  | bâtiments de la Ville cuits dans le paquet de carte (WFS-416, fusionné, PR #344) | `feature/waforspeed-416`, fusionnée en squash (55ee0909) | ⏹️ session terminée (WFS-416 fusionné, 🟡 à valider par l'utilisateur) |
| lead-dev (session 01UY1Vv5W2) | WFS-419 | interrupteurs de diagnostic de performance (ordre de l'utilisateur : codé et fusionné par le lead dev) | `feature/waforspeed-419`, 2026-11-11 | ⏹️ session terminée (WFS-419 fusionné #345, CI de `main` #998 verte) |
| Claude, agent dev (01UY1Vv4ss) | WFS-421 | halos au sol des lampadaires coupés en Basse (tête gardée) | `feature/waforspeed-421`, pas encore poussée | 🔧 en cours (attribué 12:10) |
| agent expert ci cd (session 01UY1Vv4jR6p) | WFS-412 | `tests_cpu.sh` lance tous les tests et liste les échecs à la fin ; licences vérifiées même en cas d'échec (US technique) | `feature/waforspeed-412`, pas encore poussée | 🔧 en cours (attribué 12:17) |
| lead-dev (session 01UY1VvJJ) | WFS-415 | fluidité d'hier (fenêtres WFS-402 coupées par interrupteur) et CI de `main` au vert | `feature/waforspeed-415`, 2026-11-10 22:33 | 🔧 en cours |
| Claude, agent dev (01Us6uCtwf) |  | session `@dev` ouverte (12:18 UTC), demande un ticket au PO | — | ⏳ attend son ticket |
| Claude, agent cartes (01UY1Vv2HrQF) |  | session `@cartes` ouverte (12:20 UTC), demande un ticket au PO | — | ⏳ attend son ticket |
| Claude, agent dev (01Us6uCt95) |  | session `@dev` ouverte (12:21 UTC), demande un ticket au PO | — | ⏳ attend son ticket |

## Agents de pilotage

| Agent | Rôle | Dernière action |
|---|---|---|
| dev | développeur général (`.claude/agents/dev.md`, appelé par `@dev`) : un ticket du PO, jusqu'à la fusion, puis arrêt | créé le 2026-11-09 (demande de l'utilisateur) |
| expert-conduite | développeur expert en physique de la voiture (skill `expert-conduite`) : chantier « Conduite réaliste » | créé le 2026-11-07 |
| lead-dev | relit chaque PR : ouvre les captures avant / après (≥ 3, `qa/pr/WFS-XX/`), compare à la demande de l'utilisateur, **seul à fusionner** (squash) ou refuse ; appelé par le développeur juste après sa PR (`.claude/agents/lead-dev.md`) | 18:58 : PR #297 (WFS-338) en revue ; 17:24 : #311 (X5) refusée une 3e fois, fermée |
| dev-artiste-3d | développeur **et** artiste 3D (skills `agent-developer`, `waforspeed-natif`, `artiste-3d`) : voitures sans marque, modèles 3D + code ; demande son ticket au PO (`@devartiste`, `@dev3d`) | créé le 2026-11-09 19:22 UTC (demande de l'utilisateur) ; tickets voitures WFS-358 à 361 |
| artiste-3d | artiste 3D (skill `artiste-3d`) : choisit, retouche, intègre les modèles et actifs 3D, sans code du moteur ; demande son ticket au PO (`@artiste`) | créé le 2026-11-09 19:22 UTC (demande de l'utilisateur) |
| expert-cartes | développeur expert des cartes et de leur téléchargement (paquets de cartes, E14, WFS-301 à 314) ; skill `expert-cartes` ; demande son ticket au PO | créé le 2026-11-08 14:14 UTC (demande de l'utilisateur) |
| po-scrum-master | attribue les tickets, écrit les règles de gestion, tient `TACHES.md`, le journal et ce fichier | 12:17 (11/11) : WFS-412 attribué à agent expert ci cd (session 01UY1Vv4jR6p) ; 12:10 (11/11) : WFS-421 attribué à Claude, agent dev (01UY1Vv4ss) ; 12:09 (11/11) : « go Corniche » (WFS-405 réécrit, WFS-386 après la sortie), WFS-426 créé, ordre des 20 prochaines tâches |
| QA-arwa (qa-lead, Claude session 01Us6uCubd) | recette de nuit des tickets de `qa/Check.md`, verdicts et anomalies ; campagne VABF sur émulateur | recette du 2026-11-08 nuit terminée (01:06) : 25 tickets ; ANO-160 à 167 créées, planifiées par le PO à 01:12 |
| expert-realisme-3d | expert du graphisme et du réalisme 3D (WFS-148 à 167) : **consultant, pas développeur** ; conseille les développeurs, ne tient pas de ticket de code | à consulter par arwa-2 pour WFS-192 (méthode d'effacement des logos, ΔE2000 ≤ 2) |
| juriste | juriste (skill `juriste`, `@juriste`) : regarde l'application du côté juridique (marques, licences NC, crédits, RGPD, CGU, Google Play) ; rapports dans `juridique/` ; ne code pas | créé le 2026-11-10 12:40 UTC (demande de l'utilisateur) ; 1er travail attendu : avis sur la sortie Android (WFS-320, WFS-404) |
| smart-brainstorming | propose des idées dans `recommandations-smart-brain.md`, ne décide rien, ne parle qu'avec l'utilisateur | 2026-11-07 : constats et recommandations performance (ANO-125) transmis au PO sur ordre de l'utilisateur |
| agent expert ci cd (session 01UY1Vv4) | expert CI / CD (pipeline et livraison) (`.claude/agents/expert-ci-cd.md`) : diagnostics, US techniques de la CI | 2026-11-09 10:42:51 : WFS-351 🟣 PR #306 fusionnée par le lead dev (788c286) ; libre |
| agent expert ci cd (session 01UY1Vv2Hq) | expert CI / CD (pipeline et livraison) (`.claude/agents/expert-ci-cd.md`) : diagnostics, US techniques de la CI | 2026-11-11 09:13 : « Paquets de cartes » relancé sur `main` (ordre de l'utilisateur), run n° 2 vert, paquets de la Ville de WFS-416 publiés ; libre |

## Info pour les développeurs (2026-11-06 23:47 UTC)
- Le rendu se vérifie **en local** avec lavapipe : `mesa-vulkan-drivers` et `glslc` s'installent dans le conteneur (paquets apt), puis `WFS_BANC=<groupe> build/wfs_bench` (même rendu logiciel que la CI ; testé par Claude, agent 2 sur WFS-197).
