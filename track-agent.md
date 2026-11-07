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

**Dernière mise à jour** : 2026-11-07 18:33:02 UTC (po-scrum-master)

## Agents développeurs

| Agent (session) | Ticket en cours | Ce qu'il fait | Branche, dernier envoi (UTC) | État |
|---|---|---|---|---|
| agent arwa (Claude, session trafic) | WFS-270, WFS-257, WFS-244, WFS-224 | **P1 WFS-270 en premier** (correction de son WFS-226 REFUSÉ en recette : ANO-147 voiture du joueur noire la nuit + ANO-148 preuves sens inverse / flaque lointaine ; E10, exception PO à côté de WFS-235), attribué 18:00 ; P1 WFS-257 personnes qui apparaissent / disparaissent (E11), attribué 16:35 ; **en parallèle** (demandes de l'utilisateur) P3 WFS-244 plaques à bords nets sur la chaussée (E9) 🟣 fusionné, et P1 WFS-224 fumée des pneus + lignes du nitro sous le HUD (E4, `effets_*` seul), attribué 17:19 ; WFS-226 fusionné, QA REFUSÉ | 270 : 18:01 (27d742b, ouverture de la branche) ; 257 : 17:33 (3d917ff, apparition naturelle, fondus 0,5 s, banc des apparitions) ; 244 : fusionnée 18:05 (PR #218, squash 4a0fbb9, 🟣 CI de main à suivre) ; 224 : 17:47 (4467370, volutes douces) ; 226 : fusionnée 16:40 (PR #214) | 🔴 en cours (P1 + P1 + P3 + P1) |
| Claude, agent 1 (01UY1Vv2) | — | WFS-230 fusionnée (PR #221, squash b7825cf), 🟡 à valider par l'utilisateur ; libre, attend l'accord de l'utilisateur avant un nouveau ticket | 230 : PR #221 fusionnée ; 234 : PR #209 | 🟢 libre |
| Claude, agent 2 (01UY1Vv5) | — (WFS-227 fusionné) | libre (ne prend pas de ticket sans demande de l'utilisateur) | 227 : fusionnée 15:58 (PR #210) ; 221 : fusionnée (PR #193) | 🟢 libre |
| Claude, agent 3 (01UY1Vv3) | WFS-239 | P1 [TECHNIQUE] banc qualité : voiture du trafic absente à 120 m en Basse ; recherche de la cause | 239 : 13:43 (323eeb1, commit de début seul) ; 233 : fusionnée (PR #203, 4761db5) | 🔴 ⚠️ P1 : rien poussé depuis 13:43 (4 h 30 à 18:13) ; relance, pas de retrait avant 10 h |
| Claude, agent 4 (011cpqZan) |  | Stoppé par l'utilisateur (15:06 UTC) : **plus aucun ticket à lui donner** ; WFS-224 repris, puis donné à agent arwa (17:19 UTC) | 224 : 14:49 (533e360, commit de début seul, sans code) ; 240 : fusionnée (PR #205) | ⛔ stoppé par l'utilisateur |
| Claude, agent 5 (011cpqZaDiS2) | — | libre (demande son ticket suivant au PO ; WFS-273 lui est proposé en premier) | 268 : fusionnée par confiance le 2026-11-07 19:34 UTC (Ville et Corniche préparées au chargement, Campagne et Grand Prix en repli, décision du PO), 🟡 ; 248 : fusionnée (PR #208, 🟡) | 🟢 libre |
| Claude, agent dev X (0183bv2X, ex-« session dev expert ») | WFS-246 | P1 [TECHNIQUE] performance, 2e passage, **recentré à 17:13** : mesure sur le téléphone (RG-06), faces arrière (levier 1 poussé), résolution sans `resize()` en course (RG-07) ; suite WFS-262 à 265 (E4) proposée à lui ensuite | 246 : 18:27 (8f95756), CI de branche 863 | 🔴 en cours (P1) |
| Claude, agent dev Y |  | WFS-216 fusionné (PR #196, squash f5340d7) | 216 : fusionnée 12:42 | 🟢 libre : pause de l'utilisateur finie à 18:00 UTC ; ticket seulement à sa demande |
| arwa-2 (Claude, session 01Us6uCu) | WFS-235, WFS-245 | P1 WFS-235 vue Cockpit (E3 + E10) : rétroviseur et haut du pare-brise par modèle (avis d'expert, critère 2), mesure exacte de la part cachée (intérieur en couleur clé) ; banc des 20 voitures en cours, puis tableau, parcours et fusion (🟡) ; **en parallèle** (demande de l'utilisateur, 17:07) P3 WFS-245 étiquettes du HUD lisibles (E5) : tailles arbitrées par le PO (17:30) codées, captures après regardées ; reste avant / après, tests et parcours | 235 : 18:30 (2fe8df3) ; 245 : 18:30 (b2db78c) | 🔴 en cours (P1 + P3) |
| arwa-4 (Claude, session 01Us6uCw) | WFS-272 | domaine **son** : P2 [RÉALISME] son des autres voitures (E6) : adversaires de la Course à 5 et du Duel, trafic et police entendus à leur place (gauche / droite, distance, Doppler) ; créé et attribué 18:10 à sa demande (règles « une session = un domaine » et « jamais à court de tickets ») ; WFS-261 fusionné | 272 : 18:16 (1a03077, commit de début) ; 261 : fusionnée 18:06 (PR #219) | 🔧 en cours (P2) |
| arwa-3 (Claude, session 01Us6uCt) |  | WFS-242 fusionné (PR #222, squash 71fed22) : grille de départ en quinconce derrière la ligne, place du joueur tirée au sort, chrono à 0 jusqu'au « GO », commandes au premier « 3 », Duel à sa place ; 🟡 à valider par l'utilisateur ; demande son ticket suivant au PO | 242 : fusionnée 18:45 (PR #222, 71fed22) | 🟢 libre |
| Claude, Agent 0 developer (session 01UY1Vv2SSLK) | WFS-247 | P1 [TECHNIQUE] fluidité, 2e passage (WFS-221 refusé, ANO-126) : la voiture saute encore devant la caméra en vue Proche / Poursuite ; + roues avant d'après l'angle appliqué ; **+ ANO-139** (17:52 : banc qui compte les virages, aller-retour ≤ 2 cm, cadences 10, 15, 20 img/s) | 247 : 16:15 (791997c, roues avant) ; 161 : fusionnée 15:20 | 🔴 ⚠️ P1 : rien poussé depuis 16:15 (≈ 2 h à 18:13) ; relance, pas de retrait avant 10 h |
| arwa-5 (Claude, session 01Us6uCwsJ63) | WFS-267 | P1 [TECHNIQUE] CI de main rouge #852 (banc `realisme`, étalonnage des couleurs non mesurable) : RG-00 d'abord (job `realisme` du run #859 en cours), sinon cause et correction sans affaiblir le banc ; coordination avec l'agent expert ci (WFS-251, `bench_realisme.cpp`) ; attribué 17:35 à sa demande (nouveau développeur, **pas** Claude, agent 5 ; petites tâches demandées par l'utilisateur) | 267 : 17:37 (9f42617, branche créée) | 🔴 en cours (P1) |
| Claude, agent dev conduite (session 01UY1Vv1Hhvw) | WFS-258 | P1 conduite 2e passage : **d'abord remettre l'ancienne conduite** (`g_conduiteMonde = false`), puis corriger les points de l'utilisateur ; **+ ANO-138** (cas de banc 150-230 km/h, direction moins vive à haute vitesse), **ANO-140** (dérive ≤ 5°, vitesse ≥ 0 sur le bas-côté), **ANO-139** (vérification ≤ 2 cm en virage), rangées à 17:52 ; WFS-259 ensuite ; sujets de conduite réservés à lui (exception de l'utilisateur) | 256 : fusionné #216, 🟡 ; 258 : RG-00 fusionné #217 (04a1628), suite sur `feature/waforspeed-258` (17:35) | 🔴 en cours (P1) |

## Tickets prêts sans agent

| Ticket | Épic | Pourquoi il attend |
|---|---|---|
| WFS-269 (P1) | E13 + E4 | après WFS-268 ; E13 tenue par WFS-230 (agent 1) |
| WFS-262 à 265 (P1) | E4 | épic E4 tenue par WFS-246 (agent dev X) ; à la suite, une par une |
| WFS-249 (P1) | E2 + E13 | E13 tenue par WFS-230 (agent 1) ; WFS-248 🟡 ; pas à la session dev 5bx |
| WFS-214 (P2) | E13 | épic E13 tenue par WFS-230 (agent 1) |
| WFS-232 (P2), WFS-215 (P3) | E3 + E10, E10 | épics tenues par WFS-235 (arwa-2) |
| WFS-243 (P2) | E3 | après WFS-242 (arwa-3) ; épic E3 tenue par WFS-258, WFS-235, WFS-247 et WFS-242 |
| WFS-266 (P1) | E11 | ANO-134 bloquante (WFS-210 REFUSÉ) : correction prioritaire, **premier ticket de E11** dès que WFS-257 (agent arwa) est fusionné ; au premier développeur qui le demande ensuite |
| WFS-271 (P2) | E2 | raccords des ronds-points (ANO-142, 143, 144 piétons) ; premier ticket permis du bandeau à 18:10, pas donné à arwa-4 (domaine son) ; au prochain développeur qui le demande |
| WFS-252 (P2) | E7 | en attente de WFS-251 et WFS-250 ; réservé à l'agent expert ci ; + ANO-135, ANO-136 et durée de l'artefact `compilation` (rangés à 17:43) |

## Agents de pilotage

| Agent | Rôle | Dernière action |
|---|---|---|
| expert-conduite | développeur expert en physique de la voiture (skill `expert-conduite`) : chantier « Conduite réaliste » | créé le 2026-11-07 |
| po-scrum-master | attribue les tickets, écrit les règles de gestion, tient `TACHES.md`, le journal et ce fichier | 2026-11-07 18:13 : WFS-272 (son des autres voitures, E6) créé et attribué à arwa-4 à sa demande (domaine son ; WFS-271 du bandeau = autre domaine) ; suivi des branches mis à jour |
| QA-arwa (qa-lead, Claude session 01Us6uCubd) | recette de nuit des tickets de `qa/Check.md`, verdicts et anomalies ; campagne VABF sur émulateur | recette du 2026-11-07 soir terminée (26 tickets ; lot 3 : WFS-218 VALIDÉ, WFS-158 et 219 PASSABLES, WFS-226 REFUSÉ) |
| expert-realisme-3d | expert du graphisme et du réalisme 3D (WFS-148 à 167) : **consultant, pas développeur** ; conseille les développeurs, ne tient pas de ticket de code | à consulter par arwa-2 pour WFS-235 (RG-04, intérieurs) |
| smart-brainstorming | propose des idées dans `recommandations-smart-brain.md`, ne décide rien, ne parle qu'avec l'utilisateur | 2026-11-07 : constats et recommandations performance (ANO-125) transmis au PO sur ordre de l'utilisateur |
| agent expert ci (session 01UY1Vv4) | expert de la pipeline CI (`.claude/agents/expert-ci.md`) : diagnostics, US techniques de la CI | 2026-11-07 16:52:32 : WFS-250 🟣 fusionné (PR #215) ; WFS-251 🔧 (dernier envoi 18:01 UTC, b75b75e : parties rééquilibrées) ; WFS-252 ensuite ; **à prévenir par arwa-5** avant toute retouche de `bench_realisme.cpp` (WFS-267) |

## Info pour les développeurs (2026-11-06 23:47 UTC)
- Le rendu se vérifie **en local** avec lavapipe : `mesa-vulkan-drivers` et `glslc` s'installent dans le conteneur (paquets apt), puis `WFS_BANC=<groupe> build/wfs_bench` (même rendu logiciel que la CI ; testé par Claude, agent 2 sur WFS-197).
