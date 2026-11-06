# WFS-60 — Roues séparées dans les modèles de voiture (.wfm)
Épic : E5 (+ API du rendu, E0) · Priorité : P2 Haute (version stable) · Dépend de : — · Taille : ≈ 1 à 1,5 jour (absorbe WFS-61 et WFS-62)

## Besoin (qui, quoi, pourquoi)
Aujourd'hui les roues d'une voiture sont dessinées avec la carrosserie (simple drapeau « roue » au niveau du matériau) : on ne peut ni les faire tourner ni les braquer. Il faut que chaque voiture ait **4 roues distinctes**, chacune avec son centre, pour que WFS-61 / WFS-62 les animent.

## Règles de gestion
- RG-01 : à la conversion, chaque roue devient un **maillage à part** (4 par voiture : avant gauche, avant droite, arrière gauche, arrière droite), repéré par un drapeau et un numéro de roue (0 à 3) dans le `.wfm`.
- RG-02 : pour chaque roue on enregistre son **centre** (x, y, z, en mètres, repère de la voiture) et son **rayon** ; la géométrie de la roue est recentrée sur son centre (elle tourne autour de son axe).
- RG-03 : l'étrier de frein ne tourne pas avec la roue : il reste dans la carrosserie.
- RG-04 : voiture rendue **à l'identique** qu'avant la conversion (aucune différence visible sur les captures du garage / de la course).
- RG-05 : le chargeur de modèles lit les roues et les expose (centre, rayon, maillage) sans casser les anciens `.wfm` (version de format augmentée, lecture des deux).
- RG-06 : les 4 voitures actuelles sont reconverties ; les autres voitures (WFS-13) utiliseront la même conversion.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/cars/models.js` → `splitWheels(...)` (découpe d'un maillage « 4 roues en un » par coin : LF, RF, LB, RB), `looksLikeWheel`, `WHEEL_RX` ; `wheelRx` des versions LOD.
- Modèles : `models/` (mêmes fichiers), conversion par `tools/convert_car.mjs` (déjà `wheel:` regex et drapeau 128).
- Retours connus : certaines voitures (Corsa, GT86, Range Rover, A 45) sont déjà simplifiées ; roues parfois découpées par coin.

## Critères d'acceptation (captures du banc PC)
- [ ] Captures de voiture (garage / course) identiques à avant (0 écart visible), 4 roues présentes sur chaque voiture
- [ ] Test : chaque `.wfm` donne 4 roues avec centre et rayon cohérents (rayon 0,25 à 0,45 m, centres symétriques gauche/droite)

## Hors périmètre
Animation (WFS-61, WFS-62), flou de roue, les 16 autres voitures.


## Périmètre élargi (PO, 2026-11-04 : gros tickets, une seule CI)
Ce ticket **absorbe WFS-61** (accroche de rendu par roue, E0) **et WFS-62** (animation selon la vitesse et le braquage). **Lire et appliquer aussi** `regles-de-gestion/WFS-61.md` et `regles-de-gestion/WFS-62.md` (toutes leurs règles et critères). Une seule branche `feature/waforspeed-60`, une seule CI à la fin.
Priorité relevée P4 → **P2 Haute** : sans roues qui tournent, la voiture « glisse » sur la route à chaque image ; c'est le défaut le plus visible restant pour la version stable, et il débloque WFS-67 (trafic vivant).

### Règles complémentaires
- RG-07 : ordre de travail : (1) conversion des 4 voitures avec roues séparées + chargeur (RG-01 à RG-06) ; (2) API du rendu : `draw(model, world, paint, const RouesPose* roues = nullptr)` ou équivalent, valeur par défaut = rendu actuel (WFS-61 RG-02) ; (3) calcul des angles dans un **nouveau module** `src/game/roues.{h,cpp}` (WFS-62), branché par accroche ≤ 10 lignes.
- RG-08 : le garage (WFS-11, `src/ui/garage.cpp`) n'est pas modifié : il appelle `draw` sans roues, voitures inchangées. Les 16 autres voitures (ex-WFS-13, étape 4 de WFS-11) **ne sont pas** dans ce ticket mais utiliseront la même conversion.
- RG-09 : angle de braquage lu depuis l'état du joueur (`wheelAngle` de `src/game/player.cpp`, exposé en lecture seule) **sans modifier la physique** ; si une valeur doit être exposée dans `player.h`, c'est une accroche ≤ 3 lignes annoncée au Journal (WFS-70 travaille dans ce fichier).

### Zone de code autorisée
`tools/convert_car.mjs`, `assets/cars/*.wfm` (4 voitures actuelles), `src/render/model_loader.{h,cpp}`, `src/render/renderer.h` (API, E0) et l'appel de dessin d'une voiture dans `src/render/renderer.cpp` (accroche minimale : WFS-09 y travaille), nouveaux `src/game/roues.{h,cpp}`, `tests/test_roues.cpp`, scène de banc `roues-*` (accroche ≤ 10 lignes dans `src/platform/desktop_main.cpp` et `.github/workflows/build.yml`), accroches ≤ 10 lignes dans `src/game/game.cpp` / trafic pour appeler le calcul.
**Interdit** : `src/game/player.*` et `src/game/direction.*` (WFS-70) sauf RG-09, `src/game/sauvegarde.*` (WFS-24), `src/ui/garage.*` et `src/ui/hud.cpp` (WFS-11), `shaders/scene*.frag`, `post*`, `sky*`, `shadow*` (WFS-09), `src/render/effets_*`.

### Critères d'acceptation regroupés (captures du banc PC)
- [ ] Captures `roues-arret-1` / `roues-arret-2` : roues identiques ; `roues-roule` : repère de jante tourné ; `roues-virage` : roues avant braquées vers l'intérieur, arrière droites
- [ ] Test `test_roues` : chaque `.wfm` donne 4 roues (rayon 0,25 à 0,45 m, centres symétriques gauche/droite) ; angle de roulage = distance / rayon ; braquage ×1,6 limité à ±0,6 rad, Ackermann 1,12 / 0,9
- [ ] Captures du garage et des autres scènes inchangées (banc `0 écart(s)` hors scènes `roues-*`) ; aucun ralentissement mesurable

### Rappel : réutiliser, ne rien recréer
`waaps/waforspeed/js/cars/models.js` → `splitWheels`, `looksLikeWheel`, `WHEEL_RX`, `animWheels(steerAngle, meters)` ; `js/world/scene.js` (≈ l. 208 et 238, appels `animWheels(yaw * 1.6, vitesse × dt)`) ; modèles `models/<voiture>/<voiture>.json` (mêmes sources que les `.wfm` actuels).

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| P2 Haute | WFS-60 | Roues animées : roues séparées dans les .wfm, rotation et braquage (absorbe WFS-61, WFS-62) | E5 (+ API E0) | ✅ Fusionné (fusion par confiance, QA de nuit à faire) | Claude (agent 4, session 011cpqZan), 2026-11-04 | — | `feature/waforspeed-60` | #48 |  |

Critères et notes de suivi :

**WFS-60 — Roues animées : roues séparées, rotation et braquage (absorbe WFS-61, WFS-62)** · E5 (+ API E0) · priorité P2 Haute · ✅ Fusionné le 2026-11-04 par confiance (PR #48, QA de nuit : `qa/Check.md`) (règles : `regles-de-gestion/WFS-60.md`, `WFS-61.md`, `WFS-62.md`)
- [ ] 4 roues distinctes (centre, rayon) par voiture ; voiture à l'arrêt identique à avant
- [ ] Rotation et braquage par roue dans l'API du rendu ; sans appel, rien ne change
- [ ] Roues immobiles à l'arrêt, qui tournent en roulant, avant braquées en virage (joueur et trafic proche)
