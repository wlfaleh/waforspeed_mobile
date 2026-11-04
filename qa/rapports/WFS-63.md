# QA WFS-63 : point d'extension, maillages animés par squelette (skinning)

## Tour 1 (2026-11-03 21:42:36 UTC)

**Verdict : VALIDÉ.** Aucune anomalie, ni bloquante ni mineure. **Fusion autorisée.**

- Construction regardée : PR #39, `feature/waforspeed-63`, commit `0eb5465` (synchronisé avec main après WFS-69), run 41722597629 vert
  (android, banc-essai partie1 / partie2 / texte / ecrans-2400, publication). Release `apercu-waforspeed-63` (21:39:04Z), 73 captures.
- Comparaison : toutes les captures comparées pixel par pixel à celles de `apercu-waforspeed-69` (main, commit `d375d88`).
  Depuis la base de la branche, main n'a reçu que de la documentation (TACHES, journal, règles WFS-70 / WFS-71).

### Captures du ticket

| Capture | Constat |
|---|---|
| `squelette-essai` | `homme` debout en `Idle_Loop` (0,50 s) sur la piste d'essai, de face. Proportions justes (tête, buste, bras, jambes, pieds), aucun sommet étiré ni morceau qui flotte, coiffure en place sur la tête. Éclairage PBR identique aux voitures (soleil à droite, peau et short ombrés), ombre portée au sol vers la gauche, cohérente avec `ombres-a` / `vitrine-*`. |
| `squelette-marche-0` | `Walk_Loop` à 0,10 s : jambe gauche (à l'écran) en avant, bras droit en avant. Pose de marche naturelle. |
| `squelette-marche-1` | `Walk_Loop` à 0,75 s : jambes et bras inversés par rapport à `marche-0`. **Poses nettement différentes**, critère rempli. |
| `squelette-8` | 8 instances côte à côte, chacune à une phase de marche différente, toutes entières, ombrées, sans déformation. |

### Règles de gestion

| Règle | Constat |
|---|---|
| RG-01 | `SkinVertex`, `SkinnedMesh`, `makeSkinnedMesh`, `drawSkinned` dans `renderer.h` (64 os, 16 instances par image) ; `Mesh`, `Model` et `draw` inchangés. |
| RG-02 | `scene_skin.vert` / `shadow_skin.vert` avec `scene.frag` : même matériau, mêmes ombres (vues sur les captures). |
| RG-03 | `tools/convert_people.mjs`, `assets/people/homme.wfp` et `femme.wfp`, licence copiée dans `assets/licences/personnages-quaternius.txt`. Aucun personnage recréé. |
| RG-04 | `src/render/squelette.*` : lecture CPU, 0,009 à 0,010 ms par instance au journal. |
| RG-05 | Journal du banc : « squelette : 8 instances (corps + coiffure), échantillonnage CPU 0.030 ms par image, image complète 184.7 ms (banc, GPU logiciel) ». Le coût est mesuré ; 8 instances visées, 16 au plus. |
| Doc | Section « Personnages animés par squelette (point d'extension WFS-63) » dans le skill `waforspeed-natif`. |

### Non-régression

- 69 captures communes avec main : **63 identiques au pixel près**. 6 captures (`chargement-fondu-2400`, `encoche-hud-2400`, `hud-2400`,
  `menu-1`, `texte-essai-1280`, `texte-essai-2400`) ont des écarts de **1 niveau sur 255 au plus**, et aucun pixel ne s'écarte de plus de 2 niveaux.
  Ce sont des écrans d'interface 2D que le ticket ne touche pas : c'est du bruit du rendu logiciel, invisible à l'œil. Ce n'est pas une anomalie.
- Aucune capture manquante. Les 4 captures `squelette-*` sont nouvelles.
- Journal du banc : aucune erreur Vulkan. Le seul `[ERREUR]` (`cars/golf.wfm` introuvable) est la panne volontaire de la capture `chargement-erreur`, comme sur main.

### Choix du développeur, jugés

- **Vêtements non colorés** (`_CLOTH`, `pedMaterial`) : accepté. L'apparence est explicitement hors périmètre (WFS-19). Personnage torse nu en short sombre :
  rien de cassé, la peau et le short sont propres. À reprendre dans WFS-19.
- **65 os ramenés à une palette de 62 os à poids** : accepté. Aucune déformation visible en `Idle_Loop` ni en `Walk_Loop` (mains, pieds, tête corrects).

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| — | — | — | Aucune anomalie. |
