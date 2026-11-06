# WFS-45 — Lumières : projecteurs (spots), lumière hémisphère et brouillard linéaire
Épic : E4 (Rendu et performances) · Priorité : 0.46 · Dépend de : —

## Besoin (qui, quoi, pourquoi)
Le garage (WFS-11) est éclairé par 3 projecteurs colorés et une lumière hémisphère ; le renderer n'a qu'un soleil + ambiance.
Ces spots servent aussi aux phares de nuit (WFS-09). Catalyseur pour E5 (garage) et E4 (nuit).

## Règles de gestion
- RG-01 : jusqu'à **4 spots** par image dans le UBO de scène (`FrameParams`) : position, direction, angle (demi-angle du cône, rad),
  pénombre (0-1), couleur, intensité, portée (distance), décroissance ; `spotCount` = 0 par défaut.
- RG-02 : atténuation comme three.js r128 (`SpotLight(color, intensity, distance, angle, penumbra, decay)`) : cône lissé
  `smoothstep(cos(angle), cos(angle·(1−penumbra)), cosθ)`, distance `pow(saturate(1 − (d/distance)^4), 2) / d^decay` ;
  diffus + spéculaire GGX déjà utilisés pour le soleil (`shaders/scene.frag`). Pas d'ombre portée des spots.
- RG-03 : **lumière hémisphère** facultative : couleur haut, couleur bas, intensité (three `HemisphereLight`) : mélange selon
  `N.y` ; si désactivée, l'ambiance actuelle (ciel) reste identique.
- RG-04 : **brouillard linéaire** facultatif `fogNear` / `fogFar` (three `Fog(color, near, far)`) ; sinon brouillard exponentiel actuel.
- RG-05 : valeurs par défaut = rendu actuel au pixel près (0 spot, pas d'hémisphère, brouillard exponentiel).
- RG-06 : coût maîtrisé : boucle bornée par `spotCount`, coût de la passe de scène au journal du banc ≤ +0,3 ms avec 3 spots.
- RG-07 : seuls les champs ajoutés à `FrameParams` touchent `src/render/renderer.h` (ligne d'accroche E0 annoncée au Journal) ;
  le reste dans `src/render/**` et `shaders/scene*`.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- `js/world/garage.js` → `buildGarage` : valeurs du garage à reproduire au banc :
  `HemisphereLight(0x5060a0, 0x050508, 0.6)` ; `SpotLight(0xff8a3a, 1.6, 30, 0.6, 0.6, 1.2)` en (−6, 8, 4) ;
  `SpotLight(0x40e0ff, 1.6, 30, 0.6, 0.6, 1.2)` en (6, 8, −3) ; `SpotLight(0xffffff, 1.4, 30, 0.5, 0.7, 1.2)` en (0, 10, 8),
  visant l'origine ; `Fog(0x07070c, 14, 40)` ; sol `0x101219` rugosité 0,22, métal 0,7.
- Formules : celles de three.js r128 (skill `waforspeed-3d`) ; ne pas inventer un autre modèle d'éclairage.

## Critères d'acceptation (captures du banc PC)
- [ ] Toutes les captures existantes **identiques** à `main` (aucun spot par défaut)
- [ ] Nouvelle capture `spots-sol` : sol sombre (valeurs du garage), 3 taches de lumière orange, cyan et blanche aux bords
      adoucis, hémisphère bleutée, brouillard linéaire noir au loin ; une voiture `.wfm` au centre éclairée par les trois
- [ ] Coût GPU de la passe de scène au journal du banc : ≤ +0,3 ms avec 3 spots

## Hors périmètre
Ombres des spots ; phares et lampadaires de nuit (WFS-09, qui réutilisera ces spots) ; scène du garage (WFS-11).

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| P2 Haute | WFS-45 | Lumières : projecteurs (spots), hémisphère, brouillard linéaire | E4 | ✅ Fusionné (ordre de l'utilisateur, QA de nuit à faire) | Claude (agent 4, session 011cpqZan), 2026-11-03 | — | `feature/waforspeed-45` | #35 |  |

Critères et notes de suivi :

**WFS-45 — Lumières : projecteurs (spots), hémisphère, brouillard linéaire** · E4 · priorité P2 Haute · ✅ Fusionné le 2026-11-03 sur ordre de l'utilisateur (QA de nuit : `qa/Check.md`) — Claude (agent 4, session 011cpqZan), 2026-11-03 (règles : `regles-de-gestion/WFS-45.md`)
- [ ] Captures existantes identiques à `main` (0 spot par défaut)
- [ ] Capture `spots-sol` : 3 taches orange / cyan / blanche aux bords adoucis, hémisphère bleutée, brouillard linéaire, voiture éclairée
- [ ] Coût de la passe de scène ≤ +0,3 ms avec 3 spots (journal du banc)
