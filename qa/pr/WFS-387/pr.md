Agent : Claude, agent dev3d (01UY1Vv43z)

**Ticket** : WFS-387 — règles : `regles-de-gestion/WFS-387.md` (périmètre réduit par le PO le 2026-11-13 00:21 UTC, RG-09 : réserves de WFS-372, muret des épingles, sol jusqu'à l'horizon, inventaire). Épic **E15** : avis de l'agent 3D demandé avant le lead dev.

## Description

- **Demande de l'utilisateur** (en une phrase) : refaire le décor de la Campagne pour qu'il soit réaliste à 99 %. Pour ce ticket découpé : corriger les réserves de WFS-372 sur les bottes, la ferme, la clôture, le poteau électrique et le hangar ; faire arrêter la voiture au muret visible des épingles ; ne plus laisser aucun vide au-delà de la bande de sol ; faire l'inventaire du reste.
- **Ce qui change** :
  - `src/world/decor_campagne_modeles.{h,cpp}` :
    - **grain porté par les sommets** : la part P_UNI du décor a le drapeau MAT_VTINT, donc la longueur de la normale d'un sommet est sa luminosité. Les nouvelles fonctions `teinte` (bruit lissé, moyenne 1, bornes 0,75 à 1,25) et `relief` (normales penchées) s'en servent, sans texture ni nouveau matériau ;
    - **bottes (réserve 1)** :
      - flanc plus fin : 24 côtés et 11 anneaux, au lieu de 20 côtés et 3 anneaux ;
      - fibres de la paille dans le sens du roulement (± 24 %) et couches de paille enroulée, une teinte par tour ;
      - touffes (normales penchées) ;
      - **filet** blanc crème sur les épaules, nouvelle case `C_FILET` voisine de `C_FOIN` dans la palette, pour un fondu sans couleur parasite ;
      - bouts à brins coupés ;
    - **ferme (réserve 2)** :
      - enduit découpé en carreaux de 0,25 m : taches de ± 10 % et grain de ± 7 % ;
      - pied de mur plus sombre de 12 % sur 1 m (rejaillissement de la pluie) ;
      - tuiles de cuissons variées ;
    - **clôture (réserve 3)** : poteaux et lisses dans la même case de bois, avec des veines dans le fil du bois ;
    - **poteau de ligne (réserve 4)** : gerces verticales et pied plus sombre ;
    - **hangar (réserve 5)** :
      - creux des ondes plus sombres que les crêtes (± 12 %) ;
      - recouvrement des plaques marqué tous les 1,06 m ;
      - teinte propre à chaque plaque (± 10 %), plaques de 2,5 m dans le sens de la pente ;
      - version « loin » faite de bandes de plaques, au lieu d'un seul quadrilatère ;
    - **`fenetre()` (remarque 9)** : plus de comparaison de flottants avec `==` (table des traverses) ;
    - **`poteauSuivant` (remarque 7)** : renvoie le prochain poteau réellement posé d'une file ;
    - **`muretTroncon` et `chocMuret` (RG-11)** : parapet de moellons (une teinte par pierre, joints creux) et chaperon arrondi, aux mêmes mesures que la boîte de choc ;
  - `src/world/campagne.cpp` :
    - **palette** :
      - enduit ocre clair chaud 0xdcbd8d, au lieu du 0xcbbb9b qui virait au gris-bleu à l'ombre ;
      - bois de la clôture gris-brun 0x967f65 ;
      - poteau électrique gris-brun 0x7e7264, au lieu du 0x56463a presque noir ;
      - filet 0xd6cfb6 ;
    - **liaisons des poteaux (remarque 7)** :
      - `place()` note les poteaux réellement posés ;
      - une lisse ou un câble n'est tracé que jusqu'au prochain poteau posé **et dessiné** (mêmes filtres que `buildChunk`) ;
      - si ce poteau est dans le morceau voisin, il est aussi posé dans le morceau courant, pour que le bout ne flotte jamais quand le voisin passe en version loin ;
    - **épingles (RG-11)** : `T_MURET` est dessiné par `muretTroncon`, à la place de la boîte grise ; son choc vient de `chocMuret` ;
    - **sol jusqu'à l'horizon (RG-10)** :
      - le plan uni (aplat vert, gris dans le brouillard) est remplacé par une grille de 48 m qui prolonge le bord des bandes de sol, puis monte en collines douces loin des routes ;
      - sous les bandes, la grille passe 3 m sous la plus basse des routes voisines ;
      - teinte de la prairie lointaine (luminosité par sommet, parcelles de ± 8 %), en 20 tuiles ; une tuile à plus de 2,6 km de la caméra n'est pas dessinée ; les cases entièrement cachées sous les bandes de sol ne sont pas construites ; les tuiles sont dessinées après les autres opaques (pixels cachés rejetés avant le shader) ; le sol naturel de WFS-383 reste sur les bandes près de la route (il coûtait 9 ms par image en Basse sur la grille lointaine) ;
      - elle est préparée au chargement, plus à la première image ;
  - `src/world/decor_carte.cpp` (**fichier commun, accroche de 6 lignes annoncée au journal**) : la cause du vide gris des vues de dessus `course5-*-dessus` était que le décor n'était pas dessiné dans la scène dédiée de ces vues. `vueDessusCourse()` laisse dessiner le décor quand l'écran est la course et que la caméra est à plus de 30 m au-dessus du point visé ; rien ne change pour les autres scènes dédiées (garage, menus) ;
  - `tests/test_muret_campagne.cpp` (nouveau) et **une ligne** dans `CMakeLists.txt` (fichier commun, annoncée au journal) : la vraie voiture lancée sur le parapet extérieur de chaque épingle (30, 80, 150 km/h, conduite réelle et conduite d'avant) s'arrête à la face visible (8,00 m) à ± 0,2 m ;
  - `src/world/decor_campagne_banc.cpp` :
    - commentaire en double retiré (remarque 8) ;
    - vues de nuit pour chaque objet ;
    - nouvelles vues `bottes-pres`, `ferme-soleil` (façade au soleil) et `hangar-moyen` (couverture à 80 m) ;
  - `tests/test_decor_campagne.cpp` :
    - luminosité des sommets bornée, de moyenne 1 ± 0,06 ;
    - grain de la botte, de l'enduit et de la tôle ;
    - liaisons des poteaux, avec un cas qui montre l'ancienne faute de « a + 3 » ;
    - parapet et boîte de choc à la même place ;
  - `qa/pr/WFS-387/inventaire.md` : inventaire RG-02, chaque partie renvoyée à son ticket (WFS-518 à 521).
- **Reprise après l'avis de l'agent 3D (À REPRENDRE, commit 2f642514)** :
  1. RG-11 : nouvelles vues `campagne-objets-muret` et `-muret-voiture` (jour et nuit) : parapet extérieur d'une épingle de près (moellons en pierre calcaire `C_PIERRE0`, joints creux, chaperon) et la voiture lancée vers lui puis freinée, arrêtée contre lui (choc WFS-256) ;
  2. RG-10 : parois grises supprimées : la hauteur du sol lointain n'est plus celle de la route la plus proche (elle sautait entre deux routes de hauteurs différentes) ; les points loin des bandes sont relâchés (Gauss-Seidel sur-relaxé) entre les points fixés contre le bord des bandes : pentes douces, collines ajoutées ensuite ;
  3. filet des bottes crème chaud 0xd2bf8f, bouts 0xcaae6c ;
  4. bois du poteau 0x8d7d68 et de la clôture 0x9c8466, vues `poteau-soleil` et `cloture-soleil` ajoutées ;
  5. sol lointain dans une case à lui, kaki d'herbe sèche 0x8f8c5e (comme les bandes voisines), au lieu du vert vif des collines ;
  6. `inventaire.md` : teintes alignées sur le code.
- **Comment c'est vérifié** :
  - **tests CPU** : `bash tools/qa/tests_cpu.sh build` → **120 test(s) réussi(s)** (branche synchronisée avec `main`, avant la reprise ; `test_decor_campagne` et `test_muret_campagne` relancés après : OK), dont `test_decor_campagne` et `test_muret_campagne` (coin de la voiture arrêté entre 7,99 et 8,07 m pour une face visible à 8,00 m) ;
  - **captures** : bancs `campagne-objets`, `campagne` (jour et nuit) et `course5` (vues de dessus), lavapipe comme la CI, avant `main` et après branche (tableau ci-dessous) ;
  - **taille du décor (≤ 700 Mo)** : `du -cb assets/world/animaux/{vache,cheval,mouton}.wfa assets/world/feuillage.png` = **2 906 962 octets (2,9 Mo)** ; le reste du décor est construit par le code au chargement (aucun fichier ajouté par ce ticket) ;
  - **performance, banc `perf` en Basse** (`WFS_BANC=perf WFS_PERF_CARTE=campagne`, lavapipe, `main` et branche lancés l'un après l'autre, en alternance) :

    | Passage | main | branche | écart |
    |---|---|---|---|
    | 1 (sol lointain en sol naturel) | 240,2 ms | 256,9 ms | +7,0 % |
    | 2 (sol lointain en sol naturel) | 254,6 ms | 272,0 ms | +6,8 % |
    | 4 (version de la PR) | 230,9 ms | 243,3 ms | **+5,4 %** |

    Le bruit entre deux passages de `main` va jusqu'à 6 %. Cause : ≈ +4 % pour la grille du sol jusqu'à l'horizon (RG-10 : elle remplit des pixels où l'on voyait le ciel), le reste pour les objets refaits ; triangles 287 k → 331 k, appels de dessin 187 → 192. **Arbitrage du PO (2026-11-13, commit 9b923a4e sur main)** : écart accepté jusqu'à +6 % sans reculer sur RG-10 ni RG-12 ; le gain va dans **WFS-528** (LOD des bottes et du crépi, sol lointain plus grossier) ; juge final : le téléphone de l'utilisateur (ticket 🟡). Tout reste préparé au chargement, rien n'est créé en course.
