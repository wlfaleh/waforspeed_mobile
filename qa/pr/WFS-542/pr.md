## Agent : Claude, agent dev3d (01Us6uCtQB) (session_01Us6uCtQBbGQhJnxpBzvFjn)

**Ticket** : WFS-542 — règles : `regles-de-gestion/WFS-542.md` (ANO-273, P1)

## Description

- **Demande de l'utilisateur** : dans la Poursuite, la « grande voiture irréaliste » en travers de la route (Range Rover Sport agrandi 1,45 fois) devient un vrai camion, à ses vraies mesures, qui ne reste pas toute la course sous les yeux du joueur.
- **Ce qui change** :
  - **Modèle (RG-01)** : vrai camion porteur à caisse, « '90 Light Commercial Truck » (aussi publié « LCT 3000 '95 ») de Daniel Zhabotinsky, **CC BY 4.0**, marque inventée par l'auteur. Trouvé dans le dépôt public GitHub `MiguelJordans/LAPR5` (licence lue dans son `license.txt` ; SHA-256 dans `assets/licences/camion-poursuite.txt` et au journal). Converti par les outils du dépôt (`tools/convertir_camion.sh` : `convert_car.mjs` puis `alleger_voiture.mjs`) dans `assets/world/camion/porteur.wfm` (1,8 Mo, 19 000 triangles) et `porteur.lod` (3 niveaux allégés).
  - **Échelle 1, sans marque** : mis à l'échelle sur son pneu réel 215/75 R17.5 (767 mm, porteur de 7,5 t) : **6,19 m de long, 2,93 m de haut, 2,22 m de large** (2,69 m aux rétroviseurs), empattement 3,42 m (`references/reel/camion_poursuite.json`). Badges « LCT » retirés, plaques américaines remplacées par une plaque française neutre de 520 × 110 mm (`tools/plaques/camion.png`). `CAMION_ECHELLE` et `camionCar()` sont supprimés : plus aucune voiture agrandie.
  - **Boîte de choc (RG-02)** : `camionTravers()` prend les vraies mesures (6,19 × 2,22 m). Le bout côté fermé est à 1,0 m du bord, ce qui laisse moins que la plus étroite des voitures (1,63 m) : personne ne passe. Une voiture au centre de la file du milieu le heurte. La file libre garde **7,81 m de passage**, au moins la plus large voiture (2,05 m) + 1 m.
  - **Immobile (RG-03)** : le camion n'a ni vitesse ni rotation ; une voiture ne le déplace jamais (code de choc inchangé : arrêt net de face, glissement de côté par le bout).
  - **Retrait (RG-04)** : `camionsRetires()` retire le camion 50 m derrière le joueur, ou 60 s après son apparition. L'âge est compté dès que le camion est à portée de vue (300 m) : posé 450 m avant, il ne peut pas disparaître avant d'avoir été vu.
  - **Budget (RG-05)** : niveaux allégés choisis par la taille à l'écran (système existant `vehicule_lod`, jamais de cube). Le camion coûte moins que le Range Rover qu'il remplace : **50 700 triangles au lieu de 327 000** au niveau complet, **21 pièces au lieu de 51**, fichier de 1,8 Mo au lieu de 6,8 Mo.
  - **Réutilisé (RG-06)** : `Surprise::Camion`, `camionsTouches`, `camionsTrafic`, le bandeau, `loadCarModel` (roues séparées, LOD, peinture), `plaques_neutres.mjs` (une ligne ajoutée).
  - **Crédits en anglais** : `assets/textes/credits-en.txt` (section et entrée du camion, retour du lead dev).
  - **Crédits** : `tools/qa/verifier_licences.py` (section « Camion de la Poursuite » et couverture de `world/camion/porteur.*`) ; `CREDITS.txt` et `store/credits.md` régénérés.
