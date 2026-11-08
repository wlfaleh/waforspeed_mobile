Agent : Claude, agent 1 (01UY1Vv2)

**Ticket** : WFS-299 — règles : `regles-de-gestion/WFS-299.md` (périmètre fixé par le PO : RG-01 seul ; RG-02 déjà fait par WFS-263 ; RG-03 sorti du ticket, mesure sur le téléphone ensuite)

## Description

- **Demande de l'utilisateur** : alléger la Ville de nuit, la scène la plus lourde (diagnostic de WFS-297) : piétons simplifiés au loin, même nombre de personnes, aucun saut visible.
- **Ce qui change** :
  - `src/render/squelette.cpp` : au chargement de chaque personnage, un niveau de détail 3 « très simple » est fabriqué à partir du niveau 2 de chaque plage (`corps-2` → `corps-3`, coiffures, `Hair_Long-3`). Il est obtenu par regroupement des sommets en cubes de 7 cm (Rossignac et Borrel 1993).
    - Chaque sommet est remplacé par le premier sommet de son cube : il garde ses os, ses poids et son UV, donc l'animation reste juste.
    - Les triangles aplatis disparaissent.
    - Aucun sommet nouveau : seuls des indices s'ajoutent, et le fichier `.wfp` ne change pas.
    - Résultat : environ 800 triangles avec la coiffure, contre 2 300 au niveau 2.
  - `src/world/pietons.cpp` : seuils de Basse 8 / 20 / 30 m (avant : 8 / 30 m, sans niveau 3).
    - Moyenne et Haute sont inchangées : pas de 3e seuil.
    - Repli sur le niveau 2 si un modèle n'a pas de niveau 3.
  - `src/world/foule_apparition.h` : `niveauVoulu` / `avancerNiveau` prennent 3 seuils. Le fondu croisé et l'hystérésis existants servent aussi au niveau 3.
  - **Banc** :
    - `WFS_PIETONS_AVANT=1` rétablit les anciens seuils, pour comparer avant et après dans la même vue ;
    - `WFS_APPARITIONS_BASSE=1` lance le banc des apparitions en Basse ;
    - le banc des parties passe en vue Proche, avec le HUD affiché (remarques du lead dev sur WFS-297), et prend une suite d'images à 0,5 s d'écart.
- **Comment c'est vérifié** :
  - 69 tests CPU verts, dont `test_apparition_pietons`, qui passe maintenant 3 seuils et vérifie le niveau 3 ; parcours complet 0 écart ;
  - **apparitions** (WFS-257, Basse, 4 cartes, jour et nuit, 12 s chacune) : 0 apparition, 0 disparition, 0 saut, 0 détail brutal ;
  - **mesure** (banc PC lavapipe, Ville de nuit, Basse, 2340×1080, vue Proche) :

    | | Piétons au départ | Scène au départ | GPU au départ |
    |---|---|---|---|
    | Avant | 129 appels, 260 k triangles | 744 k triangles | 658 ms |
    | Après | 129 appels, **174 k triangles (−33 %)** | 658 k triangles (−12 %) | 659 ms |

  - Le banc tourne en rendu logiciel (lavapipe), limité par le nombre de pixels : le temps n'y bouge pas. Le gain se verra sur le GPU du téléphone, où les personnages animés coûtent cher en sommets.
- **Limites et risques connus** :
  - RG-03 (≥ 25 img/s) est sorti du ticket par le PO : après la fusion, la ligne « Images (20 s) » du téléphone dira quel poste coûte le plus.
  - Sur les captures des Réglages, la ligne « Images (20 s) » affiche les valeurs d'essai du banc.
  - À moins de 30 m, rien ne change.

## Captures (2340×1080, Basse, vue Proche ; avant = `WFS_PIETONS_AVANT=1`, même binaire)

| Capture | Avant | Après |
|---|---|---|
| Ville de nuit, départ | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-299/qa/pr/WFS-299/avant-parties-1-ville-nuit.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-299/qa/pr/WFS-299/apres-parties-1-ville-nuit.jpg?raw=true) |
| Ville de nuit, 30 s | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-299/qa/pr/WFS-299/avant-parties-1-ville-nuit-30s.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-299/qa/pr/WFS-299/apres-parties-1-ville-nuit-30s.jpg?raw=true) |
| Ville de jour, trottoir animé | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-299/qa/pr/WFS-299/avant-parties-2-ville-jour.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-299/qa/pr/WFS-299/apres-parties-2-ville-jour.jpg?raw=true) |
| Réglages (Rendu, Images, mémoire) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-299/qa/pr/WFS-299/avant-parties-1-reglages.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-299/qa/pr/WFS-299/apres-parties-1-reglages.jpg?raw=true) |

### Agrandissement des piétons de la Ville de nuit (avant en haut, après en bas)

![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-299/qa/pr/WFS-299/zoom-ville-nuit-avant-dessus-apres-dessous.jpg?raw=true)

### Suite à 0,5 s d'écart (on se rapproche des piétons : passages 3 → 2 → 1 → 0), après

| | | |
|---|---|---|
| ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-299/qa/pr/WFS-299/apres-parties-1-suite-0.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-299/qa/pr/WFS-299/apres-parties-1-suite-1.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-299/qa/pr/WFS-299/apres-parties-1-suite-2.jpg?raw=true) |
| ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-299/qa/pr/WFS-299/apres-parties-1-suite-3.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-299/qa/pr/WFS-299/apres-parties-1-suite-4.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-299/qa/pr/WFS-299/apres-parties-1-suite-5.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv2MVfXZF3cPf1r8qRq
