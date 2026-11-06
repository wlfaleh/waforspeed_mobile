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

**Dernière mise à jour** : 2026-11-06 23:02:08 UTC (po-scrum-master)

## Agents développeurs

| Agent (session) | Ticket en cours | Ce qu'il fait | Branche, dernier envoi (UTC) | État |
|---|---|---|---|---|
| agent arwa (Claude, session trafic) | WFS-190 | CI de `main` au vert de bout en bout : `endurance` verte au run 790 ; reste `banc-essai (realisme)` rouge (banc des passes WFS-150, 1 écart), enquête en cours (journal 22:45) | 190 : 20:40 (rien poussé depuis 2 h 22) | 🔴 P1 en cours, ⚠️ branche sans envoi depuis plus de 2 h ; suit aussi la CI de WFS-175 et WFS-150 (🟣) |
| Claude, agent 1 (01UY1Vv2) | WFS-199 | Grand Prix : 2 tribunes au plus, vrais spectateurs (personnages animés), plus aucune boîte | 199 : attribué 23:02, branche à pousser avant 23:32 ; 198 : fusionnée 22:36 (PR #165) ; 179 : PR #161 ; 176 : PR #158 | 🔴 P1 en cours ; WFS-198 🟣 (archives #1 vert à la main, ✅ au run de nuit de 03:07) ; suit la CI de `main` de WFS-176 et WFS-179 (🟣, runs 791 à 793) |
| Claude, agent 2 (01UY1Vv5) | WFS-197 | Vue cockpit réaliste (1/3) : intérieurs Golf, M4, 911, SF90, écran carte, essuie-glaces | 197 : 21:47 (4a52d55) | ✅ actif ; CI de branche lancée à la main (une fois, rendu) : captures `cockpit-*` à regarder avant la fusion |
| Claude, agent 4 (011cpqZan) |  |  |  | ✅ libre ; suit la CI de `main` de WFS-137 (🟣, PR #150) |
| Claude, agent 5 (011cpqZaDiS2) |  |  | 191 : fusionnée (PR #164, 9c64bf6, 22:32 UTC) ; 140 : fusionnée (PR #137, ad4fa31) | 🟣 suit la CI de main sur 9c64bf6 (WFS-191) ; WFS-140 reste 🟣 (run 42257956758 rouge au seul `endurance`, WFS-190) ; libre ensuite |
| Claude, session dev expert (0183bv2X) |  | libre (5 tâches faites : WFS-136, 122, 145, 187, 188) | 188 : 19:58 | 🟣 WFS-188 fusionné (PR #154) ; suit la CI de main (WFS-145, 187, 188) |
| Claude, session dev 5bx |  |  |  | ✅ libre ; suit la CI de `main` de WFS-178 (🟣, PR #148) |
| arwa-2 (Claude, session 01Us6uCu) | WFS-158 | livraisons 1 à 5 fusionnées (PR #157, #162, #163, #166, #167 : audit, plaques, échelle et roues, Model 3 remplacée sans facettes) ; RG-C (X5) signalé au PO | 158 : 00:09 (aa9fdb9 sur main, PR #167 fusionnée par confiance) | 🔵 suit la CI de main (PR #157 à #167) ; suite : plaques avant manquantes, étriers, intérieurs, ST-4, ST-5 |
| Claude, Agent 0 developer (session 01UY1Vv2SSLK) |  | WFS-151 fusionné (PR #156, squash 00bd580, fusion par confiance) | 151 : 22:14, fusionnée | 🔵 suit la CI de `main` de WFS-151 (🟣) |

## Tickets prêts sans agent

| Ticket | Épic | Pourquoi il attend |
|---|---|---|
| WFS-186 | E2 | prêt, mais E2 occupée par WFS-199 (P1) : un ticket actif par épic |
| — | — | WFS-152 à 167 (réalisme) attendent WFS-151 ✅ (🟣, PR #156) |

## Agents de pilotage

| Agent | Rôle | Dernière action |
|---|---|---|
| po-scrum-master | attribue les tickets, écrit les règles de gestion, tient `TACHES.md`, le journal et ce fichier ; **suit la CI de `main` de WFS-189 et WFS-196** et décide qui corrige si elle tombe rouge | 2026-11-06 23:02:08 : WFS-199 (P1, E2 + E11) attribué à Claude, agent 1 (01UY1Vv2) ; rouge `banc-essai (realisme)` du run 790 laissé à agent arwa (WFS-190 / WFS-150) ; règle « Ménage quotidien » mise à jour (WFS-198 RG-09) |
| qa-lead | recette de nuit des tickets de `qa/Check.md`, verdicts et anomalies ; campagne VABF sur émulateur | recette des voitures en course (19:00) : PASSABLE, ANO-97 à 102 |
| expert-realisme-3d | expert du graphisme et du réalisme 3D (WFS-148 à 167) : **consultant, pas développeur** ; conseille les développeurs, ne tient pas de ticket de code | WFS-148 (référentiel du réel) ; aucun ticket en cours |
| smart-brainstorming | propose des idées dans `recommandations-smart-brain.md`, ne décide rien, ne parle qu'avec l'utilisateur | recommandations R01, R02 |
