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

**Dernière mise à jour** : 2026-11-07 13:38:00 UTC (po-scrum-master)

## Agents développeurs

| Agent (session) | Ticket en cours | Ce qu'il fait | Branche, dernier envoi (UTC) | État |
|---|---|---|---|---|
| agent arwa (Claude, session trafic) | WFS-222 | P1 [TECHNIQUE] : performance sur téléphone (≥ 60 img/s Basse, ≥ 45 Haute), piétons −50 % en premier (attribué 10:42) | 222 : 12:15 (5a3713c), PR #195 ouverte, CI de branche run 831 ; 217 : fusionnée 10:32 (PR #189) | 🔴 actif (P1) ; épic E4 tenue : WFS-218, 224, 226 attendent sa fusion |
| Claude, agent 1 (01UY1Vv2) | WFS-234 | P2 : carte de navigation façon GPS à la place de la mini-carte (attribué 12:53) | 234 : aucune branche poussée (attribué 12:53) ; 220 : PR #190 fusionnée | ⚠️ pas de branche 30 min après l'attribution (ticket gardé : règle des 10 h) |
| Claude, agent 2 (01UY1Vv5) | WFS-227 | P2 [RÉALISME 1.3a] ombres du soleil en cascades stabilisées, pénombre physique (attribué 11:44) | 227 : 12:12 (f51c43e) ; 221 : fusionnée 11:30 (PR #193) | 🔧 en cours |
| Claude, agent 3 (01UY1Vv3) | WFS-233 | P2 : enfants et familles dans les rues (E11) | 233 : 12:54 (960715d) | 🔧 en cours |
| Claude, agent 4 (011cpqZan) |  | WFS-236 fusionné (PR #199) ; rattrapage des runs #818 à #825 lancé (RG-06) | 236 : 13:16 (PR #199, squash c2be79f) | ✅ libre : WFS-239 (P1) attribuable |
| Claude, agent 5 (011cpqZaDiS2) |  | WFS-219 et 210 fusionnés | 219 : fusionnée 11:10 (PR #191) ; 210 : fusionnée (PR #185) | ✅ libre : WFS-239 (P1) attribuable ; décisions Mustang et remplaçants attendues de l'utilisateur |
| Claude, session dev expert (0183bv2X) |  | libre (WFS-136, 122, 145, 187, 188 faits) | — | ✅ libre : WFS-239 (P1) attribuable |
| Claude, session dev 5bx |  | WFS-216 fusionné (PR #196, squash f5340d7) | 216 : fusionnée 12:42 | ✅ libre : WFS-239 (P1) attribuable |
| arwa-2 (Claude, session 01Us6uCu) | WFS-235 | P1 : vue Cockpit (bandes noires, regard qui suit la route, réglage « Regard cockpit », intérieurs réalistes), attribué 13:36 ; WFS-158 🟣 (PR #157 à #202) | 235 : branche à pousser tout de suite ; 158 : 13:30 (PR #202 fusionnée) | 🔧 en cours |
| Claude, Agent 0 developer (session 01UY1Vv2SSLK) | WFS-161 | Kit d'architecture : ST-1 à ST-6 faits ; CI de branche run 822 avant fusion | 161 : 13:30 (28f5b54) | 🔧 en cours |

## Tickets prêts sans agent

| Ticket | Épic | Pourquoi il attend |
|---|---|---|
| 🔴 WFS-239 (P1) | E4 (exception PO : `qualite_banc.cpp` seul) | attribuable tout de suite au premier développeur libre qui demande |
| WFS-218 (P1), WFS-224 (P1), WFS-226 (P1) | E4 (+ E3) | épic E4 tenue par WFS-222 (agent arwa, PR #195) |
| WFS-230 (P1), WFS-214 (P2) | E13 | épic E13 tenue par WFS-227 (agent 2) |
| WFS-232 (P2), WFS-215 (P3) | E3 + E10, E10 | épics tenues par WFS-235 (arwa-2) |

## Agents de pilotage

| Agent | Rôle | Dernière action |
|---|---|---|
| po-scrum-master | attribue les tickets, écrit les règles de gestion, tient `TACHES.md`, le journal et ce fichier | 2026-11-07 13:36 : WFS-235 attribué à arwa-2 ; WFS-239 créé (banc qualité) ; verre de l'A45 → WFS-215, étriers → WFS-160 |
| QA-arwa (qa-lead, Claude session 01Us6uCubd) | recette de nuit des tickets de `qa/Check.md`, verdicts et anomalies ; campagne VABF sur émulateur | recette du 2026-11-07 : WFS-211, 212, 213 VALIDÉS, WFS-208 PASSABLE (ANO-112) |
| expert-realisme-3d | expert du graphisme et du réalisme 3D (WFS-148 à 167) : **consultant, pas développeur** ; conseille les développeurs, ne tient pas de ticket de code | à consulter par arwa-2 pour WFS-235 (RG-04, intérieurs) |
| smart-brainstorming | propose des idées dans `recommandations-smart-brain.md`, ne décide rien, ne parle qu'avec l'utilisateur | recommandations R01, R02 ; avis sur WFS-236 et 237 |
| agent expert ci (session 01UY1Vv4) | expert de la pipeline CI (`.claude/agents/expert-ci.md`) : diagnostics, US techniques de la CI | 2026-11-07 13:27:37 : WFS-238 🟣 fusionné (PR #201) ; WFS-237 🟣 (mesure avant / après prévue à 15:52 UTC) |

## Info pour les développeurs (2026-11-06 23:47 UTC)
- Le rendu se vérifie **en local** avec lavapipe : `mesa-vulkan-drivers` et `glslc` s'installent dans le conteneur (paquets apt), puis `WFS_BANC=<groupe> build/wfs_bench` (même rendu logiciel que la CI ; testé par Claude, agent 2 sur WFS-197).
