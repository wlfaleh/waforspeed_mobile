## Agent : Claude, agent dev3d (01UY1Vv1zn)

**Ticket** : WFS-597 — règles : `regles-de-gestion/WFS-597.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : mettre en fond de l'écran de chargement son image 2 (la SF-X du menu sur un boulevard sous la pluie, maquette acceptée le 2026-11-22 12:27 UTC), telle quelle, sans rien changer d'autre à l'écran.
- **Ce qui change** :
  - `assets/ui/chargement/fond.jpg` (nouveau) : `regles-de-gestion/maquettes/WFS-597/utilisateur-2-boulevard-pluie.png` (1870 × 841, SHA-256 `bcc7aa72…`) mise en 2400 × 1080 (Lanczos, recadrage centré de moins d'1 pixel en hauteur, aucune déformation) et compressée en JPEG qualité 88 : **563 242 octets** (≤ 600 Ko), SHA-256 `26c68061…`. Aucune retouche.
  - `src/ui/ecrans/chargement.cpp` (fonctions `lancerFond`, `terminerFond`, `libererFond`, `dessinerChargement`, `Chargement::debut`, `FonduChargement`) : à `debut()`, après la 1re image, le fichier est lu puis **décodé sur un autre fil pendant la construction de la police** ; envoyé à la carte graphique juste après ; dessiné plein écran **en couverture** (`uvFond` : l'écran est rempli, l'image centrée et coupée sur un seul axe, jamais déformée) à la place du fond uni et des deux reflets d'asphalte. Logo, sur-titre, panneau de verre, barre, étapes, textes, bouton « Réessayer », version et fondu de 300 ms **inchangés** (même code, mêmes places). L'image est rendue (texture détruite) à la fin du fondu ou à l'arrêt du moteur.
  - Réutilise l'emplacement « photos » de l'interface de WFS-562 (`Renderer::uiSetPhotos` / `uiPhoto`, `ui.frag`) : **aucun changement du rendu ni des shaders**. Si les photos des modes sont chargées (2e partie), elles rendent l'emplacement (`modes_photos::liberer`) et sont rechargées à la prochaine ouverture de leur écran (comportement déjà prévu par WFS-562).
  - `src/ui/ecrans/chargement.h` : `FOND_LARGEUR`, `FOND_HAUTEUR`, `FICHIER_FOND_CHEMIN`, `fondPret()`, `uvFond()` (en ligne, testable sans le moteur).
  - `tests/test_fond_chargement.cpp` + 1 ligne dans `CMakeLists.txt` : cadrage sans déformation en 20:9, 16:9, 4:3, très large et taille nulle ; fichier livré en JPEG 2400 × 1080, ≤ 600 Ko.
  - `tools/qa/verifier_licences.py` : **accroche de 3 lignes** (motif `ui/chargement/fond\.jpg`, créé / fourni par le projet, comme `ui/icone-pistolet.png` de WFS-564) pour que le contrôle des licences ne devienne pas rouge à cause de ce fichier. Annoncée au journal.
- **Comment c'est vérifié** :
  - construction complète (1 204 cibles) sans erreur ni avertissement sur les fichiers touchés ; **158 tests CPU** (branche synchronisée avec `main` 699b39e9) (`bash tools/qa/tests_cpu.sh build`), `test_fond_chargement` compris ; `verifier_licences.py` : le nouveau fichier n'est pas signalé (seuls restent les 3 `ui/modes/*.jpg` déjà rouges sur `main`, suivis par WFS-589) ;
  - banc `chargement` en 2400 × 1080 avant / après (8 paires : 0 %, 10 %, 40 %, 70 %, 85 %, 100 %, fondu sur la scène, écran d'erreur), plus 0 %, 85 % et erreur **en 16:9 (1920 × 1080) et en 4:3 (1600 × 1200)** ;
  - RG-06, journal du banc (lavapipe, PC du conteneur) : « fond 2400 × 1080 prêt (décodage 0,079 s à côté de la police, **attente 0,000 s**, envoi 0,085 s, 9.9 Mo) » ; au 2e chargement « décodage 0,040 s, attente 0,000 s, envoi 0,024 s ». La 1re image est toujours dessinée **avant** la lecture du fichier (aussi au rechargement d'une carte) : elle n'est pas retardée ;
  - parcours complet (banc `parcours`, vue normale et vue Cockpit) : accueil → garage → terrain → course → pause → reprise → fin sans écart ; **« fin → Menu » échoue, à l'identique sur `main` 699b39e9** (même banc lancé sur `main` : même erreur, même étape, dans les deux vues) : rouge déjà connu de la CI de `main`, suivi par WFS-589 (`@cicd`), sans lien avec ce ticket (aucun fichier du parcours touché).
- **Limites et risques connus** :
  - « FOR » du logo garde son remplissage couleur du fond d'avant (`#0A0B10`, code de `main` inchangé, RG-01) : sur l'image il se voit en lettres sombres au contour blanc ; en 20:9 l'image est sombre derrière, presque comme sur la maquette ; en 4:3 la lettre passe devant des façades plus claires. Un « FOR » vraiment creux serait un changement du logo, hors de ce ticket (à proposer au PO si l'utilisateur le veut).
  - Mémoire de la carte graphique : +9,9 Mo pendant le chargement et son fondu seulement (vu sur l'écran : « carte graphique 202 → 215 Mo »).
  - Au rechargement d'une carte (2e partie), la 1re image montre le fond uni d'avant une fraction de seconde, puis l'image (RG-06 : « affichée dès qu'elle est prête par-dessus le fond actuel »).
  - Non vérifié sur un vrai téléphone (pas de téléphone ici) : l'utilisateur valide sur l'APK de `main` (statut 🟡 après la fusion).

## Captures pour le lead dev (8 après)

Dossier : [`qa/pr/WFS-597`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-597/qa/pr/WFS-597) — produit par `tools/qa/captures_pr.sh`.

Captures « avant » : `origin/main` = `699b39e965f6c1fd3c8e0b1b5ac34a7f3d313b81` ; cache : 0 banc(s) relu(s), 1 banc(s) rejoué(s) ; main compilé : oui.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `chargement-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-597/qa/pr/WFS-597/avant-chargement-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-597/qa/pr/WFS-597/apres-chargement-0.jpg?raw=true) |
| `chargement-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-597/qa/pr/WFS-597/avant-chargement-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-597/qa/pr/WFS-597/apres-chargement-1.jpg?raw=true) |
| `chargement-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-597/qa/pr/WFS-597/avant-chargement-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-597/qa/pr/WFS-597/apres-chargement-2.jpg?raw=true) |
| `chargement-erreur` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-597/qa/pr/WFS-597/avant-chargement-erreur.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-597/qa/pr/WFS-597/apres-chargement-erreur.jpg?raw=true) |
| `chargement-fondu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-597/qa/pr/WFS-597/avant-chargement-fondu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-597/qa/pr/WFS-597/apres-chargement-fondu.jpg?raw=true) |
| `chargement-route` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-597/qa/pr/WFS-597/avant-chargement-route.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-597/qa/pr/WFS-597/apres-chargement-route.jpg?raw=true) |
| `chargement-ville` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-597/qa/pr/WFS-597/avant-chargement-ville.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-597/qa/pr/WFS-597/apres-chargement-ville.jpg?raw=true) |
| `chargement-voitures` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-597/qa/pr/WFS-597/avant-chargement-voitures.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-597/qa/pr/WFS-597/apres-chargement-voitures.jpg?raw=true) |

### Après seulement : 16:9 et 4:3 (RG-02)

| Capture | 16:9 (1920 × 1080) | 4:3 (1600 × 1200) |
|---|---|---|
| `chargement-0` | ![16:9](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-597/qa/pr/WFS-597/apres-chargement-0-1920x1080.jpg?raw=true) | ![4:3](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-597/qa/pr/WFS-597/apres-chargement-0-1600x1200.jpg?raw=true) |
| `chargement-ville` | ![16:9](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-597/qa/pr/WFS-597/apres-chargement-ville-1920x1080.jpg?raw=true) | ![4:3](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-597/qa/pr/WFS-597/apres-chargement-ville-1600x1200.jpg?raw=true) |
| `chargement-erreur` | ![16:9](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-597/qa/pr/WFS-597/apres-chargement-erreur-1920x1080.jpg?raw=true) | ![4:3](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-597/qa/pr/WFS-597/apres-chargement-erreur-1600x1200.jpg?raw=true) |
