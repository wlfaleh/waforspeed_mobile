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

**Dernière mise à jour** : 2026-11-06 23:47:40 UTC (po-scrum-master)

## Agents développeurs

| Agent (session) | Ticket en cours | Ce qu'il fait | Branche, dernier envoi (UTC) | État |
|---|---|---|---|---|
| agent arwa (Claude, session trafic) | WFS-190 / WFS-150 | rouge `banc-essai (realisme)` des runs 790 / 791 (M-01 nuit +X à -1,50 %) corrigé : capture d'environnement suréchantillonnée 2 × 2 (PR #170, fusion par confiance) ; suit la CI de `main` | 150 : 23:47 (PR #170) | 🟣 suit la CI de `main` (realisme et endurance) |
| Claude, agent 1 (01UY1Vv2) | WFS-186 | Campagne : animaux de ferme (vaches, moutons, chevaux) en vrais modèles 3D libres (dépôts GitHub publics) | 186 : fusionnée 00:04 (PR #171, squash 83e9746) ; 199 : fusionnée 23:36 (PR #168, f279a8c) ; 198 : PR #165 ; 179 : PR #161 ; 176 : PR #158 | ✅ libre ; WFS-186, 199, 198, 179, 176 🟣 (le robot des statuts les passe en ✅) |
| Claude, agent 2 (01UY1Vv5) | WFS-201 | Vue cockpit réaliste, lot 2 : intérieurs A 45, S3, Giulia, 208, Gallardo, Mustang, Model 3, X5 (+ `eyeheightsitting` ANSUR-II) | 201 : à créer et pousser (attribué 23:47) ; 197 : fusionnée 23:44 (PR #169, squash 2cb114b) | ✅ actif ; WFS-197 🟣 (CI de `main` suivie par le PO, ✅ par le robot des statuts) |
| Claude, agent 4 (011cpqZan) |  |  |  | ✅ libre ; suit la CI de `main` de WFS-137 (🟣, PR #150) |
| Claude, agent 5 (011cpqZaDiS2) |  |  | 191 : fusionnée (PR #164, 9c64bf6) ; 140 : fusionnée (PR #137, ad4fa31) | ✅ libre ; WFS-191 et WFS-140 restent 🟣 (runs de main rouges sur des jobs d'autres tickets : `realisme` 42305395876, `endurance` 42257956758) |
| Claude, session dev expert (0183bv2X) |  | libre (5 tâches faites : WFS-136, 122, 145, 187, 188) | 188 : 19:58 | 🟣 WFS-188 fusionné (PR #154) ; suit la CI de main (WFS-145, 187, 188) |
| Claude, session dev 5bx |  |  |  | ✅ libre ; suit la CI de `main` de WFS-178 (🟣, PR #148) |
| arwa-2 (Claude, session 01Us6uCu) | WFS-158 | livraisons 1 à 5 fusionnées (PR #157, #162, #163, #166, #167 : audit, plaques, échelle et roues, Model 3 remplacée sans facettes) ; RG-C (X5) signalé au PO | 158 : 23:06 (dernier envoi de la branche, 00:06 heure de Paris) ; PR #167 fusionnée par confiance | 🔵 suit la CI de main (PR #157 à #167) ; suite : plaques avant manquantes, étriers, intérieurs, ST-4, ST-5 |
| Claude, Agent 0 developer (session 01UY1Vv2SSLK) | WFS-200 | CI de `main` rouge : écart M-01 nuit face +X (−1,50 %) du banc des passes ; bissection en cours | 200 : 23:13 | 🔴 P1 en cours ; suit aussi la CI de `main` de WFS-151 (🟣) |

## Tickets prêts sans agent

| Ticket | Épic | Pourquoi il attend |
|---|---|---|
| — | — | aucun ticket prêt sans agent (WFS-201 attribué à Claude, agent 2, 23:47) |
| WFS-202 | E3 (+ fichiers cockpit E10) | lot 3 de la vue cockpit : attend WFS-201 |
| — | — | WFS-152 à 167 (réalisme) attendent WFS-151 ✅ (🟣, PR #156) |

## Agents de pilotage

| Agent | Rôle | Dernière action |
|---|---|---|
| po-scrum-master | attribue les tickets, écrit les règles de gestion, tient `TACHES.md`, le journal et ce fichier | 2026-11-06 23:47:40 : WFS-201 et WFS-202 créés (lots 2 et 3 de WFS-195), WFS-201 attribué à Claude, agent 2 ; demande de 2 sites présentée à l'utilisateur ; suit la CI de `main` de WFS-197 |
| qa-lead | recette de nuit des tickets de `qa/Check.md`, verdicts et anomalies ; campagne VABF sur émulateur | recette des voitures en course (19:00) : PASSABLE, ANO-97 à 102 |
| expert-realisme-3d | expert du graphisme et du réalisme 3D (WFS-148 à 167) : **consultant, pas développeur** ; conseille les développeurs, ne tient pas de ticket de code | WFS-148 (référentiel du réel) ; aucun ticket en cours |
| smart-brainstorming | propose des idées dans `recommandations-smart-brain.md`, ne décide rien, ne parle qu'avec l'utilisateur | recommandations R01, R02 |

## Info pour les développeurs (2026-11-06 23:47 UTC)
- Le rendu se vérifie **en local** avec lavapipe : `mesa-vulkan-drivers` et `glslc` s'installent dans le conteneur (paquets apt), puis `WFS_BANC=<groupe> build/wfs_bench` (même rendu logiciel que la CI ; testé par Claude, agent 2 sur WFS-197).
