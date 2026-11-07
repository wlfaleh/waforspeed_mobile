# Suivi des agents — qui fait quoi

> **Règle de l'utilisateur (2026-11-06)** : ce fichier est **toujours synchronisé** avec `main`.
> - **Chaque agent développeur** met à jour **sa propre ligne** à chaque étape (ticket attribué, branche créée et poussée, PR ouverte,
>   fusion, CI de `main` verte, ticket fini, agent libre) et l'envoie sur `main` **dans le même commit** que `TACHES.md`.
> - **Le PO** remet tout le fichier à jour à chaque appel (attribution, point d'avancement) d'après `TACHES.md` et l'heure du dernier
>   envoi de chaque branche (`git log -1 --format=%ci origin/feature/waforspeed-XX`), et met à jour « Dernière mise à jour ».
> - Mêmes règles que `TACHES.md` : partir de `main` (`git pull`), ne modifier que sa ligne, ne jamais copier ce fichier d'une autre branche.
> - **Colonne « Ticket en cours »** (règle de l'utilisateur, 2026-11-06) : seulement les tickets 🔧 En cours ou 🔀 En PR ; **vide** si l'agent n'en a
>   aucun (les tickets 🟣 fusionnés ou ✅ terminés n'y figurent pas ; la CI de `main` suivie s'écrit dans « État »).
> - **Nom de l'agent** : la colonne « Agent » donne le nom à mettre à la fin de chaque titre de commit, entre parenthèses (règle de l'utilisateur, 2026-11-06).
> - **Ce fichier suit les agents, pas les tickets** (règle de l'utilisateur, 2026-11-06) : une ligne par agent (ce qu'il fait, sa branche,
>   son dernier envoi, son état). La liste des tickets et leurs statuts restent dans `TACHES.md`.
> - **Après une fusion, l'agent est libre** (règle de l'utilisateur, 2026-11-06 23:20 UTC) : il ne « suit » plus la CI de `main` ; la colonne État ne le dit plus.
> - Alerte ⚠️ quand un ticket n'a rien reçu depuis plus de 2 h, ou n'a pas de branche 30 min après l'attribution ; 🔴 pour un ticket P1.

**Dernière mise à jour** : 2026-11-07 11:32:50 UTC (po-scrum-master)

## Agents développeurs

| Agent (session) | Ticket en cours | Ce qu'il fait | Branche, dernier envoi (UTC) | État |
|---|---|---|---|---|
| agent arwa (Claude, session trafic) | WFS-222 | P1 [TECHNIQUE] : performance sur téléphone (≥ 60 img/s Basse, ≥ 45 Haute), piétons −50 % en premier (attribué 10:42) | 222 : 11:15 (9c50a88) ; 217 : fusionnée 10:32 (PR #189, squash a63d209) | 🔴 actif (P1) |
| Claude, agent 1 (01UY1Vv2) | — | WFS-220 fusionnée (PR #190, squash 58b71b9) ; aucun ticket attribuable : WFS-214 (E13) attend WFS-213 ✅ et l'épic E13 (WFS-227 en cours) | 220 : PR #190 fusionnée ; 213 : PR #184 | 🟢 libre (aucun ticket attribuable) |
| Claude, agent 2 (01UY1Vv5) | WFS-227 | P2 Réalisme E13 : ombres du soleil en cascades stabilisées, pénombre physique (lot 1/3 de WFS-153, attribué 11:31) | 227 : branche à pousser ; 221 : fusionnée 11:30 (PR #193) | 🔧 actif |
| Claude, agent 4 (011cpqZan) |  |  |  | ✅ libre (WFS-137 : CI main #799 verte) |
| Claude, agent 5 (011cpqZaDiS2) |  |  | 219 : fusionnée (PR #191, 4448d7c, 11:10 UTC, squash) ; 210 : fusionnée (PR #185) | ✅ libre, aucun ticket attribuable (WFS-219 🟣 ; décisions Mustang et remplaçants posées à l'utilisateur) |
| Claude, session dev expert (0183bv2X) |  | libre (5 tâches faites : WFS-136, 122, 145, 187, 188) | — | ✅ libre : CI de main verte (#799), WFS-145 / 187 / 188 validés par la QA |
| Claude, session dev 5bx | WFS-225 | P1 : Campagne de nuit, lampadaires allumés et nuit lisible (attribué 11:28) | 225 : branche à pousser ; 223 : fusionnée 11:27 (PR #192, squash 00be4ab) | 🔴 actif (P1) |
| arwa-2 (Claude, session 01Us6uCu) | WFS-158 | livraisons 1 à 7 fusionnées (PR #157, #162, #163, #166, #167, #186, #187 : audit, plaques des 20 voitures, échelle et roues, Model 3 remplacée, T-Cross à sa vraie taille) ; RG-C (X5) signalé au PO | 158 : 10:43:54 UTC (c1d2e92) ; PR #187 fusionnée par confiance | ✅ actif ; suite : étriers, intérieurs, sous-caisse (ST-3), LOD 4 niveaux (ST-4), test strict (ST-5) |
| Claude, Agent 0 developer (session 01UY1Vv2SSLK) | WFS-161 | Kit d'architecture : ST-1 à ST-6 faits (textures procédurales provisoires) ; CI de branche run 822 avant fusion | 161 : 10:23 (9c5d416) | 🔧 en cours (CI de branche) |

## Tickets prêts sans agent

| Ticket | Épic | Pourquoi il attend |
|---|---|---|
| WFS-215 (P3) | E10 | épic occupée par WFS-158 (arwa-2) |
| WFS-218 (P1) | E4 | épic occupée par WFS-222 (agent arwa) |
| WFS-216 (P3) | E2 | épic occupée par WFS-225 (P1, Claude, session dev 5bx) |
| WFS-224 (P1), WFS-226 (P1) | E4 (+ E3) | épic E4 occupée par WFS-222 (agent arwa) |
| WFS-214 (P2) | E13 | attend WFS-213 ✅ (CI de main en file) et l'épic E13 (WFS-227, agent 2) |
| WFS-214 (P2) | E13 | attend WFS-213 ✅ (🟣 ; run de `main` 42361330960 en file d'attente depuis 09:49) |

## Agents de pilotage

| Agent | Rôle | Dernière action |
|---|---|---|
| po-scrum-master | attribue les tickets, écrit les règles de gestion, tient `TACHES.md`, le journal et ce fichier | 2026-11-07 11:28:30 : WFS-225 attribué à Claude, session dev 5bx |
| QA-arwa (qa-lead, Claude session 01Us6uCubd) | recette de nuit des tickets de `qa/Check.md`, verdicts et anomalies ; campagne VABF sur émulateur | recette en cours (2026-11-07) : WFS-212, 211 VALIDÉS ; WFS-213, 208 en cours |
| expert-realisme-3d | expert du graphisme et du réalisme 3D (WFS-148 à 167) : **consultant, pas développeur** ; conseille les développeurs, ne tient pas de ticket de code | WFS-148 (référentiel du réel) ; aucun ticket en cours |
| smart-brainstorming | propose des idées dans `recommandations-smart-brain.md`, ne décide rien, ne parle qu'avec l'utilisateur | recommandations R01, R02 |

## Info pour les développeurs (2026-11-06 23:47 UTC)
- Le rendu se vérifie **en local** avec lavapipe : `mesa-vulkan-drivers` et `glslc` s'installent dans le conteneur (paquets apt), puis `WFS_BANC=<groupe> build/wfs_bench` (même rendu logiciel que la CI ; testé par Claude, agent 2 sur WFS-197).
