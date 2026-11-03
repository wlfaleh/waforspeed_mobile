# QA WFS-44 : Point d'extension « scène dédiée par écran »

**Verdict : VALIDÉ** (banc d'essai PC, règle WFS-31), 1er passage

- Construction regardée : commit `b9771e0` (branche `feature/waforspeed-44`, PR #15). Release `apercu-waforspeed-44`
  (texte « Branche feature/waforspeed-44, commit b9771e0 », mise à jour le 2026-11-03 09:50 UTC).
  Run Actions 41611888607 (push de `b9771e0`) : jobs `android`, `banc-essai`, `publication` verts.
- Référence : la Release `apercu` de `main` (commit `388012c`, qui ne diffère de la base que par `TACHES.md`) ne contient
  **que l'APK** : son job `banc-essai` a été annulé (run 41611983733), aucune capture. Référence retenue :
  Release `apercu-waforspeed-34` (commit `4f268ca`). La base exacte de la branche est `5d7f875` (`git merge-base`) ;
  entre `5d7f875` et `4f268ca` ne changent que des fichiers de consignes (`.claude/agents`, `.claude/skills`, `CLAUDE.md`),
  aucun fichier de code, de shader ni de ressource : cette référence vaut la base.

## Critère 1 : captures identiques à `main` (aucune scène dédiée par défaut, RG-05)

Comparaison pixel par pixel (PIL, `ImageChops.difference(...).getbbox()`) des 28 PNG communs :
**28 / 28 identiques**, tailles identiques (`course-cam0/1-*`, `trafic-cam0/1`, `trafic-choc`, `ombres-a/b/coupees`,
`vitrine-*` des 4 voitures, `ecran-menu-vide`, `texte-essai-1280/2400`, `moteur-courbe`).

Journal du job `banc-essai` :
- `test_track` (4 cartes, golf 10 s : 180.8 km/h), `test_systems` (« systèmes : 2, ordre : 12 »), `test_traffic`
  (« mode exact : 39 voitures comparées au jeu web, 0 écart(s) », mode parallèle déterministe, choc arrière → OK)
  et `test_audio` (« → OK ») passent.
- Le banc va jusqu'au bout (toutes les captures, `scene-dediee-vide.png`, `moteur.wav`, `moteur-courbe.png` listés).
  Le système `SceneEssai (ordre 900)` est bien enregistré, inactif sauf pour la capture dédiée.

## Critère 2 : nouvelle capture `scene-dediee-vide`

- Image 1280×720 non déformée : un cube orange seul au centre, faces nettes, aucun morceau qui flotte.
- **Fond** (mesuré hors du cube, ~800 000 pixels) : moyenne (6.86, 6.86, 12.99), min (6, 6, 12), max (8, 8, 14) pour
  `#07070c` = (7, 7, 12) attendu : écart de 1/255 au plus en moyenne, uni à l'œil (grain de la passe finale, le même que
  sur les autres captures). Conforme.
- **Rien d'autre** : ni route, ni trottoir, ni ville, ni ciel bleu, ni trafic, ni voiture du joueur, ni ombre de contact,
  ni commande tactile.
- **Caméra FOV 36°** : projection recalculée avec l'œil (3.4, 2, 3.4), la cible (0, 0.5, 0) et un FOV vertical de 36° :
  coins attendus (640, 290), (640, 529), (640, 223), (480, 252), (800, 252), retrouvés exactement sur l'image.
- Observation (pas une anomalie) : la face du cube à l'ombre est presque noire (6, 4, 2) ; c'est un cube de repère, l'éclairage
  du garage viendra avec WFS-45 / WFS-11.

## Critère 3 : documentation

`.claude/skills/waforspeed-natif/SKILL.md` (section « Scène dédiée d'un écran ») et `references/architecture.md`
(section « Scène dédiée (WFS-44) ») : méthode, champs de `fp`, `SceneDediee`, `GameView::sceneDediee`, exemple
`src/core/scene_essai.cpp`. Conforme, sur le modèle de WFS-30 / WFS-36.

## Relecture du diff au regard des règles de gestion

- RG-01 : `System::scene(FrameParams&, SceneDediee&, const GameView&)` renvoie `false` par défaut ; `Game::render` l'appelle
  dans l'ordre des systèmes, s'arrête au premier `true`, **avant** `beginFrame(fp)`. OK.
- RG-02 : `fp` est rempli avec les valeurs habituelles avant l'appel ; le système de test règle vue, projection, `camPos`,
  ciel = fond uni, brouillard, soleil, ambiance, exposition, ombres. OK.
- RG-03 : `SceneDediee { route, joueur, commandes }` tous à `false` par défaut ; route, voiture + ombre de contact et
  commandes conditionnés. OK (les commandes ne sont pas visibles sur la capture qui est prise hors écran Course, mais la
  condition est bien dans le code).
- RG-04 : `GameView::sceneDediee` ; `systemsRender` reçoit la caméra choisie (`fp.camPos`, `s.cible`, `fp.view`, `fp.proj`) ;
  ville et trafic : une ligne chacun (`if (... g.sceneDediee) return;`). OK.
- RG-05 : chemin inchangé sans scène dédiée ; 28 captures identiques. OK.
- RG-06 : aucun fichier sous `src/render/**` modifié. Accroche de 2 lignes dans `src/platform/desktop_main.cpp` (banc). OK.

## Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| — | — | — | Aucune anomalie |
