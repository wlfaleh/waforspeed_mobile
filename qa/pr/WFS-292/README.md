# WFS-292 — captures avant / après (tools/qa/captures_pr.sh)

Branche `feature/waforspeed-292` comparée à `origin/main` (eb87d8e). Bancs : `effets`, `nuit`. Les « après » viennent de `tools/qa/captures_pr.sh`, coupé par la limite de 2 h, la machine étant très chargée. Les « avant » de `effets` viennent du même passage. Les « avant » de `phares-autres-ville-*` viennent d'un banc `nuit` de `main` réduit à ces scènes. `phares-autres-course5-go-haut` n'a pas d'« avant » valable : dans le banc réduit, l'adaptation de l'œil part d'un autre état (image blanche au départ, WFS-328).

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `fumee-derapage-jour` | ![avant](avant-fumee-derapage-jour.jpg) | ![après](apres-fumee-derapage-jour.jpg) |
| `fumee-derapage-jour-05s` | — | ![après](apres-fumee-derapage-jour-05s.jpg) |
| `fumee-derapage-jour-1s` | — | ![après](apres-fumee-derapage-jour-1s.jpg) |
| `fumee-derapage-nuit` | ![avant](avant-fumee-derapage-nuit.jpg) | ![après](apres-fumee-derapage-nuit.jpg) |
| `choc-etincelles` | ![avant](avant-choc-etincelles.jpg) | ![après](apres-choc-etincelles.jpg) |
| `accident-fumee` | ![avant](avant-accident-fumee.jpg) | ![après](apres-accident-fumee.jpg) |
| `accident-fumee-nuit` | ![avant](avant-accident-fumee-nuit.jpg) | ![après](apres-accident-fumee-nuit.jpg) |
| `phares-autres-ville-haut` | ![avant](avant-phares-autres-ville-haut.jpg) | ![après](apres-phares-autres-ville-haut.jpg) |
| `phares-autres-ville-face` | ![avant](avant-phares-autres-ville-face.jpg) | ![après](apres-phares-autres-ville-face.jpg) |
| `phares-autres-course5-go-haut` | — | ![après](apres-phares-autres-course5-go-haut.jpg) |
