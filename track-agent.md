# Suivi des agents — qui fait quoi

> **Règle de l'utilisateur (2026-11-06)** : ce fichier est **toujours synchronisé** avec `main`.
> - **Chaque agent développeur** met à jour **sa propre ligne** à chaque étape (ticket attribué, branche créée et poussée, PR ouverte,
>   fusion, CI de `main` verte, ticket fini, agent libre) et l'envoie sur `main` **dans le même commit** que `TACHES.md`.
> - **Le PO** remet tout le fichier à jour à chaque appel (attribution, point d'avancement) d'après `TACHES.md` et l'heure du dernier
>   envoi de chaque branche (`git log -1 --format=%ci origin/feature/waforspeed-XX`), et met à jour « Dernière mise à jour : 2026-11-13 00:54:15 UTC (po-scrum-master)
> - Mêmes règles que `TACHES.md` : partir de `main` (`git pull`), ne modifier que sa ligne, ne jamais copier ce fichier d'une autre branche.
> - **Colonne « Ticket en cours »** (règle de l'utilisateur, 2026-11-06) : seulement les tickets 🔧 En cours ou 🔀 En PR ; **vide** si l'agent n'en a
>   aucun (les tickets 🟣 fusionnés ou ✅ terminés n'y figurent pas ; la CI de `main` suivie s'écrit dans « État »).
> - **Nom de l'agent** : la colonne « Agent » donne le nom à mettre à la fin de chaque titre de commit, entre parenthèses (règle de l'utilisateur, 2026-11-06).
> - **Ce fichier suit les agents, pas les tickets** (règle de l'utilisateur, 2026-11-06) : une ligne par agent (ce qu'il fait, sa branche,
>   son dernier envoi, son état). La liste des tickets et leurs statuts restent dans `TACHES.md`.
> - **Après une fusion, l'agent est libre** (règle de l'utilisateur, 2026-11-06 23:20 UTC) : il ne « suit » plus la CI de `main` ; la colonne État ne le dit plus.
> - **Un seul sujet par session (règle de l'utilisateur, 2026-11-09 12:05 UTC)** : chaque session de développeur mène **un seul ticket** jusqu'à la fusion, puis **s'arrête** (État « ⏹️ session terminée ») ; elle ne reprend pas de ticket. Les sessions qui tiennent un ticket à 12:05 UTC le terminent, puis s'arrêtent. Nouveau sujet = nouvelle session, nouvelle ligne.
> - **Agent fini = ligne enlevée** (règle de l'utilisateur, 2026-11-09) : dès qu'un développeur a fusionné son ticket (session terminée), le PO **enlève sa ligne** de ce tableau ; l'historique reste dans `TACHES.md` et `journal.md`.
> - Alerte ⚠️ quand un ticket n'a rien reçu depuis plus de 2 h, ou n'a pas de branche 30 min après l'attribution ; 🔴 pour un ticket P1.

**Dernière mise à jour** : 2026-11-13 19:04:28 UTC (po-scrum-master)

> 🔴 **Alerte** : 18:45 UTC : **WFS-536** (E22, score : les ★ du score remplacent les 10 ★ fixes) attribué à Claude, agent dev3d (01UY1Vv3A5). **WFS-535** fusionné : **WFS-537** (écran de fin du score) ⏳ Prêt ; **WFS-510** fusionné : **WFS-511** (Google Play Billing) ⏳ Prêt, pour de nouvelles sessions `@dev`. Retours lead dev à corriger : WFS-481 (#379), 489 (#395), 490 (#388), 507 (#396), 397 (#393). En PR : 500 (#392), 401 (#394), 483 (#397). WFS-410 🔧. Autres prêts : WFS-454 (`@cicd`), 394 (`@conduite`), 532, 446. WFS-405, 319, 407, 408, 409 en attente du jour de publication ; Campagne et Grand Prix ⛔ (ordre de l'utilisateur).

## Agents développeurs

| Agent (session) | Ticket en cours | Ce qu'il fait | Branche, dernier envoi (UTC) | État |
|---|---|---|---|---|
| Claude, agent dev3d (01UY1Vv3A5) (session_01UY1Vv3A5c1kqBucUa5xkdy) | WFS-536 | Score : fin de course, les ★ du score remplacent les 10 ★ fixes ; banc d'équilibrage adapté (E22, zone E3) | `feature/waforspeed-536` (à pousser) | 🔧 attribué à 18:45 UTC |
| Claude, agent dev3d (01Us6uCv7g) (session_01Us6uCv7gTobdvCxCSWWV5j) | | Cadeau du jour, logique sans écran : tirage, garantie au 30e, jour et heure protégés, sauvegarde, test 100 000 tirages (E19, zone E3 + section `cadeau` de configgame) | `feature/waforspeed-500`, 17:34 (6b8d3a23) | ⏹️ session terminée (WFS-500 fusionné, #392, 5d27c714) |
| Claude, agent dev3d (01Us6uCxKQ) (session_01Us6uCxKQDpfjkTaDzFG4mi) | WFS-401 | [RÉALISME] Eau : écume et rides moins « filées », traînée du soleil bas, éclaboussure du dauphin (E15, eau commune ; avis agent 3D requis) | `feature/waforspeed-401`, 19:30 (96a23dca) | 🔀 PR #394 : avis 3D VALIDÉ avec réserves, lead dev appelé |
| Claude, agent dev3d (01Us6uCt5A) (session_01Us6uCt5AqkHmCmnMByoftq) | | Voitures sans marque : Classe E W212 et VW Virtus GT retouchées à ±1 % (E10, `remplacants/`) | `feature/waforspeed-397`, d83006ba (2026-11-13 19:15) | ⏹️ session terminée (WFS-397 fusionné, #393, 7c51937c) |
| Claude, agent dev3d (01Us6uCvxs) (session_01Us6uCvxsAZZA6E2U7ZipqG) |  | WFS-483 (plaque 3D du joueur) fusionné par le lead dev le 2026-11-13 19:24 UTC (#397, squash 1a24dc15) | `feature/waforspeed-483`, 19:10 (33945d53) | ⏹️ session terminée (WFS-483 fusionné) |
| Claude, agent dev3d (01Us6uCw3C) (session_01Us6uCw3CcwYdVgdJvnqpWC) | | WFS-507 fusionné (PR #396, c0bd7a95) : garage et écran des paquets en anglais | `feature/waforspeed-507`, 19:18 (769fcb6a) | ⏹️ session terminée (WFS-507 fusionné) |
| Claude, agent dev3d (01Us6uCxMq) (session_01Us6uCxMq4ZcucYCU7MHDaF) | WFS-489 | Garage : 28 couleurs, toit et rétros, cadenas, prix, achat en ★ (E18, zone E5) : retour lead dev corrigé (banc garage), lead dev rappelé | `feature/waforspeed-489`, 7697ebc6 | 🔀 En PR #395 (corrigée, à relire) |
| Claude, agent dev3d (01Us6uCveE) (session_01Us6uCveEVtGAqp5Bg6kWqe) | WFS-410 | Fenêtres et portes réelles des lieux hors kit (Corniche, Ville), église de la Campagne modélisée : PR #398 ouverte, avis de l'agent 3D puis lead dev | `feature/waforspeed-410`, 18:52 (26e38ad4) | 🔀 PR #398 : agent 3D appelé |
| Claude, agent dev3d (01Us6uCwDY) (session_01Us6uCwDYTdSxRRFoTrWT3c) | WFS-490 | 8 finitions de peinture (E18, zone E10) : 3e retour corrigé (arbitrage du PO), PR #388 à relire | `feature/waforspeed-490`, 19:29 (35d93999) | 🔁 PR #388 corrigée, lead dev rappelé |
| Claude, agent dev3d (01Us6uCvhS) (session_01Us6uCvhSyUxjQqVifkPVmG) | WFS-481 | Écran « Crée ton pilote » au 1er lancement, FR / EN, < 20 s (E17, zone E8) : PR #379 refusée par le lead dev (tests CPU, émulateur) | `feature/waforspeed-481`, 09:26 (489278ce) | ↩️ retour lead dev sur PR #379 : à corriger |

## Agents de pilotage

| Agent | Rôle | Dernière action |
|---|---|---|
| dev | développeur général (`.claude/agents/dev.md`, appelé par `@dev`) : un ticket du PO, jusqu'à la fusion, puis arrêt | créé le 2026-11-09 (demande de l'utilisateur) |
| expert-conduite | développeur expert en physique de la voiture (skill `expert-conduite`) : chantier « Conduite réaliste » | créé le 2026-11-07 |
| lead-dev | relit chaque PR : ouvre les captures avant / après (≥ 3, `qa/pr/WFS-XX/`), compare à la demande de l'utilisateur, **seul à fusionner** (squash) ou refuse ; appelé par le développeur juste après sa PR (`.claude/agents/lead-dev.md`) | 21:27 (11/11) : PR #363 (WFS-451) relue par une autre instance, approuvée et fusionnée (df461365) ; 18:58 : PR #297 (WFS-338) en revue ; 17:24 : #311 (X5) refusée une 3e fois, fermée |
| dev-artiste-3d | développeur **et** artiste 3D (skills `agent-developer`, `waforspeed-natif`, `artiste-3d`) : voitures sans marque, modèles 3D + code ; demande son ticket au PO (`@devartiste`, `@dev3d`) | créé le 2026-11-09 19:22 UTC (demande de l'utilisateur) ; tickets voitures WFS-358 à 361 |
| artiste-3d | artiste 3D (skill `artiste-3d`) : choisit, retouche, intègre les modèles et actifs 3D, sans code du moteur ; demande son ticket au PO (`@artiste`) | créé le 2026-11-09 19:22 UTC (demande de l'utilisateur) |
| expert-cartes | développeur expert des cartes et de leur téléchargement (paquets de cartes, E14, WFS-301 à 314) ; skill `expert-cartes` ; demande son ticket au PO | créé le 2026-11-08 14:14 UTC (demande de l'utilisateur) |
| po-scrum-master | attribue les tickets, écrit les règles de gestion, tient `TACHES.md`, le journal et ce fichier | 19:04 (13/11) : arbitrage WFS-490 (chromé : critère (c) retiré, WFS-539 créé, ANO-266 dans WFS-366) ; 18:45 (13/11) : WFS-536 attribué à Claude, agent dev3d (01UY1Vv3A5) (E22, 1er P1 prêt du bandeau 🏆) ; 537 et 511 ⏳ Prêt ; lignes des sessions terminées enlevées ; 15:47 (13/11) : WFS-401 attribué à Claude, agent dev3d (01Us6uCxKQ) (plus aucun P1 prêt permis à `@dev`) ; 15:43 (13/11) : WFS-489 attribué à Claude, agent dev3d (01Us6uCxMq) (9e du bandeau 🏁, 1er prêt) ; 13:05 : WFS-490 attribué à Claude, agent dev3d (01Us6uCwDY) (10e du bandeau 🏁 ; 489 attend WFS-534) ; lignes des sessions terminées 488 et 505 enlevées |
| QA-arwa (qa-lead, Claude session 01Us6uCubd) | recette de nuit des tickets de `qa/Check.md`, verdicts et anomalies ; campagne VABF sur émulateur | recette du 2026-11-08 nuit terminée (01:06) : 25 tickets ; ANO-160 à 167 créées, planifiées par le PO à 01:12 |
| expert-realisme-3d | expert du graphisme et du réalisme 3D (WFS-148 à 167) : **consultant, pas développeur** ; conseille les développeurs, ne tient pas de ticket de code | à consulter par arwa-2 pour WFS-192 (méthode d'effacement des logos, ΔE2000 ≤ 2) |
| juriste | juriste (skill `juriste`, `@juriste`) : regarde l'application du côté juridique (marques, licences NC, crédits, RGPD, CGU, Google Play) ; rapports dans `juridique/` ; ne code pas | créé le 2026-11-10 12:40 UTC (demande de l'utilisateur) ; 1er travail attendu : avis sur la sortie Android (WFS-320, WFS-404) |
| smart-brainstorming | propose des idées dans `recommandations-smart-brain.md`, ne décide rien, ne parle qu'avec l'utilisateur | 2026-11-07 : constats et recommandations performance (ANO-125) transmis au PO sur ordre de l'utilisateur |
| agent expert ci cd (session 01UY1Vv4) | expert CI / CD (pipeline et livraison) (`.claude/agents/expert-ci-cd.md`) : diagnostics, US techniques de la CI | 2026-11-09 10:42:51 : WFS-351 🟣 PR #306 fusionnée par le lead dev (788c286) ; libre |
| agent expert ci cd (session 01UY1Vv2Hq) | expert CI / CD (pipeline et livraison) (`.claude/agents/expert-ci-cd.md`) : diagnostics, US techniques de la CI | 2026-11-11 12:57 : WFS-427 attribué (bancs du matin, `ecrans-2400` scindé), voir le tableau des développeurs ; 09:13 : « Paquets de cartes » relancé sur `main` (ordre de l'utilisateur), run n° 2 vert, paquets de la Ville de WFS-416 publiés |

## Info pour les développeurs (2026-11-06 23:47 UTC)
- Le rendu se vérifie **en local** avec lavapipe : `mesa-vulkan-drivers` et `glslc` s'installent dans le conteneur (paquets apt), puis `WFS_BANC=<groupe> build/wfs_bench` (même rendu logiciel que la CI ; testé par Claude, agent 2 sur WFS-197).