- **Limites et risques connus** :
  - le grain est porté par les sommets : il se voit de près et à moyenne distance, mais ce n'est pas une texture PBR. Les vraies textures (paille, enduit, bois, tôle) attendent WFS-521, c'est-à-dire l'ouverture d'un site CC0 par l'utilisateur ;
  - la façade de la ferme vue depuis la route est à contre-jour dans la vue du banc : elle reste éclairée par le ciel bleu. La vue `ferme-soleil` montre l'enduit au soleil ;
  - performance : +5,4 % de temps par image sur le banc PC en Basse (accepté par le PO, gain dans WFS-528) ;
  - la grille du sol lointain suit le bord des bandes : vue de très près, au bord d'une bande, la jonction peut faire une petite marche de quelques dizaines de centimètres (cachée par l'herbe et le brouillard sur les captures) ;
  - le filet blanc des bottes prend encore un peu du bleu du ciel sur l'arête de l'épaule (lumière du ciel rasante sur un plastique clair) ; sa teinte propre est crème ;
  - les vues `bottes-pres`, `ferme-soleil`, `hangar-moyen`, `poteau-soleil`, `cloture-soleil`, `muret`, `muret-voiture` et plusieurs vues de nuit sont nouvelles : elles n'ont pas d'« avant ».

## Captures pour le lead dev (48 après)

