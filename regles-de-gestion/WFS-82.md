# WFS-82 — [STABLE] Performances téléphone : masquage hors champ et au loin, moins d'appels de dessin
Épic : E4 · Priorité : P2 Haute · Dépend de : WFS-15 ✅, WFS-67 ✅

## Besoin (qui, quoi, pourquoi)
Le joueur, sur son téléphone, doit avoir une course fluide (version stable). Au banc de WFS-15 (nuit, ville), la qualité
Basse fait encore **346 appels de dessin par image** (Moyenne 607), au-dessus de la cible **≤ 300** de WFS-67.
Deux points de WFS-15 n'ont pas été faits : voitures hors champ non dessinées, petits décors coupés au loin.
Ce ticket les termine **dans le renderer (E4), sans toucher aux fichiers du monde (E2, occupé par WFS-18)**.

## Règles de gestion
- RG-01 : **masquage hors champ** (frustum) : avant chaque appel de dessin d'objet de la scène (décor, voitures du trafic,
  voiture du joueur, personnages), tester sa sphère englobante (centre + rayon, calculés une fois au chargement du maillage
  et transformés par la matrice de l'objet) contre les 6 plans de la caméra ; hors champ → pas d'appel de dessin.
- RG-02 : **masquage au loin des petits objets** : un objet dont le rayon englobant est < 3 m n'est pas dessiné au-delà de
  **200 m** de la caméra en Moyenne/Haute et **150 m** en Basse ; objet moyen (3 à 10 m) au-delà de la portée du brouillard
  lointain (≤ 420 m en Basse, RG-02 de WFS-15). Les grands objets (routes, sol, immeubles > 10 m) ne sont jamais coupés par distance.
- RG-03 : passe d'ombres (Moyenne/Haute) : même test, contre le volume de la lumière (ou à défaut la distance des ombres) ;
  rien d'invisible dans l'ombre ne doit disparaître de l'image principale.
- RG-04 : si la cible n'est pas atteinte avec RG-01 à RG-03 : **regrouper les objets répétés identiques** (même maillage, même
  matériau : arbres, lampadaires, poteaux, piétons) en un appel instancié par lot, dans le renderer.
- RG-05 : **cibles mesurées au banc, scène nuit ville (même scène que WFS-15)** : Basse **≤ 300** appels de dessin, Moyenne
  **≤ 450**, Haute sans hausse ; valeurs avant / après, triangles et ms par niveau notés au Journal.
- RG-06 : aucun objet ne doit « clignoter » ou apparaître brutalement à moins de 150 m devant la voiture ; les phares, halos
  et feux du trafic (WFS-67) suivent leur voiture (masqués avec elle, jamais seuls).
- RG-07 : compteur d'appels de dessin et d'objets masqués exposé au banc (ligne de journal par image mesurée), logique
  de masquage pure (plans de caméra, test sphère, règle de distance) dans un nouveau fichier testé CPU.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Rien à porter en code : le jeu web s'appuie sur le masquage automatique de three.js (`Object3D.frustumCulled`, sphère
  englobante de `BufferGeometry.computeBoundingSphere`) : on reproduit ce comportement, pas de code à copier.
- Valeurs à garder : `js/world/scene.js` → `LOD_CAR_M = 50` (silhouette du trafic au-delà de 50 m, déjà portée par
  WFS-15/67, ne pas refaire) ; `js/world/maps.js` → `fogFar` (portée du brouillard, sert de limite des objets moyens) ;
  `js/ui/screens.js` → niveau `low` (`VIEW` 330 tronçons).
- Ne pas recréer : niveaux de qualité (`src/ui/ecrans/reglages.cpp`, `appliquerQualite`), `src/render/qualite_*` et le
  banc `qualite-*` de WFS-15 : on s'y branche.

## Zone de code
- E4 : `src/render/**` (sauf `ui_*`) ; nouveaux fichiers `src/render/masquage.{h,cpp}` ; `tests/test_masquage*.cpp`
  (fichier de test neuf, accroche CMake ≤ 10 lignes annoncée au Journal).
- Interdit : `src/world/**` (E2, WFS-18 en cours), `shaders/world_*`, `src/game/traffic_*` (sauf accroche ≤ 10 lignes
  annoncée au Journal si le trafic dessine hors du renderer).

## Critères d'acceptation (captures du banc PC et journal)
- [ ] journal du banc, nuit ville : Basse ≤ 300 appels de dessin, Moyenne ≤ 450, Haute pas plus qu'avant ; avant/après, ms et triangles notés au Journal
- [ ] captures `qualite-*` et partie1 : aucun objet manquant devant la voiture, aucune différence visible avec `main` à moins de 150 m
- [ ] trafic : phares et feux restent attachés à leur voiture ; aucune voiture qui disparaît dans le champ
- [ ] test CPU `masquage` vert (sphère dedans / dehors / à cheval, coupure par distance selon le rayon et le niveau)
- [ ] aucun plantage, autres captures du banc sans régression

## Hors périmètre
Occlusion (objets cachés derrière les immeubles), nouveau LOD des modèles, TAA, flou de mouvement, réglages de menu.