- **Comment c'est vérifié** :
  - nouveau test `test_camion_542` : mesures du `.wfm` livré ±1 %, poids, `.lod` présent, boîte de choc à l'échelle 1, file libre, côté fermé infranchissable, retrait ;
  - `test_course5` (poursuite) et `test_voitures_retirees` mis à jour ;
  - **140 tests CPU verts** sur une construction propre (commit dc63d4f2) (`bash tools/qa/tests_cpu.sh build-propre`), dont `test_credits` et `test_camion_542` : voir « Tests CPU » ;
  - `verifier_licences.py` : le camion est couvert. 0 manque après la synchronisation avec main (WFS-543 fusionné) ;
  - banc `camion-542` : les 13 captures avant / après ci-dessous, regardées une à une. Retrait (RG-04) : joueur tenu à 26-32 m du camion, camion encore là à 55 s (`retrait-00`) et retiré à 61 s (`retrait-01`) ; sur main, le Range Rover est toujours là. Le retrait à 50 m derrière est vérifié par `test_camion_542` (à 40 m derrière, le camion n'était déjà plus dessiné).
- **Limites et risques connus** :
  - Le camion est un modèle à marque inventée : il n'existe pas de fiche constructeur. L'échelle repose sur le pneu normalisé (ETRTO). Ses proportions, cabine large de 2,22 m et caisse de 4,14 × 2,18 m, sont celles d'un porteur de 7,5 t.
  - Les files du jeu font 5 m : un vrai camion de 6,19 m ne peut pas couvrir 2 files entières. Il barre la file fermée et le centre de la file du milieu ; la voie libre fait 7,81 m. Le Range Rover agrandi (7,2 m) laissait déjà 1,4 m côté fermé.
  - Le coût en millisecondes n'est pas mesuré sur un téléphone, seulement par le nombre de triangles et de pièces, qui baisse.
  - Statut après fusion : **🟡 À valider (utilisateur)**.

## Tests CPU

Commit **dc63d4f2** (branche synchronisée avec `main` après WFS-543, 544, 545), construction propre, `bash tools/qa/tests_cpu.sh <build-propre>` : **140 test(s) réussi(s) en 421 s**, aucun échec (dont `test_credits` et `test_camion_542`). Journal complet : [`qa/pr/WFS-542/tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/tests-cpu.log).

## Captures pour le lead dev (13 après)

Dossier : [`qa/pr/WFS-542`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542) : banc `WFS_BANC=camion-542` (Corniche, programme fixe). Scènes : nuit et jour (vue Poursuite, 30 / 20 / 12 m, police dans l'image de nuit), Cockpit à 10 m du flanc, vitrine à côté de la Golf (4,28 m), retrait (joueur devant le camion, 55 s puis 61 s après son apparition).

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `camion542-cockpit-00` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/avant-camion542-cockpit-00.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/apres-camion542-cockpit-00.jpg?raw=true) |
| `camion542-cockpit-01` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/avant-camion542-cockpit-01.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/apres-camion542-cockpit-01.jpg?raw=true) |
| `camion542-jour-00` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/avant-camion542-jour-00.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/apres-camion542-jour-00.jpg?raw=true) |
| `camion542-jour-01` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/avant-camion542-jour-01.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/apres-camion542-jour-01.jpg?raw=true) |
| `camion542-jour-02` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/avant-camion542-jour-02.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/apres-camion542-jour-02.jpg?raw=true) |
| `camion542-nuit-00` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/avant-camion542-nuit-00.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/apres-camion542-nuit-00.jpg?raw=true) |
| `camion542-nuit-01` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/avant-camion542-nuit-01.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/apres-camion542-nuit-01.jpg?raw=true) |
| `camion542-nuit-02` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/avant-camion542-nuit-02.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/apres-camion542-nuit-02.jpg?raw=true) |
| `camion542-retrait-00` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/avant-camion542-retrait-00.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/apres-camion542-retrait-00.jpg?raw=true) |
| `camion542-retrait-01` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/avant-camion542-retrait-01.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/apres-camion542-retrait-01.jpg?raw=true) |
| `camion542-vitrine-00` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/avant-camion542-vitrine-00.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/apres-camion542-vitrine-00.jpg?raw=true) |
| `camion542-vitrine-01` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/avant-camion542-vitrine-01.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/apres-camion542-vitrine-01.jpg?raw=true) |
| `camion542-vitrine-02` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/avant-camion542-vitrine-02.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-542/qa/pr/WFS-542/apres-camion542-vitrine-02.jpg?raw=true) |
