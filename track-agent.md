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

**Dernière mise à jour** : 2026-11-13 13:27:14 UTC (po-scrum-master)

> 🔴 **Alerte** : **vidéo de l'utilisateur (12:04, Poursuite)** → 2 tickets P1 de plus **prêts** pour de nouvelles sessions : **WFS-460** (`@dev`, police : bandes « POLICE » sur la tôle, peinture blanche stable ; ANO-250, 260) et **WFS-461** (`@conduite`, glisse au contact de la police, citadine à 300 km/h ; ANO-261, 262 ; attribué à 19:30 à Claude, agent conduite (01UY1Vv1A7)). **bancs du matin #22 verts** (robot des statuts 10:10). **Recette QA du 2026-11-12** : 3 REFUSÉ (WFS-239, 351, 370) → 4 tickets P1 de correction **prêts** pour de nouvelles sessions : **WFS-453** (`@dev`, trafic sous 120 m non dessiné ; attribué à 15:37 à Claude, agent dev3d (01UY1Vv5g8)), **WFS-454** (`@cicd`, émulateur qui appuie sur « Duel en ligne »), **WFS-455** (attribué à 15:38 à Claude, agent dev3d (01UY1Vv3N7)), **WFS-456** (`@devartiste`, disque noir du hayon de la Golf). Mineures : WFS-457 (après 456), WFS-458 (prêt), WFS-459 (après 452). WFS-452 attribué à 12:33 (Claude, agent dev3d (01Us6uCwQm)) ; **WFS-371** (Corniche, E15) attribué à 13:09 (Claude, agent dev3d (01UY1Vv2rp)), fusionné (#366) : **WFS-463** attribué à 15:32 à agent expert ci cd (01UY1Vv3nf) ; **WFS-454** (`@cicd`) prêt pour une nouvelle session. Autres prêts : **WFS-378** (P1), **WFS-446** (P2). ⚠️ **WFS-383** (PR #358) : dernier envoi 2026-11-12 08:26 (554619ce), en correction après le 2e avis agent 3D. WFS-405, 319, 407, 408, 409 en attente du jour de publication.

## Agents développeurs

