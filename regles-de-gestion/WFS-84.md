# WFS-84 — [URGENT] Nuit : feux des voitures naturels (fini l'effet artificiel)
Épic : E4 · Priorité : P1 Urgent · Dépend de : WFS-09 ✅ · Taille : 1 jour · Retour de l'utilisateur (test réel sur téléphone, 2026-11-04)

## Besoin
La nuit, les feux des voitures (surtout ceux des voitures qu'on voit de derrière) sont « catastrophiques » : ils font artificiels. On veut des feux crédibles : un petit feu rouge net sur la voiture et une lueur douce, pas de gros halos plaqués.

## Règles de gestion (on veut que ça marche, pas la perfection)
- RG-01 : feu arrière = **petite source rouge nette** à la place des feux du modèle (matériau émissif, drapeau feu arrière du `.wfm`) + **lueur douce** (dégradé radial additif) dont la taille et l'intensité **baissent avec la distance** et restent petites (de l'ordre de la taille du feu ×2 à 3, pas un disque géant).
- RG-02 : au freinage, le feu passe du rouge sombre à rouge vif (pas de changement de taille brutal) ; en marche arrière, feu blanc discret.
- RG-03 : **un seul halo par feu** (la correction du double halo du trafic est dans WFS-81 : ne pas la refaire, vérifier que les deux tickets ne se contredisent pas à la fusion).
- RG-04 : pas de halo visible à travers la carrosserie ni au-dessus du toit ; halos coupés au-delà de ~120 m (silhouette lointaine : un simple point rouge).
- RG-05 : reflet discret des feux sur la route comme le jeu web (`MAT.tailGlow` : « reflet discret des feux sur la route »), seulement quand ils sont allumés ; pas de faisceau arrière.
- RG-06 : tout est réglable par quelques constantes (taille, intensité, distance) pour pouvoir ajuster vite après un test sur téléphone.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
`js/cars/models.js` (≈ lignes 414-420 et 480 : `glowGeo`, `MAT.tailGlow`, `tailGlowBrake`, `revGlow`, `opts.glow`) ; `js/world/renderer.js` (matériaux de lueur, `TEXS.radial`). Natif : `src/render/renderer.cpp` (`feuxVoiture`, halos WFS-09).

## Critères d'acceptation (captures du banc PC, nuit)
- [ ] `nuit-trafic-arriere` : voitures vues de derrière avec de petits feux rouges nets et une lueur douce, sans gros disque ; au freinage, plus vifs
- [ ] aucune régression sur les phares et lampadaires (WFS-09)

## Hors périmètre
Lampadaires (WFS-81), reflets de pluie (WFS-18).

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| P1 Urgent | WFS-84 | [URGENT] Nuit : feux des voitures naturels (petits feux nets, lueur douce, fini l'effet artificiel) | E4 | ✅ Fusionné (par confiance, QA de nuit à faire) | Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04 | WFS-09 ✅ | `feature/waforspeed-84` | [#70](https://github.com/wlfaleh/waforspeed_mobile/pull/70) |  |

Critères et notes de suivi :

**WFS-84 — [URGENT] Nuit : feux des voitures naturels** · E4 · priorité P1 Urgent · ✅ Fusionné le 2026-11-04 (PR #70, par confiance ; QA de nuit : `qa/Check.md`) — Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04, branche `feature/waforspeed-84` (règles : `regles-de-gestion/WFS-84.md`)
- [ ] petits feux rouges nets + lueur douce, pas de gros halos (coordonné avec WFS-81)
