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
> - Alerte ⚠️ quand un ticket n'a rien reçu depuis plus de 2 h, ou n'a pas de branche 30 min après l'attribution ; 🔴 pour un ticket P1.

**Dernière mise à jour** : 2026-11-06 21:28:11 UTC (po-scrum-master)

## Agents développeurs

| Agent (session) | Ticket en cours | Ce qu'il fait | Branche, dernier envoi (UTC) | État |
|---|---|---|---|---|
| agent arwa (Claude, session trafic) | WFS-190 | CI de `main` au vert de bout en bout (job `endurance`) | 190 : 20:40 (branche poussée) ; suivi des runs noté sur main à 21:22 (783a874) | 🔴 P1 en cours ; suit aussi la CI de `main` : WFS-175 (🟣, PR #155) |
| Claude, agent 1 (01UY1Vv2) |  |  | 179 : fusionnée 21:37 (PR #161) ; 176 : fusionnée 21:11 (PR #158) | ✅ libre ; suit la CI de `main` de WFS-176 et WFS-179 (🟣) |
| Claude, agent 2 (01UY1Vv5) | WFS-197 | Vue cockpit réaliste (1/3) : intérieurs Golf, M4, 911, SF90, écran carte, essuie-glaces | 197 : 21:47 (4a52d55) | ✅ actif ; CI de branche lancée à la main (une fois, rendu) : captures `cockpit-*` à regarder avant la fusion |
| Claude, agent 4 (011cpqZan) |  |  |  | ✅ libre ; suit la CI de `main` de WFS-137 (🟣, PR #150) |
| Claude, agent 5 (011cpqZaDiS2) |  |  |  | ✅ libre |
| Claude, session dev expert (0183bv2X) |  | libre (5 tâches faites : WFS-136, 122, 145, 187, 188) | 188 : 19:58 | 🟣 WFS-188 fusionné (PR #154) ; suit la CI de main (WFS-145, 187, 188) |
| Claude, session dev 5bx |  |  |  | ✅ libre ; suit la CI de `main` de WFS-178 (🟣, PR #148) |
| arwa-2 (Claude, session 01Us6uCu) | WFS-158 | livraisons 1 et 2 fusionnées (PR #157 : audit ST-1, plaques neutres RG-A ; PR #162 : ST-2, échelle et roues de 15 voitures) ; suite : RG-C (X5), ST-3 ; Model 3 (RG-D) en attente du fichier | 158 : 21:47 (e622b04 sur main, PR #162 fusionnée par confiance) | 🔵 suit la CI de main (PR #157, #162), puis RG-C (hayon du X5) ; ⏸ RG-D : attend le fichier de la Model 3 (question à l'utilisateur) |
| Claude, Agent 0 developer (session 01UY1Vv2SSLK) | WFS-151 | Lumière : exposition physique, IBL préfiltrée, sondes, étalonnage charte grise / ColorChecker | 151 : 21:17 (737fde2, synchronisée avec main), PR #156 | 🔀 En PR ; compilation + 37 tests CPU verts en local ; run 783 (sur e97571d) en cours ; banc réalisme local en cours |

## Agents de pilotage

| Agent | Rôle | Dernière action |
|---|---|---|
| po-scrum-master | attribue les tickets, écrit les règles de gestion, tient `TACHES.md`, le journal et ce fichier ; **suit la CI de `main` de WFS-189 et WFS-196** (runs 780 à 787) et décide qui corrige si elle tombe rouge | 2026-11-06 21:28:11 : WFS-197 (lot 1 de WFS-195) attribué à Claude, agent 2 (01UY1Vv5) ; reprise du suivi CI de WFS-189 et WFS-196 |
| qa-lead | recette de nuit des tickets de `qa/Check.md`, verdicts et anomalies ; campagne VABF sur émulateur | recette des voitures en course (19:00) : PASSABLE, ANO-97 à 102 |
| expert-realisme-3d | expert du graphisme et du réalisme 3D (WFS-148 à 167) : **consultant, pas développeur** ; conseille les développeurs, ne tient pas de ticket de code | WFS-148 (référentiel du réel) ; aucun ticket en cours |
| smart-brainstorming | propose des idées dans `recommandations-smart-brain.md`, ne décide rien, ne parle qu'avec l'utilisateur | recommandations R01, R02 |
