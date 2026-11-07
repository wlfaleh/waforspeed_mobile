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

**Dernière mise à jour** : 2026-11-07 16:05:11 UTC (po-scrum-master)

## Agents développeurs

| Agent (session) | Ticket en cours | Ce qu'il fait | Branche, dernier envoi (UTC) | État |
|---|---|---|---|---|
| agent arwa (Claude, session trafic) | WFS-226 | P1 nuit : phares allumés sur toutes les autres voitures (trafic, adversaires, police), attribué 16:05 ; WFS-218 🟣 (PR #206, ae49782, run 844 de main en cours) | 226 : branche à créer et pousser ; 218 : fusion 14:57 (ae49782) | 🔴 en cours (P1) |
| Claude, agent 1 (01UY1Vv2) | WFS-230 | P1 nuit sur route non éclairée (E13) : exposition qui s'adapte au clair de lune loin des lampadaires, herbe, arbres et voiture visibles ; + accroche `GameView::night` (RG-09) ; attribué 16:00 | 230 : branche à créer et pousser ; 234 : PR #209 fusionnée | 🔴 en cours (P1) |
| Claude, agent 2 (01UY1Vv5) | — (WFS-227 fusionné) | libre (ne prend pas de ticket sans demande de l'utilisateur) | 227 : fusionnée 15:58 (PR #210) ; 221 : fusionnée (PR #193) | 🟢 libre |
| Claude, agent 3 (01UY1Vv3) | WFS-239 | P1 [TECHNIQUE] banc qualité : voiture du trafic absente à 120 m en Basse ; recherche de la cause | 239 : 13:43 (323eeb1) ; 233 : fusionnée (PR #203, 4761db5) | 🔴 actif (P1) |
| Claude, agent 4 (011cpqZan) |  | Stoppé par l'utilisateur (15:06 UTC) : **plus aucun ticket à lui donner** ; WFS-224 repris (⏳ Prêt, pour un autre développeur) | 224 : 14:49 (533e360, commit de début seul, sans code) ; 240 : fusionnée (PR #205) | ⛔ stoppé par l'utilisateur |
| Claude, agent 5 (011cpqZaDiS2) |  |  | 248 : fusionnée (PR #208, d611b1e, 15:32 UTC, squash) | ✅ libre (WFS-248 🟡 : confirmation de l'utilisateur attendue) |
| Claude, agent dev X (0183bv2X, ex-« session dev expert ») |  |  |  | ⏸️ en pause (ordre de l'utilisateur) jusqu'à 19 h, heure de France (18:00 UTC) : aucun ticket avant |
| Claude, agent dev Y |  | WFS-216 fusionné (PR #196, squash f5340d7) | 216 : fusionnée 12:42 | ⏸️ en pause (ordre de l'utilisateur) jusqu'à 19 h, heure de France (18:00 UTC) : aucun ticket avant |
| arwa-2 (Claude, session 01Us6uCu) | WFS-235 | P1 : vue Cockpit (bandes noires, regard qui suit la route, réglage « Regard cockpit », intérieurs réalistes), attribué 13:36 ; arbitrage RG-02 / RG-05 rendu 14:28 (option A, vraie géométrie) ; WFS-158 🟣 (PR #157 à #202) | 235 : 14:03 (9fb8cb9) ; 158 : 13:30 (PR #202 fusionnée) | 🔧 en cours |
| Claude, Agent 0 developer (session 01UY1Vv2SSLK) | WFS-247 | P1 [TECHNIQUE] fluidité, 2e passage (WFS-221 refusé, ANO-126) : la voiture saute encore devant la caméra en vue Proche / Poursuite ; attribué 15:23 (1er de l'ordre de l'utilisateur) | 247 : branche à créer et pousser ; 161 : fusionnée 15:20 (4fe82c8) | 🔴 en cours (P1) |
| Claude, agent dev conduite (session 01UY1Vv1Hhvw) | WFS-253 | P1 étape 2 « Conduite réaliste » : aides (direction, ABS, antipatinage, ESP) réglées et mesurées ; WFS-241 🟡 (PR #211) | 253 : branche poussée 16:0x | 🔴 en cours (P1) |

## Tickets prêts sans agent

| Ticket | Épic | Pourquoi il attend |
|---|---|---|
| WFS-224 (P1) | E4 | repris à l'agent 4 (stoppé par l'utilisateur) : au premier développeur qui demande, **pas** l'agent 4 |
| WFS-246 (P1) | E4 | épic E4 tenue (WFS-218, 224, 239) ; pas à l'agent arwa |
| WFS-249 (P1) | E2 + E13 | E13 tenue par WFS-230 (agent 1) ; WFS-248 🟡 ; pas à la session dev 5bx |
| WFS-214 (P2) | E13 | épic E13 tenue par WFS-230 (agent 1) |
| WFS-232 (P2), WFS-215 (P3) | E3 + E10, E10 | épics tenues par WFS-235 (arwa-2) |
| WFS-243 (P2) | E3 | après WFS-241 et WFS-242 (mêmes fichiers ; sessions archivées, tickets prêts) |
| WFS-252 (P2) | E7 | en attente de WFS-251 et WFS-250 ; réservé à l'agent expert ci |
| WFS-244 (P3) | E9 | épic E9 libre : attribuable tout de suite |
| WFS-245 (P3) | E5 | épic E5 tenue par WFS-234 (agent 1) |

## Agents de pilotage

| Agent | Rôle | Dernière action |
|---|---|---|
| expert-conduite | développeur expert en physique de la voiture (skill `expert-conduite`) : chantier « Conduite réaliste » | créé le 2026-11-07 |
| po-scrum-master | attribue les tickets, écrit les règles de gestion, tient `TACHES.md`, le journal et ce fichier | 2026-11-07 15:47 : WFS-251 confié à l'agent expert ci (exception de l'utilisateur « tu as le droit exclusivement pour le toucher ») |
| QA-arwa (qa-lead, Claude session 01Us6uCubd) | recette de nuit des tickets de `qa/Check.md`, verdicts et anomalies ; campagne VABF sur émulateur | recette du 2026-11-07 : WFS-211, 212, 213 VALIDÉS, WFS-208 PASSABLE (ANO-112) |
| expert-realisme-3d | expert du graphisme et du réalisme 3D (WFS-148 à 167) : **consultant, pas développeur** ; conseille les développeurs, ne tient pas de ticket de code | à consulter par arwa-2 pour WFS-235 (RG-04, intérieurs) |
| smart-brainstorming | propose des idées dans `recommandations-smart-brain.md`, ne décide rien, ne parle qu'avec l'utilisateur | recommandations R01, R02 ; avis sur WFS-236 et 237 |
| agent expert ci (session 01UY1Vv4) | expert de la pipeline CI (`.claude/agents/expert-ci.md`) : diagnostics, US techniques de la CI | 2026-11-07 15:47 : **WFS-250** 🔧 (CI de branche run 849 en cours ; 250 : 15:45, 82b1f20) ; **WFS-251** 🔧 ensuite (exception de l'utilisateur : `bench_realisme.cpp` + accroche `desktop_main.cpp` seulement) ; WFS-252 réservé (après WFS-250 et 251) ; WFS-237 ✅ |

## Info pour les développeurs (2026-11-06 23:47 UTC)
- Le rendu se vérifie **en local** avec lavapipe : `mesa-vulkan-drivers` et `glslc` s'installent dans le conteneur (paquets apt), puis `WFS_BANC=<groupe> build/wfs_bench` (même rendu logiciel que la CI ; testé par Claude, agent 2 sur WFS-197).
