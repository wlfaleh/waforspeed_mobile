Agent : Claude, agent 2 (01UY1Vv5)

**Ticket** : WFS-315 — règles : `regles-de-gestion/WFS-315.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : « c'est bien, j'aime ce mode caméra Cockpit, mais j'ai l'impression que c'est une voiture à une seule place ; la géométrie largeur / longueur est à regarder » (2026-11-08 04:00 UTC).
- **Ce qui change** :
  - `src/render/vehicule_cockpit.h`, `vehicule_cockpit_defs.cpp` — **RG-01, largeur réelle par modèle** : table `LARGEURS` (largeur aux coudes, écart œil-axe, écart des pieds de montants) pour les 20 intérieurs ; avant, planche de 1,56 m et œil à 0,36 m pour toutes. Exemples : 500 Abarth 1,31 m, Golf 1,47 m, X5 1,55 m, Range Rover Sport 1,58 m. Cotes et sources au journal (1b66a9a2) : avis d'`expert-realisme-3d`, **aucune vérifiée** (sites des constructeurs bloqués), statut AC ou E.
  - `src/render/vehicule_cockpit.cpp` — planche, jonc, pavillon, montants (pieds à l'écart réel), portières suivent ces cotes ; **RG-02, place passager** : boîte à gants (porte en relief, joint mat) et couvercle du sac gonflable passager, **posé sur la partie plate du dessus de planche** (1er refus : posé à hauteur fixe sur la pente, il flottait d'≈ 3 cm devant la rue).
  - `src/game/camera_vue.cpp` (cockpit) — œil de la caméra à l'écart œil-axe du modèle (cockpit commun : 0,36 m, inchangé).
  - `tests/test_cockpit.cpp` — une cote par modèle ; deux adultes et la console tiennent ; œil ≈ 0,25 × coudes ; montants ; montant droit ≥ 2,5 fois plus loin que le gauche ; X5 plus large que la 500.
  - Aucun bouton changé, champ de 72° inchangé (RG-04, décisions du PO).
- **Comment c'est vérifié** : 69 tests CPU verts ; branche synchronisée avec `main` (b314ed5a) ; banc `cockpit` en **2340 × 1080**, Golf / X5 / 500 Abarth, avant (main) / après (branche), même run. Mesures en regard Normal (seuils du PO, 12:03:49 UTC), avant → après :
  - **Fenêtre de la rue devant : 0,0 % caché sur toutes les captures**.
  - Intérieur (≤ 42 %) : Golf jour 38,0 → 36,6, virage 39,1 → 37,7 ; X5 jour 34,9 → 34,2, virage 36,0 → 35,2 ; 500 jour 38,9 → 36,7, virage 40,0 → 38,0 ; Plage de nuit 34,1-36,3 %.
  - Bande de la rue (≤ 12 %) : 9,9 à 11,9 % (égales ou meilleures qu'avant). Horizon (45-47 %) : 46,4-46,6 %.
  - **Agrandissements de la planche passager** (Golf de jour, X5 au départ de nuit sur la Plage) ci-dessous : plus rien ne dépasse de la ligne de la planche.
- **Limites et risques connus** (décision du PO du 12:15:35) :
  - **La place passager de la planche (bas droit de l'image) est sous les boutons NITRO, FREIN et GAZ** : la boîte à gants se devine à peine. RG-02 se juge sur ce qui se voit au-dessus des boutons. Le banc ne sait pas masquer le HUD (hors zone) : pas de capture « sans HUD ».
  - **Sur la Golf, le changement visible est faible** : ses vraies cotes sont proches des anciennes valeurs communes ; l'écart se voit surtout entre voitures (X5 contre 500). Avec 115° de champ horizontal, la vitre gauche prend un tiers de l'image. Le PO a posé la question à l'utilisateur (boutons semi-transparents ou champ plus serré, dans un autre ticket).
  - Cotes au statut AC / E ; les moins sûres : SF90 (±0,06 m), Range Rover Sport (±0,05 m), écarts des montants (±0,05 à 0,08 m).

### Agrandissements de la planche passager

| | Avant (main) | Après (branche) |
|---|---|---|
| Golf, jour | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-zoom-planche-passager-golf-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-zoom-planche-passager-golf-jour.jpg?raw=true) |
| X5, Plage, nuit | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-zoom-planche-passager-x5-plage-nuit-depart.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-zoom-planche-passager-x5-plage-nuit-depart.jpg?raw=true) |

## Captures pour le lead dev (24 après)

Dossier : [`qa/pr/WFS-315`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `cockpit-abarth-braquage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-cockpit-abarth-braquage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-cockpit-abarth-braquage.jpg?raw=true) |
| `cockpit-abarth-carte` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-cockpit-abarth-carte.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-cockpit-abarth-carte.jpg?raw=true) |
| `cockpit-abarth-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-cockpit-abarth-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-cockpit-abarth-jour.jpg?raw=true) |
| `cockpit-abarth-nuit-virage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-cockpit-abarth-nuit-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-cockpit-abarth-nuit-virage.jpg?raw=true) |
| `cockpit-abarth-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-cockpit-abarth-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-cockpit-abarth-nuit.jpg?raw=true) |
| `cockpit-abarth-plage-nuit-depart` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-cockpit-abarth-plage-nuit-depart.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-cockpit-abarth-plage-nuit-depart.jpg?raw=true) |
| `cockpit-abarth-pluie` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-cockpit-abarth-pluie.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-cockpit-abarth-pluie.jpg?raw=true) |
| `cockpit-abarth-virage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-cockpit-abarth-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-cockpit-abarth-virage.jpg?raw=true) |
| `cockpit-golf-braquage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-cockpit-golf-braquage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-cockpit-golf-braquage.jpg?raw=true) |
| `cockpit-golf-carte` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-cockpit-golf-carte.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-cockpit-golf-carte.jpg?raw=true) |
| `cockpit-golf-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-cockpit-golf-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-cockpit-golf-jour.jpg?raw=true) |
| `cockpit-golf-nuit-virage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-cockpit-golf-nuit-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-cockpit-golf-nuit-virage.jpg?raw=true) |
| `cockpit-golf-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-cockpit-golf-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-cockpit-golf-nuit.jpg?raw=true) |
| `cockpit-golf-plage-nuit-depart` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-cockpit-golf-plage-nuit-depart.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-cockpit-golf-plage-nuit-depart.jpg?raw=true) |
| `cockpit-golf-pluie` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-cockpit-golf-pluie.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-cockpit-golf-pluie.jpg?raw=true) |
| `cockpit-golf-virage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-cockpit-golf-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-cockpit-golf-virage.jpg?raw=true) |
| `cockpit-x5-braquage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-cockpit-x5-braquage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-cockpit-x5-braquage.jpg?raw=true) |
| `cockpit-x5-carte` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-cockpit-x5-carte.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-cockpit-x5-carte.jpg?raw=true) |
| `cockpit-x5-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-cockpit-x5-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-cockpit-x5-jour.jpg?raw=true) |
| `cockpit-x5-nuit-virage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-cockpit-x5-nuit-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-cockpit-x5-nuit-virage.jpg?raw=true) |
| `cockpit-x5-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-cockpit-x5-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-cockpit-x5-nuit.jpg?raw=true) |
| `cockpit-x5-plage-nuit-depart` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-cockpit-x5-plage-nuit-depart.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-cockpit-x5-plage-nuit-depart.jpg?raw=true) |
| `cockpit-x5-pluie` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-cockpit-x5-pluie.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-cockpit-x5-pluie.jpg?raw=true) |
| `cockpit-x5-virage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/avant-cockpit-x5-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-315/qa/pr/WFS-315/apres-cockpit-x5-virage.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv5pG5V2148UwYk53fJ