Dossier : [`qa/pr/WFS-387`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387) — bancs `campagne-objets`, `campagne` (1280x720) et `course5` (vues de dessus), lavapipe comme la CI. Avant = `origin/main`, après = branche.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `campagne-objets-bottes` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-campagne-objets-bottes.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-campagne-objets-bottes.jpg?raw=true) |
| `campagne-objets-bottes-nuit` | — (vue nouvelle, absente du banc de main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-campagne-objets-bottes-nuit.jpg?raw=true) |
| `campagne-objets-bottes-pres` | — (vue nouvelle, absente du banc de main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-campagne-objets-bottes-pres.jpg?raw=true) |
| `campagne-objets-cloture` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-campagne-objets-cloture.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-campagne-objets-cloture.jpg?raw=true) |
| `campagne-objets-cloture-nuit` | — (vue nouvelle, absente du banc de main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-campagne-objets-cloture-nuit.jpg?raw=true) |
| `campagne-objets-cloture-soleil` | — (vue nouvelle, absente du banc de main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-campagne-objets-cloture-soleil.jpg?raw=true) |
| `campagne-objets-ferme` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-campagne-objets-ferme.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-campagne-objets-ferme.jpg?raw=true) |
| `campagne-objets-ferme-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-campagne-objets-ferme-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-campagne-objets-ferme-nuit.jpg?raw=true) |
| `campagne-objets-ferme-soleil` | — (vue nouvelle, absente du banc de main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-campagne-objets-ferme-soleil.jpg?raw=true) |
| `campagne-objets-hangar` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-campagne-objets-hangar.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-campagne-objets-hangar.jpg?raw=true) |
| `campagne-objets-hangar-moyen` | — (vue nouvelle, absente du banc de main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-campagne-objets-hangar-moyen.jpg?raw=true) |
| `campagne-objets-hangar-nuit` | — (vue nouvelle, absente du banc de main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-campagne-objets-hangar-nuit.jpg?raw=true) |
| `campagne-objets-muret` | — (vue nouvelle, absente du banc de main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-campagne-objets-muret.jpg?raw=true) |
| `campagne-objets-muret-nuit` | — (vue nouvelle, absente du banc de main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-campagne-objets-muret-nuit.jpg?raw=true) |
| `campagne-objets-muret-voiture` | — (vue nouvelle, absente du banc de main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-campagne-objets-muret-voiture.jpg?raw=true) |
| `campagne-objets-muret-voiture-nuit` | — (vue nouvelle, absente du banc de main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-campagne-objets-muret-voiture-nuit.jpg?raw=true) |
| `campagne-objets-poteau` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-campagne-objets-poteau.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-campagne-objets-poteau.jpg?raw=true) |
| `campagne-objets-poteau-nuit` | — (vue nouvelle, absente du banc de main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-campagne-objets-poteau-nuit.jpg?raw=true) |
| `campagne-objets-poteau-soleil` | — (vue nouvelle, absente du banc de main) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-campagne-objets-poteau-soleil.jpg?raw=true) |
| `campagne-objets-vigne` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-campagne-objets-vigne.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-campagne-objets-vigne.jpg?raw=true) |
| `campagne-objets-vigne-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-campagne-objets-vigne-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-campagne-objets-vigne-nuit.jpg?raw=true) |
| `carte-campagne-autoroute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-carte-campagne-autoroute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-carte-campagne-autoroute.jpg?raw=true) |
| `carte-campagne-autoroute-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-carte-campagne-autoroute-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-carte-campagne-autoroute-nuit.jpg?raw=true) |
| `carte-campagne-cascade` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-carte-campagne-cascade.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-carte-campagne-cascade.jpg?raw=true) |
| `carte-campagne-cascade-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-carte-campagne-cascade-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-carte-campagne-cascade-nuit.jpg?raw=true) |
| `carte-campagne-chevaux` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-carte-campagne-chevaux.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-carte-campagne-chevaux.jpg?raw=true) |
| `carte-campagne-chevaux-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-carte-campagne-chevaux-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-carte-campagne-chevaux-nuit.jpg?raw=true) |
| `carte-campagne-col` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-carte-campagne-col.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-carte-campagne-col.jpg?raw=true) |
| `carte-campagne-col-haut` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-carte-campagne-col-haut.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-carte-campagne-col-haut.jpg?raw=true) |
| `carte-campagne-col-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-carte-campagne-col-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-carte-campagne-col-nuit.jpg?raw=true) |
| `carte-campagne-epingle` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-carte-campagne-epingle.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-carte-campagne-epingle.jpg?raw=true) |
| `carte-campagne-epingle-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-carte-campagne-epingle-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-carte-campagne-epingle-nuit.jpg?raw=true) |
| `carte-campagne-haut-du-col` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-carte-campagne-haut-du-col.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-carte-campagne-haut-du-col.jpg?raw=true) |
| `carte-campagne-moutons` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-carte-campagne-moutons.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-carte-campagne-moutons.jpg?raw=true) |
| `carte-campagne-moutons-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-carte-campagne-moutons-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-carte-campagne-moutons-nuit.jpg?raw=true) |
| `carte-campagne-peage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-carte-campagne-peage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-carte-campagne-peage.jpg?raw=true) |
| `carte-campagne-peage-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-carte-campagne-peage-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-carte-campagne-peage-nuit.jpg?raw=true) |
| `carte-campagne-rondpoint` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-carte-campagne-rondpoint.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-carte-campagne-rondpoint.jpg?raw=true) |
| `carte-campagne-rondpoint-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-carte-campagne-rondpoint-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-carte-campagne-rondpoint-nuit.jpg?raw=true) |
| `carte-campagne-vaches` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-carte-campagne-vaches.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-carte-campagne-vaches.jpg?raw=true) |
| `carte-campagne-vaches-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-carte-campagne-vaches-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-carte-campagne-vaches-nuit.jpg?raw=true) |
| `carte-campagne-vignes` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-carte-campagne-vignes.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-carte-campagne-vignes.jpg?raw=true) |
| `carte-campagne-vignes-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-carte-campagne-vignes-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-carte-campagne-vignes-nuit.jpg?raw=true) |
| `carte-campagne-village` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-carte-campagne-village.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-carte-campagne-village.jpg?raw=true) |
| `carte-campagne-village-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-carte-campagne-village-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-carte-campagne-village-nuit.jpg?raw=true) |
| `course5-depart-campagne-dessus` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-course5-depart-campagne-dessus.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-course5-depart-campagne-dessus.jpg?raw=true) |
| `course5-epingle-apres-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-course5-epingle-apres-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-course5-epingle-apres-1.jpg?raw=true) |
| `course5-epingle-apres-4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/avant-course5-epingle-apres-4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-387/qa/pr/WFS-387/apres-course5-epingle-apres-4.jpg?raw=true) |
