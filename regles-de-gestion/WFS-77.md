# WFS-77 — Les 20 voitures sans défaut visible (peinture unie Giulia et Elantra, roues de la GT86, contrôle des 20)
Épic : E5 (Interface et contenu) · Priorité : P2 Haute · Dépend de : WFS-13 ✅, WFS-60 ✅

## Besoin (qui, quoi, pourquoi)
Le joueur choisit sa voiture au garage puis la conduit : c'est l'objet qu'il regarde le plus. Après WFS-13 (16 voitures
ajoutées) deux défauts se voient : la **Giulia** et l'**Elantra** portent un motif de camouflage (texture de base du modèle
gardée sous la peinture) et la **GT86** n'a pas de roues animées (« roues non trouvées, modèle inchangé (WFS-60) » au
chargement). Ce ticket rend les 20 voitures propres, d'un seul coup, et le prouve par une planche de contrôle.
Pas de finitions au-delà : ce qui compte, c'est que rien ne choque au garage ni en course.

## Règles de gestion
- RG-01 (peinture unie) : un matériau « peinture » (drapeau 1) s'affiche en **couleur unie métallisée** de la peinture choisie,
  comme `repaintMat` du jeu web : métal **0,45**, rugosité **0,32**, cartes de métal / rugosité ignorées, aucune émission.
  Si sa **texture de base** est un motif (camouflage, habillage, logo), elle est **retirée** à la conversion (ou neutralisée
  en blanc) pour ce matériau. Résultat : Giulia et Elantra prennent la teinte choisie (Rouge, Turquoise…) sans aucun motif.
- RG-02 (bon matériau peinture) : utiliser les mêmes noms de matériau que le jeu web (`js/cars/catalog.js`, `MODEL_SPECS`) :
  Giulia `^EXT_carpaint$`, GT86 `^body\.001$`, Elantra `^HHyundai_ElantraNRewardRecycled_2022Paint_Material1$`. Si un
  autre matériau de carrosserie reste visible non repeint, l'ajouter à l'expression de la voiture (journal de conversion :
  `peinture: …` ne doit jamais dire `AUCUNE`).
- RG-03 (roues de la GT86) : la GT86 doit avoir **4 roues séparées** au chargement (log « 4 roues séparées … (WFS-60) »),
  qui tournent et braquent comme les 19 autres. Corriger **à la conversion** (`tools/convert_car.mjs` : drapeau 128 « roue »
  posé sur les bonnes pièces, expression de roue propre à la voiture, ou découpe en 4 coins comme `splitWheels` du web),
  puis reconvertir `assets/cars/gt86.wfm`. Pas de changement du chargeur `src/render/model_loader.cpp` (zone E4 occupée),
  sauf accroche ≤ 10 lignes annoncée au Journal si aucune autre voie n'existe.
- RG-04 (contrôle des 20) : vérifier les **20 voitures** (pas seulement les 3 citées) avec la même grille : peinture unie
  qui suit la couleur choisie, 4 roues séparées et au sol, feux arrière non blancs, voiture dans le bon sens, rien de
  transparent ou de manquant. Tout autre défaut de ce type trouvé est corrigé dans ce ticket (même outil, même zone).
- RG-05 (preuve) : le banc produit une **planche** des 20 voitures (`vitrines-planche`, 5 × 4 vignettes, nom sous chaque
  voiture) en Turquoise, et le journal du banc liste pour chaque voiture : `roues 4/4`, `peinture <noms>`. Réutiliser le job
  `vitrines` de WFS-13 plutôt que d'en créer un autre.
- RG-06 (poids) : chaque `.wfm` reconverti reste sous **9 Mo** (RG-16 de WFS-13) ; licences et crédits inchangés.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- `js/cars/models.js` : `repaintMat` (l. 229, réglages de la peinture unie), `MODEL_RX.paint` / `MODEL_RX.notPaint`
  (l. 11-12), `WHEEL_RX` (l. 349 : `wheel|roue|tire|tyre|pneu|rim` seul, pas « trim »), `splitWheels` / `looksLikeWheel`
  (l. 268-340 : découpe d'une pièce qui regroupe les 4 roues, par coin, autour de la médiane).
- `js/cars/catalog.js` : `MODEL_SPECS` (l. 70-77 : expressions de peinture et `rotateY` de giulia, gt86, elantra).
- Modèles : `models/alfa-giulia/`, `models/toyota-gt86/`, `models/hyundai-elantra-n/` (et leur version `-lod`, dont les
  pièces sont déjà nommées `wfs_paint` / `wfs_wheel` par `make-lod.mjs` : utile pour retrouver les roues de la GT86).
- Natif existant : `tools/convert_car.mjs` (drapeaux 1 peinture, 128 roue), job `vitrines` du banc (WFS-13), catalogue
  `src/game/cars.h` et `src/ui/garage.cpp` : ne rien recréer.

## Critères d'acceptation (vérifiables sur les captures du banc PC)
- [ ] `vitrine-giulia-0` et `vitrine-elantra-0` : carrosserie de couleur unie (aucun motif de camouflage)
- [ ] `garage-peinture-*` sur Giulia ou Elantra : la teinte change avec la peinture choisie
- [ ] journal : « gt86.wfm : 4 roues séparées » ; plus aucune ligne « roues non trouvées » pour les 20 voitures
- [ ] `vitrines-planche` : 20 voitures propres, dans le bon sens, 4 roues au sol, peinture Turquoise sur chacune
- [ ] autres captures inchangées ; `.wfm` < 9 Mo ; téléphone : Giulia, Elantra et GT86 propres au garage et en course

## Hors périmètre
- Nouvelles voitures, nouveaux modèles, intérieurs détaillés, reflets (E4), sons des voitures (E6).
- Toute modification du chargeur ou des shaders (E4), sauf accroche ≤ 10 lignes annoncée.

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| P2 Haute | WFS-77 | Les 20 voitures sans défaut visible (peinture unie Giulia et Elantra, roues de la GT86, contrôle des 20) | E5 | ✅ Fusionné (PR #57, sur ordre de l'utilisateur ; QA de nuit à faire) | Claude (agent 5, session 011cpqZaDiS2u), 2026-11-04 | WFS-13 ✅, WFS-60 ✅ | `feature/waforspeed-77` | [#57](https://github.com/wlfaleh/waforspeed_mobile/pull/57) |  |

Critères et notes de suivi :

**WFS-77 — Les 20 voitures sans défaut visible** · E5 · P2 Haute · ✅ Fusionné (PR #57, 0f77ae4, sur ordre de l'utilisateur ; QA de nuit à faire) — Claude (agent 5, session 011cpqZaDiS2u), 2026-11-04, branche `feature/waforspeed-77` (règles : `regles-de-gestion/WFS-77.md`)
- [ ] `vitrine-giulia-0` / `vitrine-elantra-0` : couleur unie, aucun camouflage ; la teinte suit la peinture choisie au garage
- [ ] journal : « gt86.wfm : 4 roues séparées », plus aucune « roues non trouvées » ; `vitrines-planche` : 20 voitures propres (sens, 4 roues au sol, Turquoise) ; `.wfm` < 9 Mo
