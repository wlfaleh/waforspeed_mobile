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

**Dernière mise à jour** : 2026-11-07 19:50:39 UTC (po-scrum-master)

## Agents développeurs

| Agent (session) | Ticket en cours | Ce qu'il fait | Branche, dernier envoi (UTC) | État |
|---|---|---|---|---|
| agent arwa (Claude, session trafic) | WFS-274 (rien à coder), WFS-270, WFS-257, WFS-224 | **WFS-274 : rien à faire sur les phares** (19:48 : la cause du rouge est WFS-227, **pas** ses phares ; corrigée par arwa-5 sous WFS-267 ; priorité absolue levée ; le ticket reste à lui, règle des 10 h : à lui de le clore « ↪ Regroupé avec WFS-267 ») ; reprend P1 WFS-270 (correction de son WFS-226 REFUSÉ en recette : ANO-147 voiture du joueur noire la nuit + ANO-148 ; E10, exception PO à côté de WFS-235), attribué 18:00 ; P1 WFS-257 personnes qui apparaissent / disparaissent (E11), attribué 16:35 ; **en parallèle** (demandes de l'utilisateur) P1 WFS-224 fumée des pneus + lignes du nitro sous le HUD 🟣 fusionné (PR #228) ; WFS-244 🟣 fusionné ; WFS-226 fusionné, QA REFUSÉ | 270 : 18:01 (27d742b, ouverture de la branche) ; 257 : 18:23 (a8154a3, synchronisation) ; 224 : fusionnée 19:51 (PR #228, squash 2a3f341) ; 244 : fusionnée 18:05 (PR #218, squash 4a0fbb9) ; 274 : aucune branche | 🔴 en cours (P1 + P1 + P1) |
| Claude, agent 1 (01UY1Vv2) | — | WFS-230 fusionnée (PR #221, squash b7825cf), 🟡 à valider par l'utilisateur ; libre, attend l'accord de l'utilisateur avant un nouveau ticket | 230 : PR #221 fusionnée ; 234 : PR #209 | 🟢 libre |
| Claude, agent 2 (01UY1Vv5) | — (WFS-227 fusionné) | libre (ne prend pas de ticket sans demande de l'utilisateur) | 227 : fusionnée 15:58 (PR #210) ; 221 : fusionnée (PR #193) | 🟢 libre |
| Claude, agent 3 (01UY1Vv3) | WFS-239 | P1 [TECHNIQUE] banc qualité : voiture du trafic absente à 120 m en Basse ; recherche de la cause | 239 : 13:43 (323eeb1, commit de début seul) ; 233 : fusionnée (PR #203, 4761db5) | 🔴 ⚠️ P1 : rien poussé depuis 13:43 (6 h 05 à 19:48) ; relance, pas de retrait avant 10 h (23:43) |
| Claude, agent 4 (011cpqZan) |  | Stoppé par l'utilisateur (15:06 UTC) : **plus aucun ticket à lui donner** ; WFS-224 repris, puis donné à agent arwa (17:19 UTC) | 224 : 14:49 (533e360, commit de début seul, sans code) ; 240 : fusionnée (PR #205) | ⛔ stoppé par l'utilisateur |
| Claude, agent 5 (011cpqZaDiS2) | — | libre (demande son ticket suivant au PO ; WFS-273 lui est proposé en premier) | 268 : fusionnée par confiance le 2026-11-07 19:34 UTC (Ville et Corniche préparées au chargement, Campagne et Grand Prix en repli, décision du PO), 🟡 ; 248 : fusionnée (PR #208, 🟡) | 🟢 libre |
| Claude, agent dev X (0183bv2X, ex-« session dev expert ») | WFS-246 | P1 [TECHNIQUE] performance, 2e passage, **recentré à 17:13** : mesure sur le téléphone (RG-06), faces arrière (levier 1 poussé), résolution sans `resize()` en course (RG-07) ; suite WFS-262 à 265 (E4) proposée à lui ensuite | 246 : 18:27 (8f95756), CI de branche 863 | 🔴 en cours (P1) |
| Claude, agent dev Y |  | WFS-216 fusionné (PR #196, squash f5340d7) | 216 : fusionnée 12:42 | 🟢 libre : pause de l'utilisateur finie à 18:00 UTC ; ticket seulement à sa demande |
| arwa-2 (Claude, session 01Us6uCu) | WFS-235, WFS-245 | P1 WFS-235 vue Cockpit (E3 + E10) : rétroviseur et haut du pare-brise par modèle (avis d'expert, critère 2), mesure exacte de la part cachée (intérieur en couleur clé) ; banc des 20 voitures en cours, puis tableau, parcours et fusion (🟡) ; **en parallèle** (demande de l'utilisateur, 17:07) P3 WFS-245 étiquettes du HUD lisibles (E5) : tailles arbitrées par le PO (17:30) codées, captures après regardées ; reste avant / après, tests et parcours | 235 : 18:30 (2fe8df3) ; 245 : 18:30 (b2db78c) | 🔴 en cours (P1 + P3) |
| arwa-4 (Claude, session 01Us6uCw) |  | WFS-272 fusionné (PR #227, squash 9fe1b19, 19:49 UTC) ; WFS-261 fusionné (PR #219) ; demande son ticket suivant au PO | 272 : fusionnée 19:49 (PR #227) | 🟢 libre |
| arwa-3 (Claude, session 01Us6uCt) | WFS-243 | domaine **courses et modes** : P2 WFS-243 adversaires visibles et classement juste en Course à 5, îlot du rond-point (vérification), trafic au giratoire (ANO-144), un seul changement de voie par dépassement (ANO-150, signalée par lui) ; attribué 19:45 à sa demande (n° 8 du bandeau) ; exception PO dans E3 : `course5*`, `course.*`, `course_system.*`, `traffic.*` + accroche ≤ 10 lignes `renderer.*` (cause d'ANO-117 : voitures coupées à 150 / 200 m ; accord du PO à 19:50), **pas** `player.*` (WFS-258) ; RG-04 îlot déjà solide ; 🟡 après fusion ; WFS-242 fusionné 🟡 | 243 : branche à pousser (attribué 19:45) ; 242 : fusionnée 18:45 (PR #222, 71fed22) | 🔧 en cours (P2) |
| Claude, Agent 0 developer (session 01UY1Vv2SSLK) | WFS-249 | P1 Campagne de nuit, 2e passage (WFS-225 refusé, ANO-128) : nuit lisible au clair de lune (arbitrage A + B du PO, 19:41 : EV −3,5, exposition sur lune + ciel en Campagne) (E2 + E13 ; **pas** `eclairage_banc.cpp` ni `environnement.cpp` : WFS-267 d'arwa-5) ; attribué 19:32 à sa demande | 249 : 19:43 (f1bf182, EV −3,5 et exposition de la Campagne) | 🔴 en cours (P1) |
| arwa-5 (Claude, session 01Us6uCwsJ63) | WFS-267 | P1 [TECHNIQUE] CI de main rouge depuis #852 (banc `realisme`, `eclairage_banc.cpp:560`, sortie du tunnel sans adaptation ; même panne que WFS-274) : **cause nommée par bissection : WFS-227** (ombres en cascades, pas les phares) ; correction en cours : accroche ≤ 10 lignes dans `environnement.cpp` validée par le PO (19:48), à annoncer au journal ; aucun seuil du banc changé, WFS-227 non annulé | 267 : 17:37 (9f42617, branche créée) ; bissection en local (journal 19:42) | 🔴 ⚠️ P1 : rien poussé depuis 17:37 (2 h 11) ; travail en local au journal |
| Claude, agent dev conduite (session 01UY1Vv1Hhvw) | WFS-258 | P1 conduite 2e passage : **d'abord remettre l'ancienne conduite** (`g_conduiteMonde = false`), puis corriger les points de l'utilisateur ; **+ ANO-138** (cas de banc 150-230 km/h, direction moins vive à haute vitesse), **ANO-140** (dérive ≤ 5°, vitesse ≥ 0 sur le bas-côté), **ANO-139** (vérification ≤ 2 cm en virage), rangées à 17:52 ; WFS-259 ensuite ; sujets de conduite réservés à lui (exception de l'utilisateur) | 256 : fusionné #216, 🟡 ; 258 : RG-00 fusionné #217 (04a1628), suite sur `feature/waforspeed-258` (17:35) | 🔴 ⚠️ P1 : rien poussé depuis 17:35 (2 h 10) ; relance, pas de retrait avant 10 h |

## Tickets prêts sans agent

| Ticket | Épic | Pourquoi il attend |
|---|---|---|
| WFS-269 (P1) | E13 + E4 | prêt depuis la fusion de WFS-268 (🟡) ; E13 tenue par WFS-249 (Agent 0) et WFS-274 (agent arwa) |
| WFS-262 à 265 (P1) | E4 | épic E4 tenue par WFS-246 (agent dev X) ; à la suite, une par une |
| WFS-214 (P2) | E13 | épic E13 tenue par WFS-249 (Agent 0) |
| WFS-232 (P2), WFS-215 (P3) | E3 + E10, E10 | épics tenues par WFS-235 (arwa-2) |
| WFS-266 (P1) | E11 | ANO-134 bloquante (WFS-210 REFUSÉ) : correction prioritaire, **premier ticket de E11** dès que WFS-257 (agent arwa) est fusionné ; au premier développeur qui le demande ensuite |
| WFS-271 (P2) | E2 | épic E2 tenue par WFS-249 (Agent 0) ; raccords des ronds-points (ANO-142, 143, 144 piétons) ; premier ticket permis du bandeau à 18:10, pas donné à arwa-4 (domaine son) ; au prochain développeur qui le demande |

## Agents de pilotage

| Agent | Rôle | Dernière action |
|---|---|---|
| expert-conduite | développeur expert en physique de la voiture (skill `expert-conduite`) : chantier « Conduite réaliste » | créé le 2026-11-07 |
| po-scrum-master | attribue les tickets, écrit les règles de gestion, tient `TACHES.md`, le journal et ce fichier | 2026-11-07 19:48 : cause du rouge corrigée (WFS-227, pas les phares ; vérifiée) ; WFS-274 laissé à l'agent arwa (règle des 10 h), rien à coder pour lui ; accroche `environnement.cpp` validée pour arwa-5 (WFS-267) ; 19:45 : WFS-243 attribué à arwa-3, ANO-150 rangée, WFS-269 prêt ; 19:50 : accord à arwa-3 pour une accroche ≤ 10 lignes dans `renderer.*` (WFS-243, ANO-117), hors des lignes de WFS-246 |
| QA-arwa (qa-lead, Claude session 01Us6uCubd) | recette de nuit des tickets de `qa/Check.md`, verdicts et anomalies ; campagne VABF sur émulateur | recette du 2026-11-07 soir terminée (26 tickets ; lot 3 : WFS-218 VALIDÉ, WFS-158 et 219 PASSABLES, WFS-226 REFUSÉ) |
| expert-realisme-3d | expert du graphisme et du réalisme 3D (WFS-148 à 167) : **consultant, pas développeur** ; conseille les développeurs, ne tient pas de ticket de code | à consulter par arwa-2 pour WFS-235 (RG-04, intérieurs) |
| smart-brainstorming | propose des idées dans `recommandations-smart-brain.md`, ne décide rien, ne parle qu'avec l'utilisateur | 2026-11-07 : constats et recommandations performance (ANO-125) transmis au PO sur ordre de l'utilisateur |
| agent expert ci (session 01UY1Vv4) | expert de la pipeline CI (`.claude/agents/expert-ci.md`) : diagnostics, US techniques de la CI | 2026-11-07 19:25 : WFS-251 🟣 fusionné (PR #224) ; WFS-252 🔧 CI de branche run 868 (5ca65d6) ; diagnostic du rouge arrêté (cause WFS-227, corrigée sous WFS-267) |

## Info pour les développeurs (2026-11-06 23:47 UTC)
- Le rendu se vérifie **en local** avec lavapipe : `mesa-vulkan-drivers` et `glslc` s'installent dans le conteneur (paquets apt), puis `WFS_BANC=<groupe> build/wfs_bench` (même rendu logiciel que la CI ; testé par Claude, agent 2 sur WFS-197).
