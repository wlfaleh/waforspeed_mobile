# WFS-15 — Réglages de qualité selon le téléphone
Épic : E4 · Priorité : 14 · Dépend de : WFS-07

## Besoin (qui, quoi, pourquoi)
Le jeu doit rester fluide du téléphone d'entrée de gamme au haut de gamme : trois niveaux (bas, moyen, haut), choisis
automatiquement au premier lancement, modifiables dans la pause.

## Règles de gestion
- RG-01 : le web a 2 niveaux (`high` / `low`) ; le natif en ajoute un « moyen » : haut = tout activé, bas = comportement du web `low`.
- RG-02 : effets du niveau bas (web) : portée de vue `VIEW` 330 au lieu de 470 tronçons, brouillard lointain ≤ 420 m, pas de bloom ni d'étalonnage, pas d'ombres temps réel, pas de reflets temps réel de la route, façades sans PBR, `DPR` ≤ 1,25 (haut : 1,75 ordinateur, 1,5 mobile).
- RG-03 : résolution dynamique : toutes les 2 s de course, `fps < 40` → échelle −0,1 (plancher 0,6) ; `fps > 57` → +0,1 (plafond 1).
- RG-04 : si l'échelle est à 0,6 et `fps < 30` : passage automatique au niveau inférieur une seule fois, message « Graphismes rapides pour plus de fluidité ».
- RG-05 : choix de l'utilisateur mémorisé et prioritaire ; tout nouvel effet (ombres, bloom, route mouillée) dépend du niveau.
- RG-06 : mesures au journal pour chaque niveau : ms CPU/GPU, appels de dessin, triangles.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/ui/screens.js` → `setQuality`, `watchPerf`, `qualityChosen`, `setQualityLabel` ; `js/world/renderer.js` → `onResize` (DPR, `resScale`), `VIEW`, `setupComposer` ; `js/world/maps.js` (`fogFar`).
- Retours connus (skill `waforspeed-3d`, §8) : `pixelRatio ≤ 1.5` sur mobile ; mesurer les premières secondes de course ; ne jamais changer ombres/résolution à chaque image, seulement au changement de niveau.

## Critères d'acceptation (captures du banc PC)
- [ ] trois captures de la même scène (bas / moyen / haut) avec des différences visibles et documentées
- [ ] journal : le niveau bas coûte nettement moins (ms) que le haut
- [ ] test CPU de la logique d'ajustement (fps simulés → échelle et niveau attendus)

## Hors périmètre
Contenu des effets eux-mêmes (WFS-07, 16, 17), détection fine du modèle de téléphone.


## Périmètre élargi (décision de l'utilisateur 2026-11-03 : tickets plus gros, moins de builds)
Ce ticket absorbe : WFS-20, WFS-17. **Lire et appliquer aussi** `regles-de-gestion/WFS-20.md`, `regles-de-gestion/WFS-17.md` (toutes leurs règles et critères). Une seule branche, une seule CI à la fin ; livrer par morceaux sur la branche sans lancer de CI intermédiaire.

## Recadrage du PO (2026-11-04, version stable : fluidité sur le téléphone de l'utilisateur)
**Déjà fait, ne pas refaire** : les 3 niveaux Basse / Moyenne / Haute existent (WFS-41, `src/ui/ecrans/reglages.cpp` →
`appliquerQualite` : `renderScale` 0,5 / 0,75 / 1, ombres coupées en Basse, choix mémorisé) ; lignes de vitesse et vignette
existent (WFS-65, `effets_*`). Ce ticket **branche la performance derrière ces niveaux**, dans cet ordre de valeur
(si le temps manque, livrer 1 à 3 et noter le reste au Journal) :
1. **Choix automatique au premier lancement + résolution dynamique + rétrogradation** (RG-03, RG-04 ci-dessus) : logique
   pure dans un nouveau fichier `src/render/qualite_auto.{h,cpp}` (testée CPU : fps simulés → échelle et niveau) ; si le
   joueur a déjà choisi (fichier de réglages présent), son choix gagne (RG-05).
2. **LOD et masquage** (WFS-20 RG-01 à RG-03) : trafic au-delà de 50 m en modèle allégé, petits décors coupés au-delà de
   200 m, voitures hors champ non dessinées ; compter les appels de dessin au journal (objectif RG-06 de WFS-20).
3. **Bloom** (WFS-17 RG-01, RG-06) dans `shaders/post.frag` : Moyenne et Haute seulement ; feux, néons et phares brillent.
4. TAA et flou de mouvement : **hors périmètre de ce ticket** (finitions, seulement si l'utilisateur les demande).

**Zone et partage avec WFS-09 (nuit, même épic, en cours)** — exception justifiée : 2e ticket E4, fichiers séparés.
- WFS-15 possède : `shaders/post.*`, nouveaux fichiers `src/render/qualite_*`, `src/render/lod_*`, `tests/test_qualite*`.
- WFS-09 possède : `shaders/scene*`, `shaders/sky*`, les lumières. **Ne pas toucher `scene.frag` ni l'éclairage.**
- `src/render/renderer.cpp` / `renderer.h` : accroches courtes seulement, annoncées au Journal ; synchroniser avec `main`
  souvent (`git fetch origin && git merge origin/main`), garder les deux modifications en cas de conflit.
- `src/ui/ecrans/reglages.cpp` (E8) : accroche ≤ 10 lignes pour appeler le choix automatique ; aucune nouvelle ligne de menu.

**Critères d'acceptation recadrés (captures du banc PC et journal)**
- [ ] `reglages-basse` / course en Moyenne / Haute : différences visibles (bloom des feux et néons en Moyenne/Haute, absent en Basse)
- [ ] journal du banc : niveau choisi automatiquement au premier lancement, échelle de résolution qui baisse sur fps simulés bas puis rétrogradation unique avec le message « Graphismes rapides pour plus de fluidité »
- [ ] journal : appels de dessin et ms par niveau ; Basse nettement moins coûteuse que Haute ; trafic lointain en LOD
- [ ] test CPU de `qualite_auto` vert ; autres captures du banc sans régression ; aucun plantage

**À réutiliser (vérifié sur `wlfaleh/waaps`, `waforspeed/`)** : `js/ui/screens.js` → `setQuality` (l. 456), `watchPerf` (l. 464,
seuils et rétrogradation unique) ; `js/world/scene.js` → `LOD_CAR_M = 50` (l. 35), `trafficMesh(car, lointain)` (l. 233,
silhouette au-delà de 50 m), valeurs du bloom `ENV.bloom` (l. 294) ; `js/world/renderer.js` → `setupComposer`, `onResize`.
Modèles : les versions `-lod` de `models/` déjà converties pour le trafic (WFS-13) ; sinon silhouette simple, ne pas
reconvertir les 20 voitures.
