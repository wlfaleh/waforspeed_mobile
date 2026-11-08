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

**Dernière mise à jour** : 2026-11-08 02:40:09 UTC (po-scrum-master)

## Agents développeurs

| Agent (session) | Ticket en cours | Ce qu'il fait | Branche, dernier envoi (UTC) | État |
|---|---|---|---|---|
| agent arwa (Claude, session trafic) | WFS-281, WFS-266, WFS-284 | trois tickets en parallèle (autorisation de l'utilisateur), attribués 23:59 à sa demande : P1 WFS-281 trafic qui s'accumule après 2 min (E3, exception PO `traffic.*`, `traffic_system.*`, `tests/**`, coordination avec WFS-232 d'arwa-3 ; 🟡 après fusion) ; P1 WFS-266 cheveux et yeux des 12 humains (E11 ; `pietons.*` / `foule_*` interdits) ; P2 WFS-284 aucun ANR pendant un chargement (E7, `android_main.cpp`). Avant : WFS-270, 257, 224, 244 🟣 ; WFS-274 ↪ regroupé avec WFS-267 | 281 : 01:30 (98d22f7, PR #249) ; 266 : 01:45 (ff2bb31, PR #252) ; 284 : fusionnée 02:16 (94f6f41, #250) ; 270 : fusionnée 20:12 (#231) ; 257 : 20:00 (#220) ; 224 : 19:51 (#228) ; 244 : 18:05 (#218) | ↩️ retour lead dev sur PR #249 : à corriger (2e refus, 98d22f7 ; décisions du PO rendues à 02:18 dans WFS-281.md : RG-02 à chaque mesure, montée de difficulté gardée et plafonnée, pilote du banc corrigé dans la PR) ; WFS-284 : PR #250 fusionnée par le lead dev (squash 94f6f41, 🟣), libéré de WFS-284 ; WFS-266 : PR #252 fusionnée par le lead dev (squash 8c8bdc7, 02:17), 🟣 |
| Claude, agent 1 (01UY1Vv2) | WFS-297 | P1 [TECHNIQUE] le jeu ralentit d'une partie à l'autre (ANO-169) : mesure de 3 parties de suite, cause, rien ne s'accumule ; attribué 02:26 sur accord de l'utilisateur | 297 : 02:28 (4cdefa4, banc « parties de suite ») ; 286 : fusionnée (#253) | 🔴 en cours (P1) |
| Claude, agent 2 (01UY1Vv5) | WFS-282 | Vue Cockpit 2e passage, 2e partie : RG-02 regard plus doux, RG-03 rétroviseur en glace et œil +4 cm, RG-04 habitacle lisible la nuit, RG-05 mesuré ; captures 2340 × 1080 (Golf, SF90, X5, départ de nuit sur la Plage) | 282 : 02:21 (f97172f, synchronisation avec main) ; PR #251 ouverte | ↩️ retour lead dev sur PR #251 : à corriger (2e refus, 2026-11-08 02:40 : rétroviseur clair la nuit sur la Plage ; seuils de l'arbitrage PO dépassés : SF90 virage, regard Haut, montants < 30°) ; arbitrage PO n° 2 écrit : 30° retiré, SF90 virage ≤ 42 %, regard Haut sans seuil (RG-00 + RG-06 : 🟡 PR #244) |
| Claude, agent 3 (01UY1Vv3) |  | WFS-239 **repris par le PO** le 2026-11-08 01:16 UTC (règle de l'utilisateur des 10 h : aucun envoi depuis 2026-11-07 13:43:04, commit de début seul), donné à arwa-2 à sa demande | 239 : 13:43 (323eeb1, commit de début seul) ; 233 : fusionnée (PR #203, 4761db5) | ⚪ sans ticket ; s'il revient, il demande un ticket au PO |
| Claude, agent 4 (011cpqZan) |  | Stoppé par l'utilisateur (15:06 UTC) : **plus aucun ticket à lui donner** ; WFS-224 repris, puis donné à agent arwa (17:19 UTC) | 224 : 14:49 (533e360, commit de début seul, sans code) ; 240 : fusionnée (PR #205) | ⛔ stoppé par l'utilisateur |
| Claude, agent 5 (011cpqZaDiS2) | WFS-268 | P1 WFS-268 rouvert (recette KO par l'utilisateur, suite sur son ordre) : −50 % de personnes en Ville et Corniche, tout le fixe préparé au chargement (kit, commerces, lieux, feux), ≤ 1 Go de mémoire par carte (décision du propriétaire) | 268 : 01:18 (213df11, PR #248) ; + RG-11 (ANO-164, captures ≤ 1 %) | ↩️ retour lead dev sur PR #248 : à corriger (2e revue, 213df11 : captures « après » à régénérer sur la tête, RG-11 / ANO-164 marina et lacets-dessus ≤ 1 %, resynchroniser avec main) |
| Claude, agent dev X (0183bv2X, ex-« session dev expert ») | WFS-263 | P1 [TECHNIQUE] Performance 4 (E4, exception PO en parallèle de WFS-268 d'agent 5 et WFS-239 d'arwa-2) : shaders par coût, léger au loin, complet au premier plan ; attribué 00:29 UTC à sa demande ; avant : WFS-262 🟡 (#246, squash 26ce0c5), WFS-246 🟡 (#229, ad94d9d) | 263 : 01:24 (5922ca6, captures avant / après) ; mesures au journal 02:25 | 🔴 en cours (WFS-263, PR à ouvrir) |
| Claude, agent dev Y |  | WFS-216 fusionné (PR #196, squash f5340d7) | 216 : fusionnée 12:42 | 🟢 libre : pause de l'utilisateur finie à 18:00 UTC ; ticket seulement à sa demande |
| arwa-2 (Claude, session 01Us6uCu) | WFS-239 | P1 [TECHNIQUE] WFS-239 banc qualité : cause trouvée (joueur interpolé de WFS-221, voiture posée à 120 m du joueur simulé), voitures posées depuis le joueur dessiné, contrôle `ERREUR placement` ; PR #255, lead dev appelé. Fusionnés, 🟡 À valider (utilisateur) : WFS-245 (#238, #247) ; WFS-235 (#236) refusé → WFS-282 (agent 2) | 239 : 02:44 (5573be1, PR #255) | 🔀 PR #255 (lead dev) |
| arwa-4 (Claude, session 01Us6uCw) | WFS-298 | domaine **son** : P2 WFS-298 son des chocs selon ce qu'on touche (poteau, arbre, mobilier, mur, voiture), force selon la vitesse, côté gauche / droite ; + RG-00 `(void)cad;` de `pluie_sons.cpp` (remarque du lead dev) (E6, fichiers neufs `src/audio/chocs_sons*` + branchement `audio_system.*` ; lecture seule E3) ; attribué 02:34 à sa demande ; avant : WFS-288 🟣 (PR #254, d8080a2), WFS-278 🟣 (#240), WFS-275 🟣 (#237), WFS-272 🟣 (#227), WFS-261 🟣 (#219) | 298 : 02:37 (f44fae5, commit de début) ; 288 : PR #254 fusionnée 02:23 par le lead dev | 🔧 en cours ; question RG-03 / RG-05 tranchée par le PO à 02:39 (option A : synthèse en direct dans le thread audio) |
| arwa-3 (Claude, session 01Us6uCt) | WFS-232 | domaine **courses, modes et trafic** : P2 WFS-232 (1/2) motos et scooters dans le trafic, pilotes casqués, inclinaison en virage, feux la nuit (garés et faufilement → WFS-280) ; attribué 21:30 à sa demande ; exception PO : `traffic.*`, `traffic_system.*`, `deux_roues*`, `assets/cars/deux_roues/**`, **pas** `player.*` / `conduite_*` ; 🟡 après fusion ; avant : WFS-243 🟡 (PR #239), WFS-242 🟡 (PR #222) | 232 : PR #245 corrigée 02:49 (c5a9cb7, synchronisée avec main), 2e revue du lead dev ; 243 : fusionnée 21:20 (PR #239) | 🔀 en PR, corrigée, rappel du lead dev (P2) |
| Claude, Agent 0 developer (session 01UY1Vv2SSLK) | WFS-269 | Occlusion du décor fixe précalculée : PR #256 ouverte, lead dev appelé | 269 : 03:25 (2861e5a) | 🔀 en PR |
| arwa-5 (Claude, session 01Us6uCwsJ63) | WFS-283 | P1 WFS-283 ronds-points supprimés (E2 + accroches E3, E5) : interrupteur `trace::rondsPoints()`, virage de rue de 22 m au même cap, suite des cartes à la même place ; décor du village et de la station, bancs, GPS, tests (RG-06, RG-08) faits ; 63 tests CPU OK ; captures avant / après en cours | 283 : 01:04 (dc6a4fd, synchronisée avec main) ; 279 : fusionnée 22:49 (PR #241) | 🔴 en cours (P1) |
| Claude, agent dev conduite (session 01UY1Vv1Hhvw) | WFS-295, WFS-258 | P1 **WFS-295 d'abord** (ordre de l'utilisateur, 02:15) : la voiture tourne vraiment dans les virages et épingles (facteur 1/(1 − k·n), blocage après choc ≤ 0,5 s, ANO-168) ; puis la suite de WFS-258 (conduite 2e passage : RG-00b, RG-05 à 07, ANO-138, ANO-140) et WFS-259 ; sujets de conduite réservés à lui (exception de l'utilisateur) ; **info (PO, 23:21)** : arwa-5 (WFS-283 RG-06) ajoute 2 lignes au début de `main()` de `tests/test_aide_direction.cpp` et `tests/test_conduite.cpp` (`trace::forcerRondsPoints(true)`) : à garder en synchronisant | 295 : 02:15 (f04a335, début) ; 258 : 17:35 (ae71e2f), RG-00 fusionné #217 (04a1628) ; 256 : fusionné #216, 🟡 | 🔴 en cours (WFS-295) ; WFS-258 en attente derrière WFS-295 (ordre de l'utilisateur) : pas de retrait |

## Tickets prêts sans agent

| Ticket | Épic | Pourquoi il attend |
|---|---|---|
| WFS-273 (P1) | E4 | n° 0 du bandeau : premier ticket de E4, au premier développeur du domaine performance qui le demande (agent 5 d'abord), **pas** agent arwa |
| WFS-264, WFS-265 (P1) | E4 | après WFS-263 (agent dev X, en cours), une par une |
| WFS-214 (P2) | E13 | épic E13 tenue par WFS-269 (Agent 0) |
| WFS-192 (P2) | E10 | voitures sans marque, logos des 20 modèles ; exception PO, fichiers séparés de WFS-282, WFS-232, WFS-292 ; domaine voitures, au premier qui le demande |
| WFS-215 (P3) | E10 | après WFS-192 (mêmes modèles de voitures) ; domaine voitures |
| WFS-280 (P2) | E3 + E2 | en attente de WFS-232 (modèles et pilotes des deux-roues) |
| WFS-285 (P2) | E7 | prêt depuis la fusion de WFS-284 (#250) : surface recréée sans recharger le monde (ANO-154) ; une seule E7 active à la fois (avec WFS-287) |
| WFS-287 (P2) | E7 | [TECHNIQUE] banc émulateur rouge (`tools/qa/emulateur.sh`) ; E7 libre depuis la fusion de WFS-284 |
| WFS-290 (P2) | E8 | boutons « Réglages » / « Fermer » (ANO-159) ; E8 libre depuis la fusion de WFS-286 (#253) |
| WFS-292 (P2) | E4 + E10 | recette du 08/11 : fumée des pneus de jour, chocs du banc d'effets, Golf bleue la nuit (ANO-165, ANO-167) ; au premier développeur qui le demande |
| WFS-293 (P2) | E0 + E3 | [TECHNIQUE] banc de fluidité en virage et au carrefour, à-coup corrigé (ANO-166) |
| WFS-296 (P2) | E5 | marche arrière bien visible quand le frein fait reculer (ANO-168) |
| WFS-294 (P3) | E7 | [TECHNIQUE] lot CI (ANO-135, 136, 160, 161, 162) : en attente de la fin du gel de la CI (pas avant le 2026-11-09) |

## Agents de pilotage

| Agent | Rôle | Dernière action |
|---|---|---|
| expert-conduite | développeur expert en physique de la voiture (skill `expert-conduite`) : chantier « Conduite réaliste » | créé le 2026-11-07 |
| lead-dev | relit chaque PR : ouvre les captures avant / après (≥ 3, `qa/pr/WFS-XX/`), compare à la demande de l'utilisateur, **seul à fusionner** (squash) ou refuse ; appelé par le développeur juste après sa PR (`.claude/agents/lead-dev.md`) | créé le 2026-11-07 21:45 UTC (règle de l'utilisateur) |
| po-scrum-master | attribue les tickets, écrit les règles de gestion, tient `TACHES.md`, le journal et ce fichier | 2026-11-08 02:36 : **WFS-298** (son des chocs selon ce qu'on touche, E6) créé et attribué à arwa-4, à sa demande (aucun ticket prêt du domaine son : « jamais à court de tickets ») ; remarques du lead dev sur WFS-288 rangées (averse discrète : à juger par l'utilisateur ; `(void)cad;` → WFS-298 RG-00) ; WFS-295 noté 🔧 (branche de l'agent dev conduite, 02:15) ; WFS-285 ⏳ Prêt (WFS-284 fusionné) ; avant : 02:26 WFS-297 attribué à agent 1 (accord de l'utilisateur) ; 02:18 décisions WFS-281 (PR #249) ; 02:15 WFS-295 et WFS-296 créés (ANO-168) ; 01:17 WFS-239 repris à agent 3 et donné à arwa-2, WFS-192 prêt, ménage du jour de `tache utilisateur.md` ; 01:12 recette de nuit du 08/11 planifiée (WFS-292, 293, 294) ; historique complet au journal |
| QA-arwa (qa-lead, Claude session 01Us6uCubd) | recette de nuit des tickets de `qa/Check.md`, verdicts et anomalies ; campagne VABF sur émulateur | recette du 2026-11-08 nuit terminée (01:06) : 25 tickets ; ANO-160 à 167 créées, planifiées par le PO à 01:12 |
| expert-realisme-3d | expert du graphisme et du réalisme 3D (WFS-148 à 167) : **consultant, pas développeur** ; conseille les développeurs, ne tient pas de ticket de code | à consulter par arwa-2 pour WFS-235 (RG-04, intérieurs) |
| smart-brainstorming | propose des idées dans `recommandations-smart-brain.md`, ne décide rien, ne parle qu'avec l'utilisateur | 2026-11-07 : constats et recommandations performance (ANO-125) transmis au PO sur ordre de l'utilisateur |
| agent expert ci (session 01UY1Vv4) | expert de la pipeline CI (`.claude/agents/expert-ci.md`) : diagnostics, US techniques de la CI | 2026-11-07 21:06:14 : WFS-277 🟣 (PR #234 : tableau journée / matin validé) ; bancs du matin lancés à la main pour preuve ; puis gel de la CI 2-3 jours |

## Info pour les développeurs (2026-11-06 23:47 UTC)
- Le rendu se vérifie **en local** avec lavapipe : `mesa-vulkan-drivers` et `glslc` s'installent dans le conteneur (paquets apt), puis `WFS_BANC=<groupe> build/wfs_bench` (même rendu logiciel que la CI ; testé par Claude, agent 2 sur WFS-197).
