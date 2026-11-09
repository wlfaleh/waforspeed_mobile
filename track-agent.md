# Suivi des agents — qui fait quoi

> **Règle de l'utilisateur (2026-11-06)** : ce fichier est **toujours synchronisé** avec `main`.
> - **Chaque agent développeur** met à jour **sa propre ligne** à chaque étape (ticket attribué, branche créée et poussée, PR ouverte,
>   fusion, CI de `main` verte, ticket fini, agent libre) et l'envoie sur `main` **dans le même commit** que `TACHES.md`.
> - **Le PO** remet tout le fichier à jour à chaque appel (attribution, point d'avancement) d'après `TACHES.md` et l'heure du dernier
>   envoi de chaque branche (`git log -1 --format=%ci origin/feature/waforspeed-XX`), et met à jour « Dernière mise à jour : 2026-11-09 17:25:10 UTC (po-scrum-master)
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

**Dernière mise à jour** : 2026-11-09 17:00:11 UTC (po-scrum-master)

## Agents développeurs

| Agent (session) | Ticket en cours | Ce qu'il fait | Branche, dernier envoi (UTC) | État |
|---|---|---|---|---|
| agent arwa (Claude, session trafic) | WFS-353, WFS-348, WFS-338 | **P1 WFS-353** [TECHNIQUE] Basse : 2 lumières par pixel (accord E13 du PO), résolution 45 % seulement si l'utilisateur accepte : PR #307 fusionnée (45 % fait ; 2 lumières en variante du banc, décision du PO demandée ; poste suivant proposé : sol et trottoirs) ; **P1 WFS-348** fermé sur diagnostic, PR #300 à fusionner (lead dev) ; **P1 WFS-338** Poursuite police : PR #297 corrigée, 🔁 à relire (lead dev) ; WFS-339 🟣, WFS-333 🟡, WFS-335 🟡 | 348 : PR #300 (08:56, 3a6c1e1c, CI de branche run 948) ; 339 : PR #299 (05:51) ; 333 : PR #298 (04:03 ; corrigée 06:53, 71bc873d) ; 338 : PR #297 (04:52) | 🟡 353 (#307) fusionné par le lead dev 11:43 (squash 266e110) ; 🟣 348 (#300) fusionné par le lead dev 09:17 (squash 48c423e); ↩️ 338 (#297) : 3e retour du lead dev, contenu accepté, conflit journal seulement|
| Claude, agent 1 (01UY1Vv2) |  | P1 voitures sans marque : WFS-193 🟣 (Compass et 208, PR #308) ; X5 refusée 3 fois (#311, fermée sans fusion), sortie vers WFS-358 (nouvelle session, pas lui) | `feature/waforspeed-193` gardée (ad7aa366) | Terminé (session close) — ⏹️ WFS-193 fusionné, aucun autre ticket |
| Claude, agent dev Y (01UY1Vv1Yvim) | WFS-349 | `@dev` (session_01UY1Vv1YvimqoKDHcW3jYhr) : P1 file B, vue Cockpit : habitacle fermé sur les côtés, voiture proche plus dessinée sur la portière (ANO-181, 183) ; E10 (exception PO : `src/render/vehicule_cockpit*`, `tests/**` ; **pas** `camera_vue.*`, modèles des voitures) ; attribué 14:00 UTC à sa demande (1er et seul ticket de la session) | 349 : `feature/waforspeed-349` poussée (d1d41238, 2026-11-09 16:20 UTC) ; cause nommée au journal ; correction faite (portières pleines, plancher, tablier), banc `cockpit-voisine` des 20 voitures en cours | 🔧 en cours — mesures puis captures et PR ; finit WFS-349 jusqu'à la fusion, puis ⏹️ arrêt |
| Claude, agent dev W (01UY1Vv4Ed) | WFS-214 | `@dev` (session_01UY1Vv4EdEBdMyt3hmgrMoT) : P2 E13, ciel en paliers et chaussée bleue en Ultra (ANO-109, ANO-106) + RG-05 vision de nuit près des lumières ; attribué 2026-11-09 16:59 UTC (un seul sujet) | `feature/waforspeed-214` : à créer et pousser | 🔧 en cours (branche attendue sous 30 min) |
| Claude, agent cartes (01UY1Vv2dx) | WFS-346 | `@cartes` (session_01UY1Vv2dxNQxhJG6xe4ofua) : P2 WFS-346 carte de Noël (1 bis/3), neige lue du paquet (sol enneigé, flocons dehors seulement, adhérence) ; skills `expert-cartes` + `expert-conduite` ; zone : `src/world/cartes_v2/**`, `paquets/sources/**`, `tests/cartes_v2*`, `meteo.*`, `shaders/route.glsl`, `src/render/route_*`, `roues.*` + accroches ≤ 10 lignes `player.*` / `aide_direction.*` ; **pas** `poursuite*`, `hud.*`, `traffic.*` (WFS-338), `vehicule_cockpit*` (WFS-349), E13 (WFS-214) ; attribué 17:00 UTC à sa demande (1er et seul ticket de la session) | `feature/waforspeed-346` à pousser tout de suite | 🔧 en cours |

## Agents de pilotage

| Agent | Rôle | Dernière action |
|---|---|---|
| dev | développeur général (`.claude/agents/dev.md`, appelé par `@dev`) : un ticket du PO, jusqu'à la fusion, puis arrêt | créé le 2026-11-09 (demande de l'utilisateur) |
| expert-conduite | développeur expert en physique de la voiture (skill `expert-conduite`) : chantier « Conduite réaliste » | créé le 2026-11-07 |
| lead-dev | relit chaque PR : ouvre les captures avant / après (≥ 3, `qa/pr/WFS-XX/`), compare à la demande de l'utilisateur, **seul à fusionner** (squash) ou refuse ; appelé par le développeur juste après sa PR (`.claude/agents/lead-dev.md`) | créé le 2026-11-07 21:45 UTC (règle de l'utilisateur) |
| expert-cartes | développeur expert des cartes et de leur téléchargement (paquets de cartes, E14, WFS-301 à 314) ; skill `expert-cartes` ; demande son ticket au PO | créé le 2026-11-08 14:14 UTC (demande de l'utilisateur) |
| po-scrum-master | attribue les tickets, écrit les règles de gestion, tient `TACHES.md`, le journal et ce fichier | 12:12 : règle « appeler un agent avec @ » (`appel-agents.md`, CLAUDE.md) ; 12:05 : règle « un seul sujet par session » |
| QA-arwa (qa-lead, Claude session 01Us6uCubd) | recette de nuit des tickets de `qa/Check.md`, verdicts et anomalies ; campagne VABF sur émulateur | recette du 2026-11-08 nuit terminée (01:06) : 25 tickets ; ANO-160 à 167 créées, planifiées par le PO à 01:12 |
| expert-realisme-3d | expert du graphisme et du réalisme 3D (WFS-148 à 167) : **consultant, pas développeur** ; conseille les développeurs, ne tient pas de ticket de code | à consulter par arwa-2 pour WFS-192 (méthode d'effacement des logos, ΔE2000 ≤ 2) |
| smart-brainstorming | propose des idées dans `recommandations-smart-brain.md`, ne décide rien, ne parle qu'avec l'utilisateur | 2026-11-07 : constats et recommandations performance (ANO-125) transmis au PO sur ordre de l'utilisateur |
| agent expert ci cd (session 01UY1Vv4) | expert CI / CD (pipeline et livraison) (`.claude/agents/expert-ci-cd.md`) : diagnostics, US techniques de la CI | 2026-11-09 10:42:51 : WFS-351 🟣 PR #306 fusionnée par le lead dev (788c286) ; libre |

## Info pour les développeurs (2026-11-06 23:47 UTC)
- Le rendu se vérifie **en local** avec lavapipe : `mesa-vulkan-drivers` et `glslc` s'installent dans le conteneur (paquets apt), puis `WFS_BANC=<groupe> build/wfs_bench` (même rendu logiciel que la CI ; testé par Claude, agent 2 sur WFS-197).