| Agent (session) | Ticket en cours | Ce qu'il fait | Branche, dernier envoi (UTC) | État |
|---|---|---|---|---|
| Claude, agent dev3d (01Us6uCu2B) (session_01Us6uCu2BHguHTRUmSw2Bgz) | | Correction Golf : hayon continu sans disque noir ni emblème (toutes les voitures), phares sans liseré rouge, banc `vitrines` sans les voitures retirées (ANO-259, 258, 257 ; E10) | `feature/waforspeed-456`, a38bdb52 | ⏹️ session terminée (WFS-456 fusionné, #385, 66bf9698) |
| Claude, agent dev3d (01Us6uCwDY) (session_01Us6uCwDYTdSxRRFoTrWT3c) | WFS-490 | 8 finitions de peinture dans le shader (E18, zone E10) : PR #388 ouverte, lead dev appelé | `feature/waforspeed-490`, 14:52 | ↩️ retour lead dev sur PR #388 : à corriger |
| Claude, agent dev3d (01Us6uCvhS) (session_01Us6uCvhSyUxjQqVifkPVmG) | WFS-481 | Écran « Crée ton pilote » au 1er lancement, FR / EN, < 20 s (E17, zone E8) : PR #379 refusée par le lead dev (tests CPU, émulateur) | `feature/waforspeed-481`, 09:26 (489278ce) | ↩️ retour lead dev sur PR #379 : à corriger |
| Claude, agent dev3d (01Us6uCudS) (session_01Us6uCudS6q2Mm6FiVxo6e7) |  | CGU et confidentialité en anglais (jeu et `store/`), avis du juriste (E20, zone E8) | `feature/waforspeed-508`, 13:14 (ca749915) | ⏹️ session terminée (WFS-508 fusionné, #384, d83ead9c) |
| Claude, agent dev3d-cori (01UY1Vv4T3) (session_01UY1Vv4T36y95yE18wiLeCN) | WFS-533 | [TECHNIQUE] CI de `main` rouge : crédit de `profil/mots_interdits.txt` dans le contrôle des licences (E0) | `feature/waforspeed-533`, poussée 11:50 (13/11) | ⏹️ session terminée (WFS-533 fusionné, #381, 3013cf97) |
| Claude, agent dev3d (01Us6uCvzs) (session_01Us6uCvzsxvoKDpDXD7tC4y) | | Étoiles : portefeuille, prix dans `configgame.json`, voitures à acheter, essai gratuit, banc d'équilibrage (E18, zone E3 + configgame) | `feature/waforspeed-498`, poussée 13:09 (13/11) (792e2240) | ⏹️ session terminée (WFS-498 fusionné, #382) |
| Claude, agent dev3d (01Us6uCvEv) (session_01Us6uCvEvngLZWcCkMPKtFB) | | Course en anglais : HUD, messages, consignes de navigation, fin de course des 4 modes (E20, zones E3 + E5) | `feature/waforspeed-506`, 14:42 (2b9cd185) | ⏹️ session terminée (WFS-506 fusionné, #386, 1d969bb9) |
| lead-dev (renfort) | | WFS-534 fusionné (PR #387, 497814ac), fusion par l'auteur sur ordre de l'utilisateur | `feature/waforspeed-534`, 14:49 (13/11) | ⏹️ session terminée (WFS-534 fusionné) |

## Agents de pilotage

| Agent | Rôle | Dernière action |
|---|---|---|
| dev | développeur général (`.claude/agents/dev.md`, appelé par `@dev`) : un ticket du PO, jusqu'à la fusion, puis arrêt | créé le 2026-11-09 (demande de l'utilisateur) |
| expert-conduite | développeur expert en physique de la voiture (skill `expert-conduite`) : chantier « Conduite réaliste » | créé le 2026-11-07 |
| lead-dev | relit chaque PR : ouvre les captures avant / après (≥ 3, `qa/pr/WFS-XX/`), compare à la demande de l'utilisateur, **seul à fusionner** (squash) ou refuse ; appelé par le développeur juste après sa PR (`.claude/agents/lead-dev.md`) | 21:27 (11/11) : PR #363 (WFS-451) relue par une autre instance, approuvée et fusionnée (df461365) ; 18:58 : PR #297 (WFS-338) en revue ; 17:24 : #311 (X5) refusée une 3e fois, fermée |
| dev-artiste-3d | développeur **et** artiste 3D (skills `agent-developer`, `waforspeed-natif`, `artiste-3d`) : voitures sans marque, modèles 3D + code ; demande son ticket au PO (`@devartiste`, `@dev3d`) | créé le 2026-11-09 19:22 UTC (demande de l'utilisateur) ; tickets voitures WFS-358 à 361 |
| artiste-3d | artiste 3D (skill `artiste-3d`) : choisit, retouche, intègre les modèles et actifs 3D, sans code du moteur ; demande son ticket au PO (`@artiste`) | créé le 2026-11-09 19:22 UTC (demande de l'utilisateur) |
| expert-cartes | développeur expert des cartes et de leur téléchargement (paquets de cartes, E14, WFS-301 à 314) ; skill `expert-cartes` ; demande son ticket au PO | créé le 2026-11-08 14:14 UTC (demande de l'utilisateur) |
| po-scrum-master | attribue les tickets, écrit les règles de gestion, tient `TACHES.md`, le journal et ce fichier | 13:05 (13/11) : WFS-490 attribué à Claude, agent dev3d (01Us6uCwDY) (10e du bandeau 🏁 ; 489 attend WFS-534) ; lignes des sessions terminées 488 et 505 enlevées |
| QA-arwa (qa-lead, Claude session 01Us6uCubd) | recette de nuit des tickets de `qa/Check.md`, verdicts et anomalies ; campagne VABF sur émulateur | recette du 2026-11-08 nuit terminée (01:06) : 25 tickets ; ANO-160 à 167 créées, planifiées par le PO à 01:12 |
| expert-realisme-3d | expert du graphisme et du réalisme 3D (WFS-148 à 167) : **consultant, pas développeur** ; conseille les développeurs, ne tient pas de ticket de code | à consulter par arwa-2 pour WFS-192 (méthode d'effacement des logos, ΔE2000 ≤ 2) |
| juriste | juriste (skill `juriste`, `@juriste`) : regarde l'application du côté juridique (marques, licences NC, crédits, RGPD, CGU, Google Play) ; rapports dans `juridique/` ; ne code pas | créé le 2026-11-10 12:40 UTC (demande de l'utilisateur) ; 1er travail attendu : avis sur la sortie Android (WFS-320, WFS-404) |
| smart-brainstorming | propose des idées dans `recommandations-smart-brain.md`, ne décide rien, ne parle qu'avec l'utilisateur | 2026-11-07 : constats et recommandations performance (ANO-125) transmis au PO sur ordre de l'utilisateur |
| agent expert ci cd (session 01UY1Vv4) | expert CI / CD (pipeline et livraison) (`.claude/agents/expert-ci-cd.md`) : diagnostics, US techniques de la CI | 2026-11-09 10:42:51 : WFS-351 🟣 PR #306 fusionnée par le lead dev (788c286) ; libre |
| agent expert ci cd (session 01UY1Vv2Hq) | expert CI / CD (pipeline et livraison) (`.claude/agents/expert-ci-cd.md`) : diagnostics, US techniques de la CI | 2026-11-11 12:57 : WFS-427 attribué (bancs du matin, `ecrans-2400` scindé), voir le tableau des développeurs ; 09:13 : « Paquets de cartes » relancé sur `main` (ordre de l'utilisateur), run n° 2 vert, paquets de la Ville de WFS-416 publiés |

## Info pour les développeurs (2026-11-06 23:47 UTC)
- Le rendu se vérifie **en local** avec lavapipe : `mesa-vulkan-drivers` et `glslc` s'installent dans le conteneur (paquets apt), puis `WFS_BANC=<groupe> build/wfs_bench` (même rendu logiciel que la CI ; testé par Claude, agent 2 sur WFS-197).
