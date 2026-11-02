# WFS-20 — Performances : LOD, tri des objets visibles en parallèle
Épic : E4 · Priorité : 16 · Dépend de : WFS-06 (✅), WFS-08 (✅)

## Besoin (qui, quoi, pourquoi)
Tenir 60 images/s sur un téléphone milieu de gamme avec la ville et le trafic, grâce aux niveaux de détail et à un
tri de visibilité fait sur les threads de travail.

## Règles de gestion
- RG-01 : trafic : voiture complète à moins de `LOD_CAR_M = 50` m (le skill indique 70 m : valeur du code = 50), silhouette légère au-delà (1 maillage, couleurs par sommet) ; feux de la silhouette allumés la nuit.
- RG-02 : décor : petits objets (`SMALL_DECOR` : barrières, parasols, poteaux, bancs, bornes…) non dessinés au-delà de `LOD_FAR_M = 200` m ; tronçons de décor construits/libérés selon la distance (`VIEW`).
- RG-03 : voitures du trafic hors champ (`rel < −20 m` ou au-delà de `VIEW×SEG_M + 30`) masquées sans être détruites ; maillages réutilisés (pool).
- RG-04 : piétons : niveaux à 16 m et 50 m, 8 maillages complets, 10 ombres au plus (WFS-19).
- RG-05 : le tri/culling par tronçon se fait par `parallelFor` sur les threads de `jobs()` ; le thread principal seul touche Vulkan ; résultat identique à un calcul séquentiel.
- RG-06 : objectifs : ≤ 150 appels de dessin en course (≤ 300 avec la ville) ; 60 i/s visés ; mesures avant/après dans le journal.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/world/scene.js` → `LOD_CAR_M`, `trafficMesh`, boucle de rendu du trafic ; `js/world/renderer.js` → `SMALL_DECOR`, `LOD_FAR_M`, `buildChunk(ci, lod)`, `ensureChunks`, `disposeChunk` ; `js/world/people.js` (`PED_LOD_M`) ; `models/*/*-lod.json` (versions allégées).
- Retours connus (skill §2-3) : fusion par matériau, instanciation, cacher plutôt que retirer, libérer ce qui sort ; tout objet nouveau respecte le LOD.

## Critères d'acceptation (captures du banc PC)
- [ ] captures avant/après identiques à l'œil sauf le lointain (silhouettes) ; pas de saut visible au changement de LOD
- [ ] journal : appels de dessin, triangles et ms avant/après ; gain mesuré du tri parallèle
- [ ] test de fidélité du trafic toujours à 0 écart

## Hors périmètre
Niveaux de qualité (WFS-15), nouveaux effets, mémoire des textures.
