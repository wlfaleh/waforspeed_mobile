# WFS-99 — Correction : garage lisible (voile à gauche, néons à droite, taches de lumière, voiture de 3/4 avant)
Épic : E5 · Priorité : P1 Urgent · Dépend de : — (WFS-11, WFS-45 ✅) · Taille : 1 à 2 jours
Anomalies corrigées : **ANO-22**, **ANO-23** (bloquantes), **ANO-33** (mineure) ; ANO-20 pour `garage.cpp`.

## Besoin
Au garage, les néons du décor traversent le titre et la description (ANO-22), aucune tache de lumière ne se voit sur le
sol et une voiture noire disparaît dans le fond (ANO-23), la voiture se présente de l'arrière et le panneau des
caractéristiques est en escalier (ANO-33). C'est l'écran où le joueur choisit sa voiture : il doit être net.

## Règles de gestion
- RG-01 (voile, ANO-22) : voile `.screen--garage` du web en 2 `r.uiQuad` à couleurs par sommet, de gauche à droite :
  rgba(10,11,16,.92) à 0 %, .70 à 34 %, 0 à 62 % de la largeur (inclinaison 100deg tolérée : dégradé horizontal suffit).
- RG-02 (néons, ANO-22) : les barres de néon (9 barres de `buildGarage`, 0,08 × 9 m, orange #FF6A00 une sur trois,
  sinon #00B8D4) placées pour apparaître à **x ≥ 45 % de l'écran** ou derrière le plateau ; aucune ne coupe le titre, la
  description ni la barre d'index (vérifier à 1280x720 et 2400x1080).
- RG-03 (sol, ANO-23) : sol albedo **#1A1C24, métal 0,2, rugosité 0,6** (au lieu de #101219 / 0,7 / 0,22) pour que le
  diffus des 3 projecteurs se voie : taches **orange** (s1 0xFF8A3A, 1,6, (−6, 8, 4)), **cyan** (s2 0x40E0FF, 1,6, (6, 8, −3))
  et **blanche** (s3 0xFFFFFF, 1,4, (0, 10, 8)), valeurs de `garage.js` inchangées.
- RG-04 (contre-jour, ANO-23) : liseré cyan sur la carrosserie par un **4e projecteur** `FrameParams::Spot` (8 places)
  placé derrière la voiture, #00E5FF, force 0,25 du diffus ; aucun changement de shader (s'il faut toucher `scene.frag`,
  le signaler au PO : zone de WFS-100).
- RG-05 (cap, ANO-33) : vue de départ de 3/4 avant : cap initial **0,6 + π** ; la rotation lente (`gYaw += dt × 0,35`)
  et le glissement d'arrivée restent.
- RG-06 (panneau, ANO-33 + ANO-20) : tous les `biseau()` de `garage.cpp` en **un seul `uiQuad`** (4 sommets) ; panneau
  des caractéristiques avec un coin coupé de 14 px d'un seul tenant ; crédit du modèle en **10 px #8A90A6**.
- RG-07 : l'accroche « mode regarder » de WFS-96 dans `garage.cpp` est gardée telle quelle à chaque synchronisation avec `main`.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- `js/world/garage.js` `buildGarage` l. 9-30 (sol, plateau, anneau, 9 néons, 3 projecteurs, hémisphère 0x5060A0 / 0x050508 0,6),
  `renderGarage` l. 49-62 (`gYaw`, caméra `sin(0,55) × dist2`, bloom 0,7 / 0,55).
- `css/style.css` l. 366 `.screen--garage`, l. 657 (portrait : non utilisé).
- Natif : `src/ui/garage.cpp` (`biseau`, `sol_`, `matSol_`, `spots[3]`), `src/render/bench_spots.cpp` (capture `spots-sol`),
  `Renderer::uiQuad` (WFS-65).

## Critères d'acceptation (captures du banc PC)
- [ ] `spots-sol`, `garage-0`, `garage-2` : 3 taches de lumière (orange, cyan, blanche) visibles sur le sol ; la 911 noire
  (`garage-2`) se détache du fond (liseré cyan).
- [ ] `garage-0`, `garage-1`, `garage-sf90`, `garage-m4` (et `-2400`) : aucun néon sur le titre, la description ou la barre
  d'index ; voile sombre à gauche, voiture vue de 3/4 avant.
- [ ] panneau des caractéristiques sans escalier, boutons et puces sans rayures ; crédit lisible ; tests CPU verts.

## Hors périmètre
Bloom du garage (WFS-100), mode regarder / Showroom (WFS-96), échelle dp (WFS-102), nouvelles voitures.

## Zone de code
`src/ui/garage.*`, `src/ui/planche_voitures.*`, `src/render/bench_spots.*`. **Interdit** : `src/ui/text.*`, `toasts.*`,
`shaders/**` (WFS-95, WFS-100), `src/render/renderer.cpp`, `vk.*` (WFS-100), `src/render/nuit.cpp` (WFS-97),
`src/ui/ecrans/**` (WFS-96), `src/platform/**`, `.github/**` (WFS-91), `src/ui/hud.*` (WFS-98).
