# WFS-42 — Vitrages et jantes : trame de points (lunette arrière de la Golf, rayons des jantes)
Épic : E4 (Rendu et performances) · Priorité : 8 · Dépend de : WFS-32 (✅)

## Besoin (qui, quoi, pourquoi)
Restes du rapport QA de WFS-32 : après la correction des carrosseries, il reste une trame régulière de petits points clairs
sur la **lunette arrière de la Golf** (identique avant / après WFS-32) et des points brillants sur les **rayons des jantes**
(911). Le joueur les voit en vitrine et en course derrière la voiture.

## Règles de gestion
- RG-01 : identifier la cause avant de corriger (texture ou UV du modèle, transparence / mélange du vitrage, reflet
  spéculaire trop fin sur des triangles plus petits qu'un pixel, normales) et la noter au Journal avec la capture.
- RG-02 : correction dans le rendu (`src/render/**` sauf `ui_*`, `shaders/scene*`). Si la cause est dans le modèle
  (`assets/cars/**`, `tools/convert_car.mjs`, zone E5) : ne pas le modifier, ouvrir un ticket E5 via le PO.
- RG-03 : les vitres restent teintées et réfléchissantes, les jantes métalliques (pas d'aspect mat) ; même approche
  d'anticrénelage des reflets que WFS-32 (`scene.frag`) si c'est un crénelage spéculaire.
- RG-04 : les lignes pointillées des jointures de carrosserie ne font **pas** partie de ce ticket (TAA, WFS-17).

## À réutiliser
- Rapport `qa/rapports/WFS-32.md` (comptages de points, captures), correction de WFS-32 dans `shaders/scene.frag` et `post.frag`.
- Jeu web (`wlfaleh/waaps`, `waforspeed/js/`) : réglages des matériaux vitrage et jantes des modèles glTF.

## Critères d'acceptation
- [ ] Cause notée au Journal avec la capture qui la montre
- [ ] `vitrine-golf-0` (agrandie sur la lunette arrière) : plus de trame de points ; vitre teintée et réfléchissante
- [ ] `vitrine-p911-1` (agrandie sur les jantes) : plus de points brillants isolés sur les rayons
- [ ] Aucune autre capture dégradée ; coût GPU au journal du banc inchangé à ±0,5 ms

## Hors périmètre
Jointures de carrosserie (WFS-17), modification des modèles (E5).
