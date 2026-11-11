# Suivi des agents — qui fait quoi

> **Règle de l'utilisateur (2026-11-06)** : ce fichier est **toujours synchronisé** avec `main`.
> - **Chaque agent développeur** met à jour **sa propre ligne** à chaque étape (ticket attribué, branche créée et poussée, PR ouverte,
>   fusion, CI de `main` verte, ticket fini, agent libre) et l'envoie sur `main` **dans le même commit** que `TACHES.md`.
> - **Le PO** remet tout le fichier à jour à chaque appel (attribution, point d'avancement) d'après `TACHES.md` et l'heure du dernier
>   envoi de chaque branche (`git log -1 --format=%ci origin/feature/waforspeed-XX`), et met à jour « Dernière mise à jour : 2026-11-10 09:49:00 UTC (po-scrum-master)
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

**Dernière mise à jour** : 2026-11-11 18:39:28 UTC (po-scrum-master)

> 🔴 **Alerte** : **bancs du matin rouges 2 jours de suite (runs #12, #16)** : **WFS-377** (P1, E0) attribué à 17:55 à Claude, agent dev3d (01UY1Vv51w) ; **WFS-428** (P1) en PR #357 chez Claude, agent dev (01UY1Vv5Nm). CI de `main` de nouveau verte (#1010, WFS-445 fermé). ⚠️ **WFS-383** (01UY1Vv1zt) : dernier envoi 16:32 (plus de 2 h ; relance, pas de retrait avant 10 h). **WFS-392** (P2, E14) attribué à 18:39 à Claude, agent cartes (01UY1Vv2ghpU) ; `shaders/route.glsl` partagé avec WFS-424. Sessions closes depuis le dernier point (lignes enlevées) : WFS-365 (#355), WFS-429 (#356), WFS-445 (CI #1010), WFS-423 (#354). Pour les prochaines sessions `@dev` : **WFS-446** (P2, ⏳ Prêt, après la fusion de WFS-377), **WFS-378** (P1, inventaire hors décor). WFS-405, 319, 407, 408, 409 en attente du jour de publication.

## Agents développeurs

| Agent (session) | Ticket en cours | Ce qu'il fait | Branche, dernier envoi (UTC) | État |
|---|---|---|---|---|
| Claude, agent dev3d (01UY1Vv51w) | WFS-377 | banc `realisme` : le parcours WFS-150 retrouve « Ultra » dans les Réglages à onglets (ANO-211, bancs du matin) | `feature/waforspeed-377`, 2026-11-11 18:17 (9d5588e6) | 🔧 en cours (attribué 17:55) |
| Claude, agent dev3d (01UY1Vv1zt) | WFS-383 | sols de la Corniche (et des autres cartes) : herbe sèche / terre / roche selon la pente, pins d'Alep en vrais modèles (E15 file Corniche) | `feature/waforspeed-383`, 2026-11-11 16:32 (cb210482) | 🔧 en cours ; ⚠️ rien poussé depuis plus de 2 h |
| Claude, agent dev (01UY1Vv5Nm) | | banc `realisme-2` rouge : face +Z de nuit (M-01) et capture d'éclairage abandonnée en Campagne (US technique, bancs du matin) | `feature/waforspeed-428`, 2026-11-11 17:53 (8ebad8a2) | ⏹️ session terminée (WFS-428 fusionné, #357, squash e4cbb9f1) |
| Claude, agent dev3d (01UY1Vv3k4) | WFS-420 | banc de performance automatique sur le téléphone : trajet rejoué jour et nuit, un interrupteur de WFS-419 à la fois, fichier de résultats à partager (E4) | `feature/waforspeed-420`, 2026-11-11 18:34 (ea14ad17) | 🔧 en cours (attribué 18:33) |
| Claude, agent dev3d (01UY1Vv3cC) | WFS-424 | route en texture précuite en Basse : mesuré, rien à cuire (fbm non exécuté sur Ville / Corniche), annulation proposée | `feature/waforspeed-424`, 2026-11-11 18:59 (mesures) | ⏸ attend la décision de l'utilisateur |
| Claude, agent cartes (01UY1Vv2ghpU) | WFS-392 | cartes de données : chaussée sèche sous tous les tunnels, tunnel générique éclairé par de vrais luminaires (ANO-230, E14 + E9) | `feature/waforspeed-392`, pas encore poussée | 🔧 en cours (attribué 18:39) |

## Agents de pilotage

| Agent | Rôle | Dernière action |
|---|---|---|
| dev | développeur général (`.claude/agents/dev.md`, appelé par `@dev`) : un ticket du PO, jusqu'à la fusion, puis arrêt | créé le 2026-11-09 (demande de l'utilisateur) |
| expert-conduite | développeur expert en physique de la voiture (skill `expert-conduite`) : chantier « Conduite réaliste » | créé le 2026-11-07 |
| lead-dev | relit chaque PR : ouvre les captures avant / après (≥ 3, `qa/pr/WFS-XX/`), compare à la demande de l'utilisateur, **seul à fusionner** (squash) ou refuse ; appelé par le développeur juste après sa PR (`.claude/agents/lead-dev.md`) | 18:58 : PR #297 (WFS-338) en revue ; 17:24 : #311 (X5) refusée une 3e fois, fermée |
| dev-artiste-3d | développeur **et** artiste 3D (skills `agent-developer`, `waforspeed-natif`, `artiste-3d`) : voitures sans marque, modèles 3D + code ; demande son ticket au PO (`@devartiste`, `@dev3d`) | créé le 2026-11-09 19:22 UTC (demande de l'utilisateur) ; tickets voitures WFS-358 à 361 |
| artiste-3d | artiste 3D (skill `artiste-3d`) : choisit, retouche, intègre les modèles et actifs 3D, sans code du moteur ; demande son ticket au PO (`@artiste`) | créé le 2026-11-09 19:22 UTC (demande de l'utilisateur) |
| expert-cartes | développeur expert des cartes et de leur téléchargement (paquets de cartes, E14, WFS-301 à 314) ; skill `expert-cartes` ; demande son ticket au PO | créé le 2026-11-08 14:14 UTC (demande de l'utilisateur) |
| po-scrum-master | attribue les tickets, écrit les règles de gestion, tient `TACHES.md`, le journal et ce fichier | 18:39 (11/11) : WFS-392 attribué à Claude, agent cartes (01UY1Vv2ghpU) ; 18:33 (11/11) : WFS-420 attribué à Claude, agent dev3d (01UY1Vv3k4) ; 17:55 (11/11) : WFS-377 attribué à Claude, agent dev3d (01UY1Vv51w) ; WFS-420 et WFS-424 passés ⏳ Prêt ; 16:53 (11/11) : WFS-445 attribué à agent expert ci cd (session 01UY1Vv4Ch), ordre de l'utilisateur ; 16:31 (11/11) : WFS-422 annulé (sans objet), session 01UY1Vv5hT terminée ; 16:17 (11/11) : remise de WFS-418 (RG-03 de #353) : WFS-446 à 449 créés (mémoire graphique, suites) ; 16:14 (11/11) : alerte CI rouge, WFS-445 créé (P1) ; 16:09 (11/11) : WFS-422 attribué à Claude, agent dev (01UY1Vv5hT) ; 15:53 (11/11) : WFS-365 écart de la 911 accepté (rainure noire de 1,5 cm entre les feux, DEL du bouclier retirées) ; 15:34 (11/11) : WFS-423 attribué à Claude, agent dev (01UY1Vv4BW) ; 15:29 (11/11) : WFS-365 arbitrage RG-01 (phares de la 911 hors ticket), WFS-444 créé ; 14:59 (11/11) : WFS-365 attribué à Claude, agent dev3d (01UY1Vv3Ji) ; 12:57 (11/11) : bancs du matin rouges : WFS-427 créé et attribué à agent expert ci cd (session 01UY1Vv2Hq), WFS-428 créé (P1), WFS-377 passé P1 ; 12:55 (11/11) : WFS-364 attribué à Claude, agent dev3d (01UY1Vv2nQ) ; 12:42 (11/11) : WFS-418 arbitrage RG-02 / RG-03 (atlas compris) et accroches validées ; 12:25 (11/11) : WFS-383 attribué à Claude, agent dev3d (01UY1Vv1zt) ; 12:24 (11/11) : WFS-418 attribué à Claude, agent dev (01Us6uCt95) ; 12:22 (11/11) : WFS-426 attribué à Claude, agent cartes (01UY1Vv2HrQF) ; 12:17 (11/11) : WFS-412 attribué à agent expert ci cd (session 01UY1Vv4jR6p) ; 12:10 (11/11) : WFS-421 attribué à Claude, agent dev (01UY1Vv4ss) ; 12:09 (11/11) : « go Corniche » (WFS-405 réécrit, WFS-386 après la sortie), WFS-426 créé, ordre des 20 prochaines tâches |
| QA-arwa (qa-lead, Claude session 01Us6uCubd) | recette de nuit des tickets de `qa/Check.md`, verdicts et anomalies ; campagne VABF sur émulateur | recette du 2026-11-08 nuit terminée (01:06) : 25 tickets ; ANO-160 à 167 créées, planifiées par le PO à 01:12 |
| expert-realisme-3d | expert du graphisme et du réalisme 3D (WFS-148 à 167) : **consultant, pas développeur** ; conseille les développeurs, ne tient pas de ticket de code | à consulter par arwa-2 pour WFS-192 (méthode d'effacement des logos, ΔE2000 ≤ 2) |
| juriste | juriste (skill `juriste`, `@juriste`) : regarde l'application du côté juridique (marques, licences NC, crédits, RGPD, CGU, Google Play) ; rapports dans `juridique/` ; ne code pas | créé le 2026-11-10 12:40 UTC (demande de l'utilisateur) ; 1er travail attendu : avis sur la sortie Android (WFS-320, WFS-404) |
| smart-brainstorming | propose des idées dans `recommandations-smart-brain.md`, ne décide rien, ne parle qu'avec l'utilisateur | 2026-11-07 : constats et recommandations performance (ANO-125) transmis au PO sur ordre de l'utilisateur |
| agent expert ci cd (session 01UY1Vv4) | expert CI / CD (pipeline et livraison) (`.claude/agents/expert-ci-cd.md`) : diagnostics, US techniques de la CI | 2026-11-09 10:42:51 : WFS-351 🟣 PR #306 fusionnée par le lead dev (788c286) ; libre |
| agent expert ci cd (session 01UY1Vv2Hq) | expert CI / CD (pipeline et livraison) (`.claude/agents/expert-ci-cd.md`) : diagnostics, US techniques de la CI | 2026-11-11 12:57 : WFS-427 attribué (bancs du matin, `ecrans-2400` scindé), voir le tableau des développeurs ; 09:13 : « Paquets de cartes » relancé sur `main` (ordre de l'utilisateur), run n° 2 vert, paquets de la Ville de WFS-416 publiés |

## Info pour les développeurs (2026-11-06 23:47 UTC)
- Le rendu se vérifie **en local** avec lavapipe : `mesa-vulkan-drivers` et `glslc` s'installent dans le conteneur (paquets apt), puis `WFS_BANC=<groupe> build/wfs_bench` (même rendu logiciel que la CI ; testé par Claude, agent 2 sur WFS-197).
