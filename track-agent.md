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
> - Alerte ⚠️ quand un ticket n'a rien reçu depuis plus de 2 h, ou n'a pas de branche 30 min après l'attribution ; 🔴 pour un ticket P1.

**Dernière mise à jour** : 2026-11-06 19:52:29 UTC (po-scrum-master)

## Agents développeurs

| Agent (session) | Ticket en cours | Ce qu'il fait | Branche, dernier envoi (UTC) | État |
|---|---|---|---|---|
| agent arwa (Claude, session trafic) | WFS-175 | Ville (2/4) : autoroute urbaine, tunnel et pont | 175 : attribué 19:36, branche à pousser | ✅ actif ; suit aussi la CI de `main` de WFS-181 🟣 (run 42279476180 en file) |
| Claude, agent 1 (01UY1Vv2) |  |  | 185 : fusionnée 19:29 | ✅ libre ; suit la CI de `main` de WFS-185 (🟣 fusionné 19:29, PR #149, run 777) |
| Claude, agent 2 (01UY1Vv5) |  |  | 184 : fusionnée 16:39 | ✅ libre ; WFS-184 ✅ (CI de `main` verte, run 753) |
| Claude, agent 4 (011cpqZan) |  |  |  | ✅ libre ; suit la CI de `main` de WFS-137 (🟣, PR #150) |
| Claude, agent 5 (011cpqZaDiS2) |  |  |  | ✅ libre |
| Claude, session dev expert (0183bv2X) | WFS-188 | conduite fidèle aux fiches : 0-100, vitesse max, freinage (ANO-97, ANO-98) | 188 : 19:25 | ✅ actif (WFS-145 et WFS-187 fusionnés) |
| Claude, session dev 5bx |  |  |  | ✅ libre ; suit la CI de `main` de WFS-178 (🟣, PR #148) |
| Claude, session 01Us6uCu | WFS-158 | remplacement de la Model 3 par un autre modèle libre (ordre de l'utilisateur, ANO-75), puis dimensions des 20 voitures | 158 : 10:45 | ⚠️ rien depuis 10:45 ; le ticket lui reste jusqu'à 20:45 UTC (règle des 10 h) |
| Claude, agent lumière (session 01UY1Vv1) | WFS-151 | Lumière : exposition physique, IBL préfiltrée, sondes, étalonnage charte grise / ColorChecker | 151 : 19:45 (ouverture de la branche) | ✅ actif (lancé à 19:46 ; consulte expert-realisme-3d pour les choix de graphisme) |

## Agents de pilotage

| Agent | Rôle | Dernière action |
|---|---|---|
| po-scrum-master | attribue les tickets, écrit les règles de gestion, tient `TACHES.md`, le journal et ce fichier | demande de Claude, agent 2 (01UY1Vv5) : aucun ticket ⏳ Prêt attribuable ; WFS-175 attribué à agent arwa |
| qa-lead | recette de nuit des tickets de `qa/Check.md`, verdicts et anomalies ; campagne VABF sur émulateur | recette des voitures en course (19:00) : PASSABLE, ANO-97 à 102 |
| expert-realisme-3d | expert du graphisme et du réalisme 3D (WFS-148 à 167) : **consultant, pas développeur** ; conseille les développeurs, ne tient pas de ticket de code | WFS-148 (référentiel du réel) ; aucun ticket en cours |
| smart-brainstorming | propose des idées dans `recommandations-smart-brain.md`, ne décide rien, ne parle qu'avec l'utilisateur | recommandations R01, R02 |

## Tickets prêts sans agent
| Ticket | Priorité | Sujet |
|---|---|---|
| WFS-190 | P1 Urgent | CI de `main` rouge (job `endurance`, run #761) : vérifier WFS-145 / 187 sur `main`, corriger si encore rouge |
