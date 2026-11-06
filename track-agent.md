# Suivi des agents — qui fait quoi

> **Règle de l'utilisateur (2026-11-06)** : ce fichier est **toujours synchronisé** avec `main`.
> - **Chaque agent développeur** met à jour **sa propre ligne** à chaque étape (ticket attribué, branche créée et poussée, PR ouverte,
>   fusion, CI de `main` verte, ticket fini, agent libre) et l'envoie sur `main` **dans le même commit** que `TACHES.md`.
> - **Le PO** remet tout le fichier à jour à chaque appel (attribution, point d'avancement) d'après `TACHES.md` et l'heure du dernier
>   envoi de chaque branche (`git log -1 --format=%ci origin/feature/waforspeed-XX`), et met à jour « Dernière mise à jour ».
> - Mêmes règles que `TACHES.md` : partir de `main` (`git pull`), ne modifier que sa ligne, ne jamais copier ce fichier d'une autre branche.
> - Alerte ⚠️ quand un ticket n'a rien reçu depuis plus de 2 h, ou n'a pas de branche 30 min après l'attribution ; 🔴 pour un ticket P1.

**Dernière mise à jour** : 2026-11-06 17:22:06 UTC (po-scrum-master)

## Agents développeurs

| Agent (session) | Ticket(s) | Ce qu'il fait | Branche, dernier envoi (UTC) | État |
|---|---|---|---|---|
| Claude, session trafic | WFS-150, WFS-182, WFS-183 | socle de rendu Réalisme ; halos de frein flottants ; derniers adversaires au salon | 150 : 16:22 · 182 : 15:39 · 183 : aucune branche | ✅ actif ; WFS-182 🟣 fusionné (PR #144) ; WFS-183 sans branche depuis 15:06 ⚠️ |
| Claude, agent 1 (01UY1Vv2) | — (WFS-139, WFS-180 fusionnés) | Corniche (1/3) et (2/3) | 180 : 16:42 | ✅ libre |
| Claude, agent 2 (01UY1Vv5) | — (WFS-184 fusionné) | jingles de fin de la poursuite, sons de la route | 184 : 16:38 | ✅ libre |
| Claude, agent 4 (011cpqZan) | WFS-137 | Ville (1/4) : rond-point à fontaine, carrefours à feux (ANO-81, ANO-84 déjà faites) | 137 : 17:12 | ✅ actif ; branche synchronisée avec `main` |
| Claude, agent 5 (011cpqZaDiS2) | — (WFS-140 fusionné, PR #137) | Grand Prix | 140 : 17:02 | ✅ libre |
| Claude, session dev expert (0183bv2X) | WFS-145 | CI de `main` rouge (test d'endurance) | 145 : 01:18 (avant la reprise de 13:10) | 🔴 P1, rien depuis la reprise de 13:10 ; le ticket lui reste (règle des 10 h sans commit) |
| Claude, session dev 5bx | WFS-178 | Campagne (2/3) : animaux et village | aucune branche (attribué 13:59) | ⚠️ pas de branche depuis 13:59 ; le ticket lui reste (règle des 10 h) |
| Claude, session 01Us6uCu | WFS-158 | dimensions des 20 voitures + carrosserie Model 3 (ANO-75) | 158 : 10:45 | ⚠️ rien depuis 10:45 ; le ticket lui reste jusqu'à 20:45 UTC (règle des 10 h) |

## Agents de pilotage

| Agent | Rôle | Dernière action |
|---|---|---|
| po-scrum-master | attribue les tickets, écrit les règles de gestion, tient `TACHES.md`, le journal et ce fichier | nouveaux statuts (🔀 / 🟣 / ✅), WFS-185, ce fichier |
| qa-lead | recette de nuit des tickets de `qa/Check.md`, verdicts et anomalies ; campagne VABF sur émulateur | rôle mis à jour à 17:05 |
| expert-realisme-3d | expert de l'axe Réalisme (WFS-148 à 167) | WFS-148 (référentiel du réel) ; aucun ticket en cours |
| smart-brainstorming | propose des idées dans `recommandations-smart-brain.md`, ne décide rien, ne parle qu'avec l'utilisateur | recommandations R01, R02 |

## Tickets prêts sans agent
| Ticket | Priorité | Sujet |
|---|---|---|
| WFS-185 | P1 Urgent | plus de CI sur les branches ni les PR |
