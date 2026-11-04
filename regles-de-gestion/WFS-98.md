# WFS-98 — Correction : commandes tactiles du jeu web, HUD de la vue cockpit, caméra qui suit le demi-tour
Épic : E3 (+ E0 `game.cpp`, E5 `hud.*`) · Priorité : P1 Urgent · Dépend de : — (WFS-27, WFS-66, WFS-70, WFS-75, WFS-90 ✅) · Taille : 2 jours
Anomalies corrigées : **ANO-31**, **ANO-24**, **ANO-30** (bloquantes) ; ANO-20 pour les fichiers de cette zone.

## Besoin
Le joueur ne sait pas quel disque est le gaz (ANO-31 : disques plats sans icône ni libellé, couleurs contraires au skill) ;
en vue cockpit deux compteurs se superposent (ANO-24) ; pendant le demi-tour la caméra fixe le trottoir et la voiture sort
du cadre (ANO-30). Ce sont les commandes et la vue qu'on voit en permanence en course : priorité absolue.

## Règles de gestion
- RG-01 (pédales, `.pedal` du web, téléphone couché `@media (max-height: 500px)`) : deux pédales rectangulaires à coins
  coupés 10 px (polygone `10px 0, 100%-10px 0, 100% 10px, 100% 100%, 0 100%, 0 10px`, en `uiQuad`) : **frein 84 × 88**,
  **gaz 64 × 120** (px CSS), écart 10 px, bas et droite dans les marges sûres (WFS-51 / WFS-52). Fond dégradé vertical
  rgba(44,48,66,.78) → rgba(16,18,26,.86) (2 sommets hauts / 2 bas), bordure 1 px rgba(255,255,255,.12) ; bordure du
  gaz rgba(255,106,0,.55). Libellés **« GAZ » / « FREIN »** en bas de la pédale, Rajdhani 700, 13 px, espacement .14em,
  majuscules ; icône simple au-dessus (flèche montante pour le gaz, deux barres pour le frein) couleur #FF6A00 (gaz) /
  #FF6A85 (frein).
- RG-02 (appui) : pédale appuyée = remplissage de bas en haut, gaz rgba(255,106,0,.9) → .25, frein rgba(255,31,75,.85) →
  .2 ; bordure #FF6A00 (gaz) ou #FF1F4B (frein) ; libellé et icône en #0A0B10 (`--on-accent`).
- RG-03 (nitro, `.nitro-touch`) : disque **68 px** à gauche des pédales (colonne 68 px du web couché), fond radial
  rgba(44,50,40,.9) → rgba(14,16,22,.9) ; **anneau de charge #B6FF00** (proportion = réserve de nitro, reste
  rgba(255,255,255,.1)), rayon intérieur 27 px ; libellé « NITRO » 10 px #B6FF00 en bas ; vide : gris (×0,8) ; actif :
  halo vert acide. Plus de cyan pour la nitro.
