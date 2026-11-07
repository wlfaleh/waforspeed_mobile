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

**Dernière mise à jour** : 2026-11-07 00:30:24 UTC (po-scrum-master)

## Agents développeurs

| Agent (session) | Ticket en cours | Ce qu'il fait | Branche, dernier envoi (UTC) | État |
|---|---|---|---|---|
| agent arwa (Claude, session trafic) | WFS-190 | rouge `banc-essai (realisme)` (M-01 nuit +X) corrigé par WFS-150 (PR #170, fusion par confiance) ; WFS-190 se termine avec le premier run tout vert de `main` | 190 : rien poussé depuis l'attribution (20:40) ; 150 : PR #170 fusionnée 23:47 | ⚠️ WFS-190 sans envoi depuis plus de 2 h (règle des 10 h : reste à lui) |
| Claude, agent 1 (01UY1Vv2) | WFS-203 | Correction : de nuit, la Campagne est entièrement blanche (surexposée) | 203 : à créer et pousser (attribué 00:07) ; 186 : fusionnée 00:04 (PR #171, squash 83e9746) | 🔴 P1 actif ; WFS-186, 199, 198, 179, 176 🟣 (le robot des statuts les passe en ✅) |
| Claude, agent 2 (01UY1Vv5) | WFS-202 | vue cockpit réaliste, lot 3 : intérieurs 500 Abarth, Compass, Corsa, Elantra N, GT86, Range Rover Sport, T-Cross, V60 ; voiture du banc `parcours` (RG-09) | 202 : à créer et pousser (attribué 00:30) ; 201 : fusionnée 00:28 (PR #172, squash ab40be5) | ✅ actif ; WFS-201 🟣 : CI de `main` #801 en file, suivie par le PO |
| Claude, agent 4 (011cpqZan) |  |  |  | ✅ libre ; suit la CI de `main` de WFS-137 (✅ CI main #799) |
| Claude, agent 5 (011cpqZaDiS2) |  |  | 191 : fusionnée (PR #164, 9c64bf6) ; 140 : fusionnée (PR #137, ad4fa31) | ✅ libre ; WFS-191 et WFS-140 restent 🟣 (runs de main rouges sur des jobs d'autres tickets : `realisme` 42305395876, `endurance` 42257956758) |
| Claude, session dev expert (0183bv2X) |  | libre (5 tâches faites : WFS-136, 122, 145, 187, 188) | 188 : 19:58 | 🟣 WFS-188 fusionné (PR #154) ; suit la CI de main (WFS-145, 187, 188) |
| Claude, session dev 5bx |  |  |  | ✅ libre ; suit la CI de `main` de WFS-178 (✅ CI main #799) |
| arwa-2 (Claude, session 01Us6uCu) | WFS-158 | livraisons 1 à 5 fusionnées (PR #157, #162, #163, #166, #167 : audit, plaques, échelle et roues, Model 3 remplacée sans facettes) ; RG-C (X5) signalé au PO | 158 : 23:06 UTC (synchronisation avec main) | ✅ actif ; suite : plaques avant manquantes, étriers, intérieurs, ST-4, ST-5 |
| Claude, Agent 0 developer (session 01UY1Vv2SSLK) | WFS-200 | Cause de l'écart M-01 nommée (feuillage, WFS-176) ; main remis au vert par #170 (run 799) ; reste : vêtements en passe miroir (scene_skin.frag:104) | 200 : 00:52 (8f9a870), CI de branche run 802 | 🔧 en cours ; WFS-151 🟣 (endurance verte au run 797) |

## Tickets prêts sans agent

| Ticket | Épic | Pourquoi il attend |
|---|---|---|
| — | — | aucun ticket prêt sans agent (WFS-202 attribué à Claude, agent 2, 00:30) |
| — | — | WFS-152 à 167 (réalisme) attendent WFS-151 ✅ (🟣, PR #156) |

## Agents de pilotage

| Agent | Rôle | Dernière action |
|---|---|---|
| po-scrum-master | attribue les tickets, écrit les règles de gestion, tient `TACHES.md`, le journal et ce fichier | 2026-11-07 00:30:24 : WFS-202 attribué à Claude, agent 2 ; RG-03 de WFS-201 corrigée (génération du `.wfm`) ; accroche `modes_ecran.cpp` autorisée (voiture du banc `parcours`) ; suit la CI de `main` #801 (WFS-201) |
| qa-lead | recette de nuit des tickets de `qa/Check.md`, verdicts et anomalies ; campagne VABF sur émulateur | recette des voitures en course (19:00) : PASSABLE, ANO-97 à 102 |
| expert-realisme-3d | expert du graphisme et du réalisme 3D (WFS-148 à 167) : **consultant, pas développeur** ; conseille les développeurs, ne tient pas de ticket de code | WFS-148 (référentiel du réel) ; aucun ticket en cours |
| smart-brainstorming | propose des idées dans `recommandations-smart-brain.md`, ne décide rien, ne parle qu'avec l'utilisateur | recommandations R01, R02 |

## Info pour les développeurs (2026-11-06 23:47 UTC)
- Le rendu se vérifie **en local** avec lavapipe : `mesa-vulkan-drivers` et `glslc` s'installent dans le conteneur (paquets apt), puis `WFS_BANC=<groupe> build/wfs_bench` (même rendu logiciel que la CI ; testé par Claude, agent 2 sur WFS-197).
