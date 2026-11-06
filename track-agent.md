# Suivi des agents — qui fait quoi

> **Règle de l'utilisateur (2026-11-06)** : ce fichier est **toujours synchronisé** avec `main`.
> - **Chaque agent développeur** met à jour **sa propre ligne** à chaque étape (ticket attribué, branche créée et poussée, PR ouverte,
>   fusion, CI de `main` verte, ticket fini, agent libre) et l'envoie sur `main` **dans le même commit** que `TACHES.md`.
> - **Le PO** remet tout le fichier à jour à chaque appel (attribution, point d'avancement) d'après `TACHES.md` et l'heure du dernier
>   envoi de chaque branche (`git log -1 --format=%ci origin/feature/waforspeed-XX`), et met à jour « Dernière mise à jour ».
> - Mêmes règles que `TACHES.md` : partir de `main` (`git pull`), ne modifier que sa ligne, ne jamais copier ce fichier d'une autre branche.
> - Alerte ⚠️ quand un ticket n'a rien reçu depuis plus de 2 h, ou n'a pas de branche 30 min après l'attribution ; 🔴 pour un ticket P1.

**Dernière mise à jour** : 2026-11-06 19:02:38 UTC (po-scrum-master)

## Agents développeurs

| Agent (session) | Ticket(s) | Ce qu'il fait | Branche, dernier envoi (UTC) | État |
|---|---|---|---|---|
| Claude, session trafic | WFS-181 (WFS-150, WFS-182, WFS-183 fusionnés) | Corniche (3/3) : bars de plage, casino, mouettes, dauphins | 181 : 18:37 (branche poussée) | ✅ actif ; WFS-181 🔧 en cours, branche poussée 18:37 ; WFS-150 / 182 / 183 🟣, CI de main en file |
| Claude, agent 1 (01UY1Vv2) | WFS-185 (WFS-139, WFS-180 fusionnés) | plus de CI sur les branches ni les PR | 185 : 18:04 | 🔴 P1 🔧 en cours (attribué 18:02, branche poussée) |
| Claude, agent 2 (01UY1Vv5) | — (WFS-184 fusionné) | jingles de fin de la poursuite, sons de la route | 184 : 16:38 | ✅ libre |
| Claude, agent 4 (011cpqZan) | WFS-137 | Ville (1/3) : rond-point à fontaine, carrefours à feux | 137 : 18:15 | 🟣 fusionné (PR #150), suit la CI de `main` |
| Claude, agent 5 (011cpqZaDiS2) | — (WFS-140 fusionné, PR #137) | Grand Prix | 140 : 17:02 | ✅ libre |
| Claude, session dev expert (0183bv2X) | WFS-187 (WFS-145 ⏸ en pause jusqu'à WFS-187) | fuite de jeux de descripteurs ; ensuite endurance de `main` (+ ANO-102, RG-17) | 187 : aucune branche (attribué 18:29) · 145 : 18:46 | 🔴 P1 🔧 WFS-187 en cours ; ⚠️ pas de branche 187 poussée 30 min après l'attribution ; WFS-145 en pause (décision PO 18:29) |
| Claude, session dev 5bx | WFS-178 | Campagne (2/3) : village ; WFS-138 ✅ (CI de main verte) | 178 : 17:56 (fusionné, PR #148) | ✅ actif ; WFS-178 🟣 fusionné, CI de main suivie |
| Claude, session 01Us6uCu | WFS-158 | dimensions des 20 voitures + carrosserie Model 3 (ANO-75) | 158 : 10:45 | ⚠️ rien depuis 10:45 ; le ticket lui reste jusqu'à 20:45 UTC (règle des 10 h) |

## Agents de pilotage

| Agent | Rôle | Dernière action |
|---|---|---|
| po-scrum-master | attribue les tickets, écrit les règles de gestion, tient `TACHES.md`, le journal et ce fichier | ANO-97 à 102 rangées : WFS-188, WFS-189 créés, RG-17 de WFS-145 |
| qa-lead | recette de nuit des tickets de `qa/Check.md`, verdicts et anomalies ; campagne VABF sur émulateur | recette des voitures en course (19:00) : PASSABLE, ANO-97 à 102 |
| expert-realisme-3d | expert de l'axe Réalisme (WFS-148 à 167) | WFS-148 (référentiel du réel) ; aucun ticket en cours |
| smart-brainstorming | propose des idées dans `recommandations-smart-brain.md`, ne décide rien, ne parle qu'avec l'utilisateur | recommandations R01, R02 |

## Tickets prêts sans agent
| Ticket | Priorité | Sujet |
|---|---|---|
| WFS-188 | P3 Moyenne | conduite fidèle aux fiches : 0-100, vitesse max, freinage par voiture (ANO-97, ANO-98) ; E3 libre |
| WFS-189 | P3 Moyenne | ⏳ en attente de WFS-188 : Course à 5 équilibrée, nitro sans trafic (ANO-99 à 101) |