- RG-04 (volant, `.steer-pad` / `.steer-knob`) : pavé vitré **156 × 60** px à coins biaisés 12 px (polygone
  `12px 0, 100% 0, 100%-12px 100%, 0 100%`), fond rgba(30,33,48,.62) → rgba(14,16,24,.72), bordure rgba(255,255,255,.1)
  (cyan rgba(0,229,255,.45) à l'appui) ; piste 4 px rgba(255,255,255,.1) avec remplissage cyan du côté braqué ; **bouton
  volant 56 px** avec dessin 44 px : jante, moyeu et 3 branches #F2F4F8, repère cyan en haut ; il tourne avec le
  braquage. Mode « Flèches » (WFS-70) : mêmes fonds vitrés, chevrons #8A90A6.
- RG-05 : gaucher (WFS-75, `ecrans::miroirX`) et marges sûres conservés ; zones de toucher = rectangles dessinés (≥ 44 dp
  après WFS-102 ; d'ici là ≥ les tailles CSS ci-dessus × k).
- RG-05 bis (PO, 2026-11-04, à la réattribution) : la zone de toucher peut **déborder** du dessin (confort du pouce), jamais
  l'inverse. Le point (0,89 W ; 0,74 H) doit rester dans la zone du **gaz** (en gaucher : (0,11 W ; 0,74 H), appui du banc
  `course-gaucher` dans `src/ui/ecrans/reglages.cpp`, zone de WFS-96 : **ne pas modifier ce fichier**). Les appuis de
  `benchHud` (`hud.cpp` : gaz et nitro de `hud-nitro`) sont dans la zone du ticket : à recaler sur les nouvelles commandes.
- RG-06 (cockpit, ANO-24) : en vue cockpit (`etatCockpit()` / `V.cockpit`), **ne pas dessiner le compteur en arc** du
  HUD ; la vitesse s'affiche en chiffres 28 px sous le temps ; le cadran du tableau de bord 3D reste. Le disque cyan isolé
  du tableau de bord est supprimé (ou devient le voyant nitro vert acide #B6FF00 s'il vient de `buildCockpit`).
- RG-07 (caméra, ANO-30, `updateCamera` du web) : la poursuite suit le **cap de la voiture** (cap de la route + lacet
  de la voiture) avec retard : `camHd += dh × min(1, dt × 3,4)` (≈ 0,3 s), `dh` ramené dans [−π, π] ; pendant un
  tête-à-queue (`spinYaw`) seulement ×0,15. Recul minimal 6 m, hauteur ≥ 2,5 m pendant le demi-tour ; la voiture reste
  dans le tiers bas de l'image.
- RG-08 (ANO-20, dans cette zone) : tout `biseau()` de `camera_vue.cpp`, `direction.cpp` et `hud.cpp` dessiné en **un
  seul `r.uiQuad`** à 4 sommets (x + biais, y) (x + w, y) (x + w − biais, y + h) (x, y + h) ; plus de bandes.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- `css/style.css` l. 130-251 : `.touch`, `.steer-pad`, `.steer-track`, `.steer-knob` (+ SVG du volant dans `index.html`),
  `.pedal-zone`, `.pedal`, `.pedal-fill`, `.pedal-label`, `.nitro-touch`, `.nt-ring`, `.nt-label`, `@media (max-height: 500px)`.
- `js/world/scene.js` `updateCamera` l. 131-165 (`camHd`, retard 3,4, `spinYaw × 0,15`, `camDistDyn`, sol minimal).
- `js/ui/hud.js` (pas de compteur en cockpit : `C.cockpit`).
- Natif : `Game::drawControls` (`game.cpp` l. 190-204), `direction.cpp` (`directionDessiner`), `Renderer::uiQuad` (WFS-65),
  `ui::ORANGE` et couleurs du skill `design-street-racing`.

## Critères d'acceptation (captures du banc PC)
- [ ] `hud-course`, `encoche-hud-2400`, `course-*` : pédales rectangulaires « GAZ » (orange) et « FREIN » (rouge) lisibles,
  nitro rond avec anneau vert acide et « NITRO », volant dessiné dans un pavé vitré ; rien sous l'encoche ; gaucher en miroir (`gaucher-*`).
- [ ] une capture pédale gaz appuyée (remplissage orange) ; boutons du HUD (MENU, POURSUITE) sans rayures.
- [ ] `camera-cockpit`, `camera-cockpit-2400` : un seul compteur (celui du tableau de bord), plus de disque cyan isolé.
- [ ] `conduite-demitour` : la Golf entière au tiers bas de l'image, en travers de la route ; tests CPU verts.

## Hors périmètre
Échelle dp de toute l'interface (WFS-102), sons des commandes, vibrations, nouvelles vues de caméra.

## Zone de code
`src/game/game.cpp` (seulement `drawControls`, la caméra de poursuite et l'appui des commandes ; dessin conseillé dans un
nouveau `src/game/commandes_tactiles.*`), `src/game/direction.*`, `src/game/camera_vue.*`, `src/game/camera_banc.*` (si besoin), `src/game/conduite_libre_banc.*`,
`src/ui/hud.*`. **Interdit** : `src/ui/text.*`, `toasts.*`, `shaders/ui*` (WFS-95), `src/ui/ecrans/**` (WFS-96 ;
`ecrans::miroirX`, `reglages()` en lecture seule), `src/platform/**` et `.github/**` (WFS-91 : captures dans les bancs
déjà branchés), `src/render/**` (WFS-97, WFS-100, WFS-101), fichiers du duel (WFS-89), `src/world/**` (WFS-93).
