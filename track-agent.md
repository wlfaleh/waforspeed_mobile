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

**Dernière mise à jour** : 2026-11-06 21:06:35 UTC (po-scrum-master)

## Agents développeurs

| Agent (session) | Ticket en cours | Ce qu'il fait | Branche, dernier envoi (UTC) | État |
|---|---|---|---|---|
| agent arwa (Claude, session trafic) | WFS-190 | CI de `main` au vert de bout en bout (job `endurance`) | 190 : 20:40 (branche poussée) | 🔴 P1 en cours ; suit aussi la CI de `main` de WFS-175 🟣 (PR #155) |
| Claude, agent 1 (01UY1Vv2) | WFS-176 | Ville (3/3) : casino, centre commercial, cafés et bars, garages, pigeons et chiens | 176 : 20:45 (branche poussée) ; 185 : fusionnée 19:29 | ✅ actif ; suit aussi la CI de `main` de WFS-185 (🟣, PR #149, run 777) |
| Claude, agent 2 (01UY1Vv5) | WFS-189 | Correction Course à 5 : départ sans bouchon, adversaires de la classe du joueur, nitro rechargée en Course à 5 et Duel | 189 : 21:06 (e6758c6) | 🔧 en cours ; arbitrages 21:06 reçus (protocole B, exception Compass, rangées à 13 m validées) |
| Claude, agent 4 (011cpqZan) |  |  |  | ✅ libre ; suit la CI de `main` de WFS-137 (🟣, PR #150) |
| Claude, agent 5 (011cpqZaDiS2) |  |  |  | ✅ libre |
| Claude, session dev expert (0183bv2X) |  | libre (5 tâches faites : WFS-136, 122, 145, 187, 188) | 188 : 19:58 | 🟣 WFS-188 fusionné (PR #154) ; suit la CI de main (WFS-145, 187, 188) |
| Claude, session dev 5bx |  |  |  | ✅ libre ; suit la CI de `main` de WFS-178 (🟣, PR #148) |
| arwa-2 (Claude, session 01Us6uCu) | WFS-158 | remplacement de la Model 3 par un autre modèle libre (ordre de l'utilisateur, ANO-75), puis dimensions des 20 voitures | 158 : 15:19 (e0c5eff, synchronisée avec main ; ⚠️ rien poussé depuis plus de 5 h) ; RG-B : 6 essais de lissage sans effet (journal 18:3x) | ⏸ RG-D (Model 3) : attend le fichier du modèle (téléchargement Sketchfab bloqué, question à l'utilisateur 20:5x) ; avance sur la suite de WFS-158 |
| Claude, agent lumière (session 01UY1Vv1) | WFS-151 | Lumière : exposition physique, IBL préfiltrée, sondes, étalonnage charte grise / ColorChecker | 151 : 20:51 (e97571d), PR #156 | 🔀 En PR ; CI de branche lancée une fois (run 783, confiance ≤ 50 % : gros changement du rendu) |

## Agents de pilotage

| Agent | Rôle | Dernière action |
|---|---|---|
| po-scrum-master | attribue les tickets, écrit les règles de gestion, tient `TACHES.md`, le journal et ce fichier | 2026-11-06 21:06:35 : arbitrages WFS-189 (RG-02 protocole B, exception Compass ; RG-02 bis modèle de virage accepté ; RG-03 rangées à 8 m et 21 m validées) |
| qa-lead | recette de nuit des tickets de `qa/Check.md`, verdicts et anomalies ; campagne VABF sur émulateur | recette des voitures en course (19:00) : PASSABLE, ANO-97 à 102 |
| expert-realisme-3d | expert du graphisme et du réalisme 3D (WFS-148 à 167) : **consultant, pas développeur** ; conseille les développeurs, ne tient pas de ticket de code | WFS-148 (référentiel du réel) ; aucun ticket en cours |
| smart-brainstorming | propose des idées dans `recommandations-smart-brain.md`, ne décide rien, ne parle qu'avec l'utilisateur | recommandations R01, R02 |
