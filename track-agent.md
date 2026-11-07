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

**Dernière mise à jour** : 2026-11-07 17:15:49 UTC (po-scrum-master)

## Agents développeurs

| Agent (session) | Ticket en cours | Ce qu'il fait | Branche, dernier envoi (UTC) | État |
|---|---|---|---|---|
| agent arwa (Claude, session trafic) | WFS-257, WFS-244 | P1 WFS-257 personnes qui apparaissent / disparaissent (E11), attribué 16:35 ; **en parallèle** (demande de l'utilisateur) P3 WFS-244 plaques à bords nets sur la chaussée (E9), attribué 17:05 ; WFS-226 fusionné | 257 : 16:36 (a5275c8, commit de début) ; 244 : 17:06 (9b43ee6, commit de début) ; 226 : fusionnée 16:40 (PR #214) | 🔴 en cours (P1 + P3) |
| Claude, agent 1 (01UY1Vv2) | WFS-230 | P1 nuit sur route non éclairée : exposition au clair de lune, vision de nuit, `GameView::night` ; attribué 16:00 | 230 : 17:14 ; 234 : PR #209 fusionnée | 🔴 en cours (P1) |
| Claude, agent 2 (01UY1Vv5) | — (WFS-227 fusionné) | libre (ne prend pas de ticket sans demande de l'utilisateur) | 227 : fusionnée 15:58 (PR #210) ; 221 : fusionnée (PR #193) | 🟢 libre |
| Claude, agent 3 (01UY1Vv3) | WFS-239 | P1 [TECHNIQUE] banc qualité : voiture du trafic absente à 120 m en Basse ; recherche de la cause | 239 : 13:43 (323eeb1, commit de début seul) ; 233 : fusionnée (PR #203, 4761db5) | 🔴 ⚠️ P1 : rien poussé depuis 13:43 (3 h 30) ; relance, pas de retrait avant 10 h |
| Claude, agent 4 (011cpqZan) |  | Stoppé par l'utilisateur (15:06 UTC) : **plus aucun ticket à lui donner** ; WFS-224 repris (⏳ Prêt, pour un autre développeur) | 224 : 14:49 (533e360, commit de début seul, sans code) ; 240 : fusionnée (PR #205) | ⛔ stoppé par l'utilisateur |
| Claude, agent 5 (011cpqZaDiS2) |  |  | 248 : fusionnée (PR #208, d611b1e, 15:32 UTC, squash) | ✅ libre (WFS-248 🟡 : confirmation de l'utilisateur attendue) |
| Claude, agent dev X (0183bv2X, ex-« session dev expert ») | WFS-246 | P1 [TECHNIQUE] performance, 2e passage, **recentré à 17:13** : mesure sur le téléphone (RG-06), faces arrière (levier 1 poussé), résolution sans `resize()` en course (RG-07) ; suite WFS-262 à 265 (E4) proposée à lui ensuite | 246 : 17:11 (e79fa14, levier 1) | 🔴 en cours (P1) |
| Claude, agent dev Y |  | WFS-216 fusionné (PR #196, squash f5340d7) | 216 : fusionnée 12:42 | ⏸️ en pause (ordre de l'utilisateur) jusqu'à 19 h, heure de France (18:00 UTC) : aucun ticket avant |
| arwa-2 (Claude, session 01Us6uCu) | WFS-235, WFS-245 | P1 WFS-235 vue Cockpit (E3 + E10) : fenêtre de la rue 0 % sur les 20 voitures, réglage « Regard cockpit » fait ; reste le tableau du critère 2, le parcours et la fusion (🟡) ; **en parallèle** (demande de l'utilisateur, 17:07) P3 WFS-245 étiquettes du HUD lisibles (E5), attribué 17:12 | 235 : 16:59 (6fb57bf, synchronisée avec main) ; 245 : branche à créer | 🔴 en cours (P1 + P3) |
| Claude, Agent 0 developer (session 01UY1Vv2SSLK) | WFS-247 | P1 [TECHNIQUE] fluidité, 2e passage (WFS-221 refusé, ANO-126) : la voiture saute encore devant la caméra en vue Proche / Poursuite ; + roues avant d'après l'angle appliqué | 247 : 16:15 (791997c, roues avant) ; 161 : fusionnée 15:20 | 🔴 en cours (P1) |
| Claude, agent dev conduite (session 01UY1Vv1Hhvw) | WFS-258 | P1 conduite 2e passage : **d'abord remettre l'ancienne conduite** (`g_conduiteMonde = false`), puis corriger les points de l'utilisateur ; WFS-256 (chocs) en pause ; sujets de conduite réservés à lui (exception de l'utilisateur) | 258 : branche à créer (attribué 17:02) ; 256 : 17c1334 (en pause) | 🔴 en cours (P1) |

## Tickets prêts sans agent

| Ticket | Épic | Pourquoi il attend |
|---|---|---|
| WFS-262 à 265 (P1) | E4 | épic E4 tenue par WFS-246 (agent dev X) ; à la suite, une par une |
| WFS-224 (P1) | E4 | épic E4 tenue par WFS-246 (zone `src/render/**`, qui contient `effets_*`) ; ensuite au premier développeur qui demande, **pas** l'agent 4 |
| WFS-249 (P1) | E2 + E13 | E13 tenue par WFS-230 (agent 1) ; WFS-248 🟡 ; pas à la session dev 5bx |
| WFS-214 (P2) | E13 | épic E13 tenue par WFS-230 (agent 1) |
| WFS-232 (P2), WFS-215 (P3) | E3 + E10, E10 | épics tenues par WFS-235 (arwa-2) |
| WFS-242 (P1), WFS-243 (P2) | E3 | épic E3 tenue par WFS-258 (agent dev conduite) et WFS-235 (arwa-2) ; WFS-243 après WFS-242 |
| WFS-252 (P2) | E7 | en attente de WFS-251 et WFS-250 ; réservé à l'agent expert ci |
| WFS-261 (P2) | E6 | nouveau (17:12, règle des 8 prêts) : au premier développeur qui demande ; cède l'épic à WFS-260 (agent dev conduite) |

## Agents de pilotage

| Agent | Rôle | Dernière action |
|---|---|---|
| expert-conduite | développeur expert en physique de la voiture (skill `expert-conduite`) : chantier « Conduite réaliste » | créé le 2026-11-07 |
| po-scrum-master | attribue les tickets, écrit les règles de gestion, tient `TACHES.md`, le journal et ce fichier | 2026-11-07 17:15 : constats perf de smart-brainstorming étudiés ; WFS-246 recentré ; WFS-262 à 265 créés (E4, après WFS-246) |
| QA-arwa (qa-lead, Claude session 01Us6uCubd) | recette de nuit des tickets de `qa/Check.md`, verdicts et anomalies ; campagne VABF sur émulateur | recette en cours (2026-11-07 17:1x) : 26 tickets de `qa/Check.md` en 5 lots |
| expert-realisme-3d | expert du graphisme et du réalisme 3D (WFS-148 à 167) : **consultant, pas développeur** ; conseille les développeurs, ne tient pas de ticket de code | à consulter par arwa-2 pour WFS-235 (RG-04, intérieurs) |
| smart-brainstorming | propose des idées dans `recommandations-smart-brain.md`, ne décide rien, ne parle qu'avec l'utilisateur | 2026-11-07 : constats et recommandations performance (ANO-125) transmis au PO sur ordre de l'utilisateur |
| agent expert ci (session 01UY1Vv4) | expert de la pipeline CI (`.claude/agents/expert-ci.md`) : diagnostics, US techniques de la CI | 2026-11-07 16:52:32 : WFS-250 🟣 fusionné (PR #215) ; WFS-251 🔧 (comparaison locale lavapipe en cours) ; WFS-252 ensuite |

## Info pour les développeurs (2026-11-06 23:47 UTC)
- Le rendu se vérifie **en local** avec lavapipe : `mesa-vulkan-drivers` et `glslc` s'installent dans le conteneur (paquets apt), puis `WFS_BANC=<groupe> build/wfs_bench` (même rendu logiciel que la CI ; testé par Claude, agent 2 sur WFS-197).
