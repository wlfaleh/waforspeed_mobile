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

**Dernière mise à jour** : 2026-11-07 14:33:14 UTC (po-scrum-master)

## Agents développeurs

| Agent (session) | Ticket en cours | Ce qu'il fait | Branche, dernier envoi (UTC) | État |
|---|---|---|---|---|
| agent arwa (Claude, session trafic) |  | WFS-218 attribué 14:19 et branche poussée 14:20 (2d3c20e), puis réservé à 14:25 à l'agent dev course dans `TACHES.md` (après WFS-242) ; WFS-222 🟣 | 218 : 14:20 (2d3c20e) ; 222 : fusionnée 13:53 (PR #195) | 🔴 conflit d'attribution WFS-218 à trancher par la session PO (règle des 10 h : branche poussée 14:20) |
| Claude, agent 1 (01UY1Vv2) | WFS-234 | P2 : carte de navigation façon GPS à la place de la mini-carte (attribué 12:53) | 234 : aucune branche poussée (attribué 12:53) ; 220 : PR #190 fusionnée | ⚠️ pas de branche 30 min après l'attribution (ticket gardé : règle des 10 h) |
| Claude, agent 2 (01UY1Vv5) | WFS-227 | P2 [RÉALISME 1.3a] ombres du soleil en cascades stabilisées, pénombre physique (attribué 11:44) | 227 : 14:05 (98327cb) ; 221 : fusionnée 11:30 (PR #193) | 🔧 en cours |
| Claude, agent 3 (01UY1Vv3) | WFS-239 | P1 [TECHNIQUE] banc qualité : voiture du trafic absente à 120 m en Basse ; recherche de la cause | 239 : 13:43 (323eeb1) ; 233 : fusionnée (PR #203, 4761db5) | 🔴 actif (P1) |
| Claude, agent 4 (011cpqZan) | WFS-240 | P3 [RÉALISME] enfants : poussettes, enfants en tribune, cheveux longs, cadence (attribué 13:58) ; WFS-236 🟣 | 240 : 14:23 (f0513fc) ; 236 : PR #199 (squash c2be79f) | 🔧 en cours |
| Claude, agent 5 (011cpqZaDiS2) |  | WFS-219 et 210 fusionnés | 219 : fusionnée 11:10 (PR #191) ; 210 : fusionnée (PR #185) | ✅ libre ; décisions Mustang et remplaçants attendues de l'utilisateur |
| Claude, session dev expert (0183bv2X) |  | libre (WFS-136, 122, 145, 187, 188 faits) | — | ✅ libre |
| Claude, session dev 5bx |  | WFS-216 fusionné (PR #196, squash f5340d7) | 216 : fusionnée 12:42 | ✅ libre |
| arwa-2 (Claude, session 01Us6uCu) | WFS-235 | P1 : vue Cockpit (bandes noires, regard qui suit la route, réglage « Regard cockpit », intérieurs réalistes), attribué 13:36 ; arbitrage RG-02 / RG-05 rendu 14:28 (option A, vraie géométrie) ; WFS-158 🟣 (PR #157 à #202) | 235 : 14:03 (9fb8cb9) ; 158 : 13:30 (PR #202 fusionnée) | 🔧 en cours |
| Claude, Agent 0 developer (session 01UY1Vv2SSLK) | WFS-161 | Kit d'architecture : ST-1 à ST-6 faits ; CI de branche run 822 avant fusion | 161 : 14:22 (bf45677) | 🔧 en cours |
| Claude, agent dev conduite (session 01UY1Vv1Hhvw) | WFS-241 | P1 : conduite réaliste, étape 1 : banc, nouveau modèle de la voiture du joueur, aide à la direction réelle (ouvert par le PO 14:25) | 241 : 14:26 (36776a0) | 🔴 en cours (P1) |
| Claude, agent dev course (session 01UY1Vv1dqtm) | WFS-242 | P1 : départ propre de la Course à 5 ; ensuite WFS-218 + 226 réservés (voir conflit WFS-218) (ouvert par le PO 14:25) | 242 : aucune branche poussée (attribué 14:25) | 🔴 en cours (P1) |

## Tickets prêts sans agent

| Ticket | Épic | Pourquoi il attend |
|---|---|---|
| WFS-226 (P1), WFS-224 (P1) | E4 (+ E3) | WFS-226 réservé à l'agent dev course avec WFS-218, après WFS-242 (même fichier `nuit.cpp`) ; WFS-224 au prochain libre après WFS-239 |
| WFS-230 (P1), WFS-214 (P2) | E13 | épic E13 tenue par WFS-227 (agent 2) |
| WFS-232 (P2), WFS-215 (P3) | E3 + E10, E10 | épics tenues par WFS-235 (arwa-2) |
| WFS-243 (P2) | E3 | après WFS-241 (agent dev conduite) et WFS-242 (agent dev course), en cours (mêmes fichiers) |
| WFS-244 (P3) | E9 | épic E9 libre : attribuable tout de suite |
| WFS-245 (P3) | E5 | épic E5 tenue par WFS-234 (agent 1) |
| Claude, agent dev perf (session 01UY1Vv56Dje) | WFS-246 | Performance, 2e passage (WFS-222 refusé par l'utilisateur) | 246 : branche `feature/waforspeed-246` poussée, mesure de départ au banc `perf` | 🔧 en cours |

## Agents de pilotage

| Agent | Rôle | Dernière action |
|---|---|---|
| expert-conduite | développeur expert en physique de la voiture (skill `expert-conduite`) : chantier « Conduite réaliste » | créé le 2026-11-07 |
| po-scrum-master | attribue les tickets, écrit les règles de gestion, tient `TACHES.md`, le journal et ce fichier | 2026-11-07 14:28 : arbitrage WFS-235 (question d'arwa-2) : option A, vraie géométrie ; RG-05 remplacé (fenêtre de la rue 0 %, bande de la rue ≤ vraie voiture + 2 points) ; conflit d'attribution WFS-218 signalé |
| QA-arwa (qa-lead, Claude session 01Us6uCubd) | recette de nuit des tickets de `qa/Check.md`, verdicts et anomalies ; campagne VABF sur émulateur | recette du 2026-11-07 : WFS-211, 212, 213 VALIDÉS, WFS-208 PASSABLE (ANO-112) |
| expert-realisme-3d | expert du graphisme et du réalisme 3D (WFS-148 à 167) : **consultant, pas développeur** ; conseille les développeurs, ne tient pas de ticket de code | à consulter par arwa-2 pour WFS-235 (RG-04, intérieurs) |
| smart-brainstorming | propose des idées dans `recommandations-smart-brain.md`, ne décide rien, ne parle qu'avec l'utilisateur | recommandations R01, R02 ; avis sur WFS-236 et 237 |
| agent expert ci (session 01UY1Vv4) | expert de la pipeline CI (`.claude/agents/expert-ci.md`) : diagnostics, US techniques de la CI | 2026-11-07 13:27:37 : WFS-238 🟣 fusionné (PR #201) ; WFS-237 🟣 (mesure avant / après prévue à 15:52 UTC) |

## Info pour les développeurs (2026-11-06 23:47 UTC)
- Le rendu se vérifie **en local** avec lavapipe : `mesa-vulkan-drivers` et `glslc` s'installent dans le conteneur (paquets apt), puis `WFS_BANC=<groupe> build/wfs_bench` (même rendu logiciel que la CI ; testé par Claude, agent 2 sur WFS-197).
