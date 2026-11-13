## Agent : Claude, agent dev3d (01Us6uCutV)

**Ticket** : WFS-492 — règles : `regles-de-gestion/WFS-492.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : des habillages complets (skins) posés sur la carrosserie, d'abord la skin **Rallye** sur les 6 voitures 208, 500, GT86, Golf, Corvette C7 (id `mustang`) et Giulia (épic E18, RG-01 à 07 de `regles-de-gestion/WFS-492.md`).
- **Ce qui change** :
  - **Moteur** `src/render/vehicule_skin.*` : une texture d'habillage par couple voiture × skin (`assets/cars/skins/<voiture>-rallye.etc2.ktx2`), chargée par `Game::setCar` (skin possédée, `perso::aspectCourse` ; bancs : `WFS_SKIN=<id>`), **une seule en mémoire**, libérée au changement de voiture et à `Game::shutdown` ; copie habillée des matériaux **peints** de la voiture du joueur seulement (trafic, adversaires, police, fantôme : sans skin, RG-06). GPU sans ETC2 (PC, lavapipe) : niveaux EAC décodés sur le processeur en RG8, même image.
  - **Shader** `shaders/vehicule.glsl` (`habillage`) : dépliage par projection en mètres du repère de la voiture (flancs gauche / droit, dessus, avant, arrière), mêlé selon la normale lissée ; motifs à taille réelle, continus d'un panneau à l'autre (aucune couture porte / aile, RG-04) ; la finition (WFS-490) et le vernis restent par-dessus (RG-01). Numéro de la skin transporté dans le métal codé (`+ 16 × skin`), sans constante de poussée en plus.
  - **Texture** : EAC RG11 1024 × 1024 (famille ETC2, compressée, 11 mipmaps), deux masques (R = couleur A, G = couleur B par-dessus) ; couleurs de chaque skin dans le shader (`SKIN_COULEURS`) ; **1,33 Mo** par voiture (≤ 1,5 Mo, RG-02).
  - **Outil** `tools/skins/generer.py` (+ `voitures.json`, mesures relevées au banc) : dessin du Rallye en mètres, sur-échantillonné × 4, mipmaps linéaires, **encodeur EAC écrit dans le dépôt** (recherche exhaustive), PSNR 56,6 à 59,1 dB au niveau 0 (≥ 38 dB), KTX2.
  - **Rallye (RG-05, dessin original)** : deux larges bandes blanches bordées d'un filet anthracite du capot au hayon, qui descendent sur les boucliers ; plaque de course « 07 » sur chaque portière avant (police Rajdhani, OFL, déjà créditée) ; bande et filet le long du bas de caisse. Aucun logo, aucun sponsor, aucune livrée d'écurie connue. Vitres, phares, calandre, jantes jamais peints (seules les pièces `MAT_PAINT` la reçoivent).
  - **Accroches annoncées au journal** : `renderer.cpp` (draw : matériau habillé + code, 3 lignes ; include), `renderer.h` (`vuesMateriau`, 2 lignes), `game.cpp` (setCar, shutdown), `scene.frag` (1 ligne), `ktx2.cpp` (format EAC RG11, 2 lignes), `tools/textures/ktx2.py` (format EAC RG11), `tools/qa/verifier_licences.py` (skins = dessins du projet, 2 lignes), `desktop_main.cpp` (banc `skins`), `CMakeLists.txt` (test).
- **Comment c'est vérifié** :
  - `bash tools/qa/tests_cpu.sh build` : **130 tests réussis**, dont le nouveau `test_vehicule_skin` (décodeur EAC : 0 / 2047 exacts, multiplicateur nul, ordre des texels ; codage skin + finition + métal ; les 6 textures : format, 11 niveaux, ≤ 1,5 Mo, bandes / peinture / filet / numéro au bon endroit du dépliage, flancs symétriques).
  - Banc `WFS_BANC=skins` (captures ci-dessous) ; journal : « 1 texture(s) d'habillage en mémoire » en vitrine et en course, « 0 en fin de banc », **0 maillage et 0 matériau créés en course** (RG-03).
  - **Sans skin = aspect d'avant** : planche `peinture2-planche` (17 voitures) de main et de la branche **identiques au pixel près** (0 pixel différent, comparaison des PNG) ; vitrine `skins-golf-sans`.
  - **RG-07, endurance en Basse** (banc `peinture2-mesure`, 208 en course, Ville, même binaire, sans skin / Rallye en alternance) : sans 3,89 et 4,00 img/s, Rallye 3,93 et 3,95 img/s (1 % les plus lents : 2,63 / 2,86 contre 2,96 / 2,97) : **aucun écart mesurable (< 1 img/s)**.
  - **Mémoire GPU** : téléphone (EAC) 1 365 Ko pour la texture d'habillage ; PC (RG8 décodé) 2 731 Ko ; textures du GPU du banc 521,7 Mo sans skin → 524,3 Mo avec (+2,6 Mo, repli RG8).
  - `python3 tools/qa/verifier_licences.py` : plus aucun manque dû aux skins (les 4 manques restants, `cars/remplacants/*`, existent déjà sur main).
- **Limites et risques connus** :
  - **ASTC non produit** : l'encodeur ASTC du dépôt (`tools/textures/encodeurs.sh`) est un binaire téléchargé, que `regles-reseau.md` § 3.5 interdit d'exécuter ; j'ai choisi EAC RG11 (ETC2, lu par les GPU Android Vulkan) avec un encodeur écrit dans le dépôt. Un GPU sans ETC2 prend le repli décodé (2,7 Mo au lieu de 1,4 Mo).
  - Les couleurs A et B de chaque skin sont dans le shader (la texture ne porte que les masques) : WFS-493 ajoute les siennes dans `SKIN_COULEURS` (et un dessin dans `generer.py`).
  - Écran de choix des skins et aperçu au garage : hors ticket (WFS-494) ; ici, la skin vient de la sauvegarde (possédée) ou de `WFS_SKIN` au banc.
  - Rendu vérifié sur lavapipe seulement (pas de vrai GPU de téléphone). Corvette : vitres claires et GT86 : lunette noire déjà présentes sans skin (aspect des modèles, hors ticket).

## Captures pour le lead dev (18 après)

Dossier : [`qa/pr/WFS-492`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-492/qa/pr/WFS-492) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `peinture2-planche` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-492/qa/pr/WFS-492/avant-peinture2-planche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-492/qa/pr/WFS-492/apres-peinture2-planche.jpg?raw=true) |
| `skins-abarth-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-492/qa/pr/WFS-492/apres-skins-abarth-jour.jpg?raw=true) |
| `skins-giulia-course-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-492/qa/pr/WFS-492/apres-skins-giulia-course-jour.jpg?raw=true) |
| `skins-giulia-course-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-492/qa/pr/WFS-492/apres-skins-giulia-course-nuit.jpg?raw=true) |
| `skins-giulia-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-492/qa/pr/WFS-492/apres-skins-giulia-jour.jpg?raw=true) |
| `skins-giulia-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-492/qa/pr/WFS-492/apres-skins-giulia-nuit.jpg?raw=true) |
| `skins-golf-course-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-492/qa/pr/WFS-492/apres-skins-golf-course-jour.jpg?raw=true) |
| `skins-golf-course-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-492/qa/pr/WFS-492/apres-skins-golf-course-nuit.jpg?raw=true) |
| `skins-golf-jonction` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-492/qa/pr/WFS-492/apres-skins-golf-jonction.jpg?raw=true) |
| `skins-golf-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-492/qa/pr/WFS-492/apres-skins-golf-jour.jpg?raw=true) |
| `skins-golf-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-492/qa/pr/WFS-492/apres-skins-golf-nuit.jpg?raw=true) |
| `skins-golf-sans` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-492/qa/pr/WFS-492/apres-skins-golf-sans.jpg?raw=true) |
| `skins-gt86-course-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-492/qa/pr/WFS-492/apres-skins-gt86-course-jour.jpg?raw=true) |
| `skins-gt86-course-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-492/qa/pr/WFS-492/apres-skins-gt86-course-nuit.jpg?raw=true) |
| `skins-gt86-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-492/qa/pr/WFS-492/apres-skins-gt86-jour.jpg?raw=true) |
| `skins-gt86-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-492/qa/pr/WFS-492/apres-skins-gt86-nuit.jpg?raw=true) |
| `skins-mustang-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-492/qa/pr/WFS-492/apres-skins-mustang-jour.jpg?raw=true) |
| `skins-p208-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-492/qa/pr/WFS-492/apres-skins-p208-jour.jpg?raw=true) |
