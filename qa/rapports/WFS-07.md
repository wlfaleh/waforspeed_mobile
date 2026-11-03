# QA — WFS-07 : ombres dynamiques du soleil
Date : 2026-11-03 · Relecteur : qa-lead · Branche : feature/waforspeed-07 (commit 6743fbc) · CI run 41550172280
Captures : Release `apercu-waforspeed-07` (reconstruite au commit 6743fbc), comparées à `apercu-waforspeed-32` (main avant le ticket)

## Verdict : VALIDÉ (2e passage)

Le défaut du 1er passage (aucune ombre au sol) est corrigé : la route, les trottoirs et les bas-côtés reçoivent enfin le
soleil direct et les ombres portées. Les ombres des voitures et des immeubles sont nettes sur la chaussée et les
trottoirs, sans acné, sans décollement et sans coupure visible. La route éclairée est plus claire qu'avant mais pas
blanche ni brûlée : sur la bande basse de l'image, aucun pixel saturé, maximum moyen d'environ 107 à 123 sur 255.

## Critères d'acceptation
- [x] **`course-cam0-*` de jour : ombres des voitures, immeubles et lampadaires sur chaussée et trottoirs** : OUI.
  Grandes ombres d'immeubles en diagonale sur la route et les deux trottoirs (`course-cam0-0`, `-1`, `-2`,
  `course-cam1-1`, `-2`). L'ombre de la Golf est bien posée au sol, collée au pare-chocs (`course-cam1-0`, `course-cam1-1`,
  `ombres-a`). Les 4 voitures en vitrine projettent leur ombre sur la chaussée (`vitrine-golf-0`, `vitrine-gallardo-0`,
  `vitrine-m4-1`, `vitrine-p911-2`). L'ombre fine d'un lampadaire est visible sur la chaussée (`vitrine-golf-0`,
  `vitrine-gallardo-0`, à droite de la voiture).
- [x] **Deux captures à 1 m d'écart, bords stables** : OUI. Le journal indique 1,07 m d'écart à 234 km/h. Dans
  `ombres-a` / `ombres-b`, le bord d'ombre de l'immeuble sur la route est net et rectiligne dans les deux cas, sans marches
  d'escalier ni dentelure. Il ne bouge que comme le prévoit le déplacement de la caméra.
- [x] **Nuit : aucune ombre solaire, ombre de contact conservée** (`ombres-coupees`) : aucune ombre portée. La route est
  uniforme et nettement plus claire que dans `ombres-a` (moyenne RVB 90/95/108 contre 35/48/72 sur le bas de l'image), ce qui
  prouve que les ombres agissent maintenant sur le sol. On voit une légère ombre de contact sous l'arrière de la Golf.
- [x] **Coût de la passe ; pas de régression de fidélité** : fidélité OK (« mode exact : 39 voitures comparées au jeu web,
  0 écart(s) »). Coût : de 24,8 à 35,7 ms GPU par passe, carte 2048, sur lavapipe (GPU logiciel). Ce chiffre n'est **pas
  représentatif** d'un téléphone et ne permet pas de vérifier le seuil de 3 ms : à remesurer sur un vrai GPU mobile
  (ticket « Vérification simulateur E4 » / WFS-15).

## Constats par capture
- `course-cam0-0` : Golf visible, image non déformée. Ombre d'immeuble en diagonale sur la route et sur le trottoir de gauche ;
  trottoirs bien éclairés au soleil, sans être blancs.
- `course-cam0-1`, `course-cam0-2` : la chaussée est presque entièrement à l'ombre des immeubles, avec une bande ensoleillée
  à droite. Le rendu est cohérent : le contraste est fort mais pas noir, l'ombre étant bleutée par la lumière du ciel.
- `course-cam1-0` : ombre nette et bien attachée sous la Golf, et ombre d'immeuble en bas à gauche.
- `course-cam1-1`, `course-cam1-2` : ombres d'immeubles sur la route et les trottoirs, ombre de la Golf au sol. On voit aussi
  des taches d'ombre plus loin sur la chaussée.
- `vitrine-*` : les 4 voitures sont complètes, avec des couleurs correctes et une ombre au sol. Sur les façades, on voit les
  ombres des arbres et des lampadaires.
- `ombres-a`, `ombres-b`, `ombres-coupees` : voir critères 2 et 3.
- Acné d'ombre (rayures), peter-panning (ombre décollée de l'objet), coupure au bord de la boîte : **rien vu**.
- Pas d'écran noir, pas de couleurs aberrantes, pas d'objet flottant.

## Remarques (non bloquantes)
- Les ombres des **arbres** sur le trottoir sont peu lisibles sur ces angles : le soleil les projette surtout vers les
  façades, ou bien les trottoirs sont déjà à l'ombre des immeubles. Le critère ne cite que les immeubles et les lampadaires.
- Les zones à l'ombre sont nettement plus sombres qu'avant le ticket. C'est voulu, car le soleil est désormais bloqué, mais
  il faudra en tenir compte pour l'éclairage de nuit (WFS-09).

## Limites
- Coût GPU mesuré sur lavapipe, avec la carte 2048 : à remesurer sur un vrai GPU mobile avec la carte 1024 (RG-02 et RG-06).
- Pas encore de mode nuit dans le jeu : `ombres-coupees` le simule (ombres forcées à zéro, soleil conservé).

## Historique
- 1er passage (commit 3ac3d0f, CI run 41547683867) : **REFUSÉ**. Aucune ombre sur la chaussée ni sur les trottoirs ; les
  triangles du sol étaient traités comme des faces arrière et leur normale était retournée vers le bas.
- 2e passage (commit 6743fbc, CI run 41550172280) : **VALIDÉ**. Correction dans `shaders/scene.frag`.
