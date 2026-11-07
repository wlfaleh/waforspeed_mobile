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

**Dernière mise à jour** : 2026-11-07 13:11:11 UTC (po-scrum-master)

## Agents développeurs

| Agent (session) | Ticket en cours | Ce qu'il fait | Branche, dernier envoi (UTC) | État |
|---|---|---|---|---|
| agent arwa (Claude, session trafic) | WFS-222 | P1 [TECHNIQUE] : performance sur téléphone (≥ 60 img/s Basse, ≥ 45 Haute), piétons −50 % en premier (attribué 10:42) | 222 : PR #195 ouverte, CI de branche run 831 lancée (12:19, 5a3713c) ; 217 : fusionnée 10:32 (PR #189, squash a63d209) | 🔴 actif (P1) ; a demandé un 2e ticket (12:48) : aucun ticket attribuable hors E4 (WFS-230 et 214 attendent E13, WFS-215 attend E10) |
| Claude, agent 1 (01UY1Vv2) | WFS-234 | P2 : carte de navigation façon GPS à la place de la mini-carte (choix de l'utilisateur : « regarder avec le PO » ; attribué 12:53) | 234 : à pousser dès sa création ; 220 : PR #190 fusionnée | 🔧 actif |
| Claude, agent 2 (01UY1Vv5) | WFS-227 | relancé par l'utilisateur (« Prend une tâche ») : P2 [RÉALISME 1.3a] ombres du soleil en cascades stabilisées, pénombre physique (attribué 11:44) | 227 : 11:45 (55b5298) ; 221 : fusionnée 11:30 (PR #193) | 🔧 en cours |
| Claude, agent 3 (01UY1Vv3) | WFS-233 | Enfants et familles dans les rues (réalisme, E11) : lecture des règles et du code des piétons | 233 : branche poussée 12:55 | 🔧 en cours |
| Claude, agent 4 (011cpqZan) | WFS-236 | P1 [TECHNIQUE] : robot des statuts, un run de main annulé seulement à la publication compte comme vert (attribué 13:05) ; **à relire : RG-02 bis et test (e) ajoutés à 13:11** (publication annulée avant de démarrer seulement) | 236 : à pousser dès sa création ; 137 : fusionnée (PR #150) | 🔴 actif (P1) |
| Claude, agent 5 (011cpqZaDiS2) |  |  | 219 : fusionnée (PR #191, 4448d7c, 11:10 UTC, squash) ; 210 : fusionnée (PR #185) | ✅ libre, aucun ticket attribuable (WFS-219 🟣 ; décisions Mustang et remplaçants posées à l'utilisateur) |
| Claude, session dev expert (0183bv2X) |  | libre (5 tâches faites : WFS-136, 122, 145, 187, 188) | — | ✅ libre : CI de main verte (#799), WFS-145 / 187 / 188 validés par la QA |
| Claude, session dev 5bx | WFS-216 | P3 : lot E2 (ANO-103, 105, 108) (fusionné, PR #196, squash f5340d7) | 216 : fusionnée 12:42 | 🟢 libre |
| arwa-2 (Claude, session 01Us6uCu) | WFS-158 | livraisons 1 à 9 fusionnées (PR #157, #162, #163, #166, #167, #186, #187, #197, #198 : audit, plaques des 20 voitures, échelle et roues, Model 3 remplacée, T-Cross à sa vraie taille, 4 niveaux de détail, test strict) ; RG-C (X5) signalé au PO | 158 : 12:57:17 UTC ; PR #198 fusionnée par confiance | ✅ actif ; suite : étriers, intérieurs, sous-caisse (ST-3) |
| Claude, Agent 0 developer (session 01UY1Vv2SSLK) | WFS-161 | Kit d'architecture : ST-1 à ST-6 faits (textures procédurales provisoires) ; CI de branche run 822 avant fusion | 161 : 11:48 | 🔧 en cours (CI de branche) |

## Tickets prêts sans agent

| Ticket | Épic | Pourquoi il attend |
|---|---|---|
| WFS-215 (P3) | E10 | épic occupée par WFS-158 (arwa-2) |
| WFS-218 (P1) | E4 | épic occupée par WFS-222 (agent arwa) |
| WFS-224 (P1), WFS-226 (P1) | E4 (+ E3) | épic E4 occupée par WFS-222 (agent arwa) |
| WFS-230 (P1) | E13 | nouveau (risque signalé par session dev 5bx) ; attend l'épic E13 (WFS-227, agent 2) |
| WFS-237 (P1) | E7 | réservé à l'agent expert ci ; attend WFS-236 🟣 (agent 4) |
| WFS-214 (P2) | E13 | WFS-213 ✅ (CI main #817) ; attend l'épic E13 (WFS-227, agent 2), après WFS-230 |

## Agents de pilotage

| Agent | Rôle | Dernière action |
|---|---|---|
| po-scrum-master | attribue les tickets, écrit les règles de gestion, tient `TACHES.md`, le journal et ce fichier | 2026-11-07 13:11:11 : WFS-236 complété (RG-02 bis), WFS-237 créé et réservé à l'agent expert ci |
| QA-arwa (qa-lead, Claude session 01Us6uCubd) | recette de nuit des tickets de `qa/Check.md`, verdicts et anomalies ; campagne VABF sur émulateur | recette du 2026-11-07 : WFS-211, 212, 213 VALIDÉS, WFS-208 PASSABLE (ANO-112) |
| expert-realisme-3d | expert du graphisme et du réalisme 3D (WFS-148 à 167) : **consultant, pas développeur** ; conseille les développeurs, ne tient pas de ticket de code | WFS-148 (référentiel du réel) ; aucun ticket en cours |
| smart-brainstorming | propose des idées dans `recommandations-smart-brain.md`, ne décide rien, ne parle qu'avec l'utilisateur | recommandations R01, R02 |
| agent expert ci (session 01UY1Vv4) | expert de la pipeline CI (`.claude/agents/expert-ci.md`) : diagnostics, propositions d'US techniques | 2026-11-07 : propositions 1, 2, 5 validées (Smart Brain, utilisateur) → WFS-236 complété, WFS-237 qu'il développera après WFS-236 |

## Info pour les développeurs (2026-11-06 23:47 UTC)
- Le rendu se vérifie **en local** avec lavapipe : `mesa-vulkan-drivers` et `glslc` s'installent dans le conteneur (paquets apt), puis `WFS_BANC=<groupe> build/wfs_bench` (même rendu logiciel que la CI ; testé par Claude, agent 2 sur WFS-197).
