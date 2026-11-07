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

**Dernière mise à jour** : 2026-11-07 21:27:36 UTC (po-scrum-master)

## Agents développeurs

| Agent (session) | Ticket en cours | Ce qu'il fait | Branche, dernier envoi (UTC) | État |
|---|---|---|---|---|
| agent arwa (Claude, session trafic) | WFS-274 (rien à coder), WFS-270 | **WFS-274 : rien à faire sur les phares** (19:48 : la cause du rouge est WFS-227, **pas** ses phares ; corrigée par arwa-5 sous WFS-267 ; priorité absolue levée ; le ticket reste à lui, règle des 10 h : à lui de le clore « ↪ Regroupé avec WFS-267 ») ; P1 WFS-270 🟣 fusionné (PR #231, squash b21b9c8 ; 🟡 à valider par l'utilisateur ; ANO-147 / 148 renvoyées au QA) ; P1 WFS-257 personnes qui apparaissent / disparaissent (E11), attribué 16:35 ; **en parallèle** (demandes de l'utilisateur) P1 WFS-224 fumée des pneus + lignes du nitro sous le HUD 🟣 fusionné (PR #228) ; WFS-244 🟣 fusionné ; WFS-226 fusionné, QA REFUSÉ | 270 : fusionnée 20:12 (PR #231, squash b21b9c8) ; 257 : fusionnée 20:00 (PR #220, squash ebe798e) ; 224 : fusionnée 19:51 (PR #228, squash 2a3f341) ; 244 : fusionnée 18:05 (PR #218, squash 4a0fbb9) ; 274 : aucune branche | 🔴 en cours (P1 + P1 + P1) |
| Claude, agent 1 (01UY1Vv2) | — | WFS-230 fusionnée (PR #221, squash b7825cf), 🟡 à valider par l'utilisateur ; libre, attend l'accord de l'utilisateur avant un nouveau ticket | 230 : PR #221 fusionnée ; 234 : PR #209 | 🟢 libre |
| Claude, agent 2 (01UY1Vv5) | — (WFS-227 fusionné) | libre (ne prend pas de ticket sans demande de l'utilisateur) | 227 : fusionnée 15:58 (PR #210) ; 221 : fusionnée (PR #193) | 🟢 libre |
| Claude, agent 3 (01UY1Vv3) | WFS-239 | P1 [TECHNIQUE] banc qualité : voiture du trafic absente à 120 m en Basse ; recherche de la cause | 239 : 13:43 (323eeb1, commit de début seul) ; 233 : fusionnée (PR #203, 4761db5) | 🔴 ⚠️ P1 : rien poussé depuis 13:43 (7 h 44 à 21:27) ; relance, pas de retrait avant 10 h (23:43) |
| Claude, agent 4 (011cpqZan) |  | Stoppé par l'utilisateur (15:06 UTC) : **plus aucun ticket à lui donner** ; WFS-224 repris, puis donné à agent arwa (17:19 UTC) | 224 : 14:49 (533e360, commit de début seul, sans code) ; 240 : fusionnée (PR #205) | ⛔ stoppé par l'utilisateur |
| Claude, agent 5 (011cpqZaDiS2) | — | libre (demande son ticket suivant au PO ; WFS-273 lui est proposé en premier) | 268 : fusionnée par confiance le 2026-11-07 19:34 UTC (Ville et Corniche préparées au chargement, Campagne et Grand Prix en repli, décision du PO), 🟡 ; 248 : fusionnée (PR #208, 🟡) | 🟢 libre |
| Claude, agent dev X (0183bv2X, ex-« session dev expert ») |  | WFS-246 fusionné (PR #229), 🟡 à valider par l'utilisateur ; libre ; E4 libre : WFS-273 (n° 0 du bandeau) puis WFS-262 à 265, au premier qui demande (WFS-262 proposé d'abord à lui) | 246 : fusionné #229 (ad94d9d), 🟡 À valider (utilisateur) | 🟢 libre |
| Claude, agent dev Y |  | WFS-216 fusionné (PR #196, squash f5340d7) | 216 : fusionnée 12:42 | 🟢 libre : pause de l'utilisateur finie à 18:00 UTC ; ticket seulement à sa demande |
| arwa-2 (Claude, session 01Us6uCu) | WFS-245 | P3 WFS-245 étiquettes du HUD lisibles (E5, en parallèle, demande de l'utilisateur) : codé, 59 tests CPU et parcours 0 écart, captures regardées ; fusion par confiance en cours (🟡). WFS-235 (P1) **fusionné** PR #236 (squash 66fdd3d), 🟡 À valider (utilisateur) | 235 : 21:04 (fusionné, #236) ; 245 : 21:04 (a44542b) | 🔴 en cours (WFS-245) |
| arwa-4 (Claude, session 01Us6uCw) | WFS-278 | domaine **son** : P2 WFS-278 acoustique des lieux (2) : façades du casino, du centre commercial et des garages (WFS-176), sons masqués à la Corniche, à la Campagne et au Grand Prix, diffraction multiple et absorption des parois sourcées (limites de WFS-275) ; attribué 20:57 à sa demande ; avant : WFS-275 🟣 (PR #237, squash b009c48), WFS-272 🟣 (PR #227), WFS-261 🟣 (PR #219) | 278 : 20:59 (59ed778) ; 275 : fusionnée 20:54 (PR #237) | 🔧 en cours (P2) |
| arwa-3 (Claude, session 01Us6uCt) | WFS-232 | domaine **courses, modes et trafic** : P2 WFS-232 (1/2) motos et scooters dans le trafic, pilotes casqués, inclinaison en virage, feux la nuit (WFS-232 découpé à 21:28 : garés et faufilement → WFS-280) ; attribué 21:30 à sa demande ; exception PO : `traffic.*`, `traffic_system.*`, `deux_roues*`, `assets/cars/deux_roues/**`, **pas** `player.*` / `conduite_*` (WFS-258) ; 🟡 après fusion ; avant : WFS-243 🟡 (PR #239), WFS-242 🟡 (PR #222) | 232 : 21:31 (2da6fe7, branche créée et poussée) ; 243 : fusionnée 21:20 (PR #239) | 🔧 en cours (P2) |
| Claude, Agent 0 developer (session 01UY1Vv2SSLK) | WFS-276 | P1 WFS-276 nuit, tonemapping local (E13) ; WFS-249 🟡 (PR #235, squash d3c1c68) | 276 : 20:57 (b10b61e) ; 249 : fusionnée 20:40 (d3c1c68) | 🔴 en cours (P1) |
| arwa-5 (Claude, session 01Us6uCwsJ63) | WFS-267 | P1 [TECHNIQUE] CI de main rouge depuis #852 (banc `realisme`, `eclairage_banc.cpp:560`, sortie du tunnel sans adaptation ; même panne que WFS-274) : **cause nommée par bissection : WFS-227** (ombres en cascades, pas les phares) ; correctif : accroche de 6 lignes dans `environnement.cpp` (une face de sonde attend que la cascade 0 couvre son point, 120 images au plus) + RG-05 (messages d'erreur du banc) ; aucun seuil changé, WFS-227 non annulé ; tunnel local OK ; PR #232 ; CI de branche #876 | 267 : 20:04 (d25b5b7, correctif + RG-05) | 🔴 en cours (P1) |
| Claude, agent dev conduite (session 01UY1Vv1Hhvw) | WFS-258 | P1 conduite 2e passage : **d'abord remettre l'ancienne conduite** (`g_conduiteMonde = false`), puis corriger les points de l'utilisateur ; **+ ANO-138** (cas de banc 150-230 km/h, direction moins vive à haute vitesse), **ANO-140** (dérive ≤ 5°, vitesse ≥ 0 sur le bas-côté), **ANO-139** (vérification ≤ 2 cm en virage), rangées à 17:52 ; WFS-259 ensuite ; sujets de conduite réservés à lui (exception de l'utilisateur) | 256 : fusionné #216, 🟡 ; 258 : RG-00 fusionné #217 (04a1628), suite sur `feature/waforspeed-258` (17:35) | 🔴 ⚠️ P1 : rien poussé depuis 17:35 (3 h 52 à 21:27) ; relance, pas de retrait avant 10 h (03:35) |

## Tickets prêts sans agent

| Ticket | Épic | Pourquoi il attend |
|---|---|---|
| WFS-269 (P1) | E13 + E4 | prêt depuis la fusion de WFS-268 (🟡) ; E13 tenue par WFS-276 (Agent 0) |
| WFS-273 (P1) | E4 | prêt depuis la fusion de WFS-246 (19:51, 🟡) ; n° 0 du bandeau : premier ticket de E4, au premier développeur qui le demande (agent 5 d'abord), **pas** agent arwa |
| WFS-262 à 265 (P1) | E4 | après WFS-273 (un ticket actif par épic), une par une ; WFS-262 proposé d'abord à agent dev X |
| WFS-214 (P2) | E13 | épic E13 tenue par WFS-276 (Agent 0) |
| WFS-215 (P3) | E10 | E10 tenue en partie par WFS-232 (arwa-3 : nouveau dossier `assets/cars/deux_roues/` et `convert_car.mjs` seuls) ; domaine voitures |
| WFS-266 (P1) | E11 | ANO-134 bloquante (WFS-210 REFUSÉ) : correction prioritaire ; E11 libre (WFS-257 🟣) : au premier développeur qui le demande (domaine personnages) |
| WFS-271 (P2) | E2 | E2 libre (WFS-249 🟡) ; raccords des ronds-points (ANO-142, 143, 144 piétons) ; pas donné à arwa-4 (domaine son) ; au prochain développeur qui le demande (domaine décor) |
| WFS-280 (P2) | E3 + E2 | en attente de WFS-232 (modèles et pilotes des deux-roues) |

## Agents de pilotage

| Agent | Rôle | Dernière action |
|---|---|---|
| expert-conduite | développeur expert en physique de la voiture (skill `expert-conduite`) : chantier « Conduite réaliste » | créé le 2026-11-07 |
| po-scrum-master | attribue les tickets, écrit les règles de gestion, tient `TACHES.md`, le journal et ce fichier | 2026-11-07 21:27 : WFS-232 découpé (1/2 trafic, 2/2 garés et faufilement = WFS-280 créé) et attribué à arwa-3 à sa demande ; cédez-le-passage de WFS-243 sans objet (décision) ; 2026-11-07 20:57 : WFS-278 (son, acoustique des lieux 2, tiré des limites de WFS-275) créé et attribué à arwa-4 à sa demande (aucun ticket prêt du son) ; 2026-11-07 19:56 : WFS-275 (son, acoustique des lieux) créé et attribué à arwa-4 à sa demande ; WFS-273 prêt (E4 libérée par la fusion de WFS-246), WFS-262 après lui ; 2026-11-07 19:48 : cause du rouge corrigée (WFS-227, pas les phares ; vérifiée) ; WFS-274 laissé à l'agent arwa (règle des 10 h), rien à coder pour lui ; accroche `environnement.cpp` validée pour arwa-5 (WFS-267) ; 19:45 : WFS-243 attribué à arwa-3, ANO-150 rangée, WFS-269 prêt ; 19:50 : accord à arwa-3 pour une accroche ≤ 10 lignes dans `renderer.*` (WFS-243, ANO-117), hors des lignes de WFS-246 |
| QA-arwa (qa-lead, Claude session 01Us6uCubd) | recette de nuit des tickets de `qa/Check.md`, verdicts et anomalies ; campagne VABF sur émulateur | recette du 2026-11-07 soir terminée (26 tickets ; lot 3 : WFS-218 VALIDÉ, WFS-158 et 219 PASSABLES, WFS-226 REFUSÉ) |
| expert-realisme-3d | expert du graphisme et du réalisme 3D (WFS-148 à 167) : **consultant, pas développeur** ; conseille les développeurs, ne tient pas de ticket de code | à consulter par arwa-2 pour WFS-235 (RG-04, intérieurs) |
| smart-brainstorming | propose des idées dans `recommandations-smart-brain.md`, ne décide rien, ne parle qu'avec l'utilisateur | 2026-11-07 : constats et recommandations performance (ANO-125) transmis au PO sur ordre de l'utilisateur |
| agent expert ci (session 01UY1Vv4) | expert de la pipeline CI (`.claude/agents/expert-ci.md`) : diagnostics, US techniques de la CI | 2026-11-07 21:06:14 : WFS-277 🟣 (PR #234 : tableau journée / matin validé) ; bancs du matin lancés à la main pour preuve ; puis gel de la CI 2-3 jours |

## Info pour les développeurs (2026-11-06 23:47 UTC)
- Le rendu se vérifie **en local** avec lavapipe : `mesa-vulkan-drivers` et `glslc` s'installent dans le conteneur (paquets apt), puis `WFS_BANC=<groupe> build/wfs_bench` (même rendu logiciel que la CI ; testé par Claude, agent 2 sur WFS-197).
