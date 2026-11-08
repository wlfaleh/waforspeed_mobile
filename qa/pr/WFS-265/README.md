# WFS-265 — captures avant / après

Branche `feature/waforspeed-265` (synchronisée avec `main` f139b49b) comparée à `main` au même point. `voiture-distance-*` : banc `qualite` (tools/qa/captures_pr.sh), même voiture du trafic de 10 à 180 m, Haute et Basse. `lod-*` : banc `WFS_BANC=lod` (nouveau, `src/render/bench_lod.cpp` ; « avant » = `main` f139b49b + ce seul banc) : vues au téléobjectif à 50, 150 et 300 m des 4 cartes (Haute et Basse), traversée du fondu d'effacement en Basse (70 à 115 m), feux du trafic de nuit en Basse à 10, 25 et 50 m (voiture du joueur au premier plan, identique avant / après).

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `voiture-distance-10` | ![avant](avant-voiture-distance-10.jpg) | ![après](apres-voiture-distance-10.jpg) |
| `voiture-distance-10-basse` | ![avant](avant-voiture-distance-10-basse.jpg) | ![après](apres-voiture-distance-10-basse.jpg) |
| `voiture-distance-25` | ![avant](avant-voiture-distance-25.jpg) | ![après](apres-voiture-distance-25.jpg) |
| `voiture-distance-25-basse` | ![avant](avant-voiture-distance-25-basse.jpg) | ![après](apres-voiture-distance-25-basse.jpg) |
| `voiture-distance-50` | ![avant](avant-voiture-distance-50.jpg) | ![après](apres-voiture-distance-50.jpg) |
| `voiture-distance-50-basse` | ![avant](avant-voiture-distance-50-basse.jpg) | ![après](apres-voiture-distance-50-basse.jpg) |
| `voiture-distance-60` | ![avant](avant-voiture-distance-60.jpg) | ![après](apres-voiture-distance-60.jpg) |
| `voiture-distance-60-basse` | ![avant](avant-voiture-distance-60-basse.jpg) | ![après](apres-voiture-distance-60-basse.jpg) |
| `voiture-distance-80` | ![avant](avant-voiture-distance-80.jpg) | ![après](apres-voiture-distance-80.jpg) |
| `voiture-distance-80-basse` | ![avant](avant-voiture-distance-80-basse.jpg) | ![après](apres-voiture-distance-80-basse.jpg) |
| `voiture-distance-120` | ![avant](avant-voiture-distance-120.jpg) | ![après](apres-voiture-distance-120.jpg) |
| `voiture-distance-120-basse` | ![avant](avant-voiture-distance-120-basse.jpg) | ![après](apres-voiture-distance-120-basse.jpg) |
| `voiture-distance-180` | ![avant](avant-voiture-distance-180.jpg) | ![après](apres-voiture-distance-180.jpg) |
| `voiture-distance-180-basse` | ![avant](avant-voiture-distance-180-basse.jpg) | ![après](apres-voiture-distance-180-basse.jpg) |
| `lod-campagne-150` | ![avant](avant-lod-campagne-150.jpg) | ![après](apres-lod-campagne-150.jpg) |
| `lod-campagne-150-basse` | ![avant](avant-lod-campagne-150-basse.jpg) | ![après](apres-lod-campagne-150-basse.jpg) |
| `lod-campagne-300` | ![avant](avant-lod-campagne-300.jpg) | ![après](apres-lod-campagne-300.jpg) |
| `lod-campagne-300-basse` | ![avant](avant-lod-campagne-300-basse.jpg) | ![après](apres-lod-campagne-300-basse.jpg) |
| `lod-campagne-50` | ![avant](avant-lod-campagne-50.jpg) | ![après](apres-lod-campagne-50.jpg) |
| `lod-campagne-50-basse` | ![avant](avant-lod-campagne-50-basse.jpg) | ![après](apres-lod-campagne-50-basse.jpg) |
| `lod-circuit-150` | ![avant](avant-lod-circuit-150.jpg) | ![après](apres-lod-circuit-150.jpg) |
| `lod-circuit-150-basse` | ![avant](avant-lod-circuit-150-basse.jpg) | ![après](apres-lod-circuit-150-basse.jpg) |
| `lod-circuit-300` | ![avant](avant-lod-circuit-300.jpg) | ![après](apres-lod-circuit-300.jpg) |
| `lod-circuit-300-basse` | ![avant](avant-lod-circuit-300-basse.jpg) | ![après](apres-lod-circuit-300-basse.jpg) |
| `lod-circuit-50` | ![avant](avant-lod-circuit-50.jpg) | ![après](apres-lod-circuit-50.jpg) |
| `lod-circuit-50-basse` | ![avant](avant-lod-circuit-50-basse.jpg) | ![après](apres-lod-circuit-50-basse.jpg) |
| `lod-plage-150` | ![avant](avant-lod-plage-150.jpg) | ![après](apres-lod-plage-150.jpg) |
| `lod-plage-150-basse` | ![avant](avant-lod-plage-150-basse.jpg) | ![après](apres-lod-plage-150-basse.jpg) |
| `lod-plage-300` | ![avant](avant-lod-plage-300.jpg) | ![après](apres-lod-plage-300.jpg) |
| `lod-plage-300-basse` | ![avant](avant-lod-plage-300-basse.jpg) | ![après](apres-lod-plage-300-basse.jpg) |
| `lod-plage-50` | ![avant](avant-lod-plage-50.jpg) | ![après](apres-lod-plage-50.jpg) |
| `lod-plage-50-basse` | ![avant](avant-lod-plage-50-basse.jpg) | ![après](apres-lod-plage-50-basse.jpg) |
| `lod-ville-150` | ![avant](avant-lod-ville-150.jpg) | ![après](apres-lod-ville-150.jpg) |
| `lod-ville-150-basse` | ![avant](avant-lod-ville-150-basse.jpg) | ![après](apres-lod-ville-150-basse.jpg) |
| `lod-ville-300` | ![avant](avant-lod-ville-300.jpg) | ![après](apres-lod-ville-300.jpg) |
| `lod-ville-300-basse` | ![avant](avant-lod-ville-300-basse.jpg) | ![après](apres-lod-ville-300-basse.jpg) |
| `lod-ville-50` | ![avant](avant-lod-ville-50.jpg) | ![après](apres-lod-ville-50.jpg) |
| `lod-ville-50-basse` | ![avant](avant-lod-ville-50-basse.jpg) | ![après](apres-lod-ville-50-basse.jpg) |
| `lod-fondu-ville-70-basse` | ![avant](avant-lod-fondu-ville-70-basse.jpg) | ![après](apres-lod-fondu-ville-70-basse.jpg) |
| `lod-fondu-ville-80-basse` | ![avant](avant-lod-fondu-ville-80-basse.jpg) | ![après](apres-lod-fondu-ville-80-basse.jpg) |
| `lod-fondu-ville-85-basse` | ![avant](avant-lod-fondu-ville-85-basse.jpg) | ![après](apres-lod-fondu-ville-85-basse.jpg) |
| `lod-fondu-ville-90-basse` | ![avant](avant-lod-fondu-ville-90-basse.jpg) | ![après](apres-lod-fondu-ville-90-basse.jpg) |
| `lod-fondu-ville-100-basse` | ![avant](avant-lod-fondu-ville-100-basse.jpg) | ![après](apres-lod-fondu-ville-100-basse.jpg) |
| `lod-fondu-ville-105-basse` | ![avant](avant-lod-fondu-ville-105-basse.jpg) | ![après](apres-lod-fondu-ville-105-basse.jpg) |
| `lod-fondu-ville-110-basse` | ![avant](avant-lod-fondu-ville-110-basse.jpg) | ![après](apres-lod-fondu-ville-110-basse.jpg) |
| `lod-fondu-ville-115-basse` | ![avant](avant-lod-fondu-ville-115-basse.jpg) | ![après](apres-lod-fondu-ville-115-basse.jpg) |
| `lod-nuit-trafic-10-basse` | ![avant](avant-lod-nuit-trafic-10-basse.jpg) | ![après](apres-lod-nuit-trafic-10-basse.jpg) |
| `lod-nuit-trafic-25-basse` | ![avant](avant-lod-nuit-trafic-25-basse.jpg) | ![après](apres-lod-nuit-trafic-25-basse.jpg) |
| `lod-nuit-trafic-50-basse` | ![avant](avant-lod-nuit-trafic-50-basse.jpg) | ![après](apres-lod-nuit-trafic-50-basse.jpg) |
