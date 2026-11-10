# WFS-374 — notes de diagnostic (Claude, agent dev Z (01UY1Vv3EZ))

Ticket : dernière version de `main` cassée sur le téléphone. Bissection au banc d'essai PC sur les fusions
#313 (WFS-356), #312 (WFS-343), #297 (WFS-338), #311 (WFS-193). Résultats ajoutés ici au fil du travail.

## 2026-11-09 20:40 UTC — 1er passage de diagnostic (sur main 48f096c3, banc PC lavapipe)
- **APK que l'utilisateur avait à 19:35 UTC** : la Release `apercu` a reçu un nouvel APK à **19:23:49 UTC**, celui du run
  **#966 = WFS-338 poursuite police (8c4d65a)**, 12 min avant la plainte. Avant : APK du run #965 (WFS-343, 17:26 UTC),
  qui contenait aussi WFS-356, WFS-293 (B-spline du tracé), WFS-215 (runs #960 à 963 annulés).
- Bancs `menu1` (1280x720), `parcours`, `paquets` : **0 écart, codes 0**. Captures regardées (terrain, rouler, course,
  départ de nuit) : rien de cassé.
- Construction avec AddressSanitizer + UBSan : banc `carte` (les 4 cartes jour / nuit, changements de carte) **sans
  erreur mémoire** ; seul signalement : débordement d'entier signé dans le hachage de `effets_particules.h:55` (ancien,
  sans effet visible). Banc `course5` (Course à 5 sur les 4 cartes + poursuite police de WFS-338) sous ASan : **code 0, aucune erreur mémoire** (même seul signalement du hachage).
- Relecture des 5 fusions : aucun pointeur pendant ni double libération trouvés (WFS-343 : registre en `deque`,
  `Game::demandes().carte` est une `std::string` ; WFS-338 : maillages et textures remis à zéro après destruction).
- **Le défaut ne se reproduit pas au banc PC** : il faut le symptôme de l'utilisateur (RG-01).

## 2026-11-09 23:40 UTC — symptôme reçu et correctif
- PO (21:32 puis 21:35) : Campagne « bloquée » non confirmée (aucun doigt sur GAZ après le GO dans les vidéos) ; reste ANO-233 et le banc de départ.
- ANO-233 corrigée (`commandeSous`) ; banc `depart-campagne` : main 6 écarts (nitro sur SCORE), branche 0 écart ; 94 tests CPU verts.
