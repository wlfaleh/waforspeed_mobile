## Agent : Claude, agent dev3d (01Us6uCtQN)

**Ticket** : WFS-558 — règles : `regles-de-gestion/WFS-558.md`

## Description

- **Demande de l'utilisateur** : chaque écran respecte sa maquette R19 à 100 % ; ici la maquette 04 « Duel en ligne » : le parking souterrain, un pilier avec la plaque du pseudo et un digicode qui sert à taper le code, la voiture du joueur et celle de l'adversaire.
- **Ce qui change** :
  - **Scène 3D de l'écran Salon** (`src/ui/salon_parking.cpp`, système `salon3d`, ordre 452) : le parking du menu (WFS-552, **même instance**, `parking::commun()`), sa lumière (`Parking::eclairer`, sortie de `regler` sans changer le menu), la voiture du joueur (Game), une 2e voiture garée à côté (voiture de l'adversaire dès qu'il est connu, avec sa peinture ; avant : une voiture du jeu, noire, RG-03), et sur le poteau de gauche : **digicode d'interphone en inox brossé 120 × 180 × 30 mm** (boîtier chanfreiné, joint, afficheur à cristaux liquides sous son cadre, 36 touches de 15 mm au pas de 19 mm avec collerette, 4 vis inviolables), **panneau nominatif** 600 × 200 mm (aluminium, bord embouti, bande orange « WFS », rivets, 4 entretoises six pans de 30 mm, lettres de 120 mm), **gaine IRL Ø 20 mm** sortie par un presse-étoupe PG 16, coude cintré de 60 mm de rayon, colliers ; inox brossé (stries générées au chargement) ; vis inviolables à empreinte 6 lobes et téton. Aucune forme nue : chaque pièce est chanfreinée ou tournée au niveau du vrai objet (sources en tête du fichier). 6 400 triangles, 9 appels de dessin, tout construit au chargement.
  - **Digicode = clavier du code** (RG-01, RG-05, RG-06 arbitrés par le PO) : vue large de la maquette, digicode à vraie taille ; un toucher sur lui fait glisser la caméra en 0,4 s (courbe adoucie) tout près : touches de 89 px en 2400 × 1080 (zones touchables = pas de 19 mm) ; une lettre par appui + 150 ms (filtre de WFS-545, inchangé) ; ⌫ et OK (= « Rejoindre ») ; « Retour » (et le geste retour d'Android, qui passe par le même bouton) ramène d'abord au cadrage large, puis au menu. RG-05 (arbitrage du PO, 9c6c9a25) : en vue large, un code commencé est recopié dans le panneau (« CODE » + 4 cases), l'afficheur reste allumé. Pavé 6 × 6 : 2 à 9 puis A à Z sans I, L, O ; ⌫ sur 2 colonnes, OK sur 3.
  - **Accueil** (RG-07) : pseudo seulement sur la plaque 3D ; panneau de verre « Rejoins avec un code » en haut à droite avec l'aide, puis « Salons à côté de toi » et « Derniers adversaires » **seulement s'ils ne sont pas vides** (mêmes touchers) ; masqué en vue rapprochée. Fond net (plus de flou) à l'accueil ; **salon** (hôte, prêt) : même scène, floutée, sous la mise en page 2D de la maquette 23 (RG-08).
  - Textes FR / EN : `salon.rejoins_code`, `salon.aide_digicode1/2`, `salon.code_tape`.
  - Salon (maquette 23) : aucune pièce 3D ni texte projeté net par-dessus la scène floutée (`Salon3d::drawUi` coupé dans le salon).
  - Repli (mémoire graphique refusée ou `WFS_SALON_SANS_3D`) : écran 2D d'avant, intact.
  - Accroches : `menu.cpp` (1 ligne : parking partagé), `menu_parking.*` (`eclairer`, `commun`), `desktop_main.cpp` (1 ligne : banc `WFS_BANC=salon`, salon seul).
- **Maquette ↔ code** (revue du lead dev) :
  - maquette 04, parking, éclairage ↔ `menu_parking.cpp:Parking::init` / `Parking::eclairer` (réutilisés) : conforme ;
  - maquette 04, pilier, digicode, plaque « ALEX » ↔ `salon_parking.cpp:Decor::init` (géométrie) et `Salon3d::drawUi` (légendes, code tapé, pseudo) : conforme, **écarts acceptés par le PO** : digicode à vraie taille en vue large (RG-05) et pavé 6 × 6 alphanumérique au lieu du pavé 1-9 (RG-06) ;
  - maquette 04, voiture du joueur + 2e voiture ↔ `Salon3d::scene` (s.joueur) et `Decor::dessiner` : conforme (2e voiture à droite, sur la place voisine) ;
  - maquette 04, panneau « Rejoins avec un code » ↔ `salon.cpp:Salon::dessinerPanneau3d` : conforme (place, taille, titre, aide) ;
  - maquette 04, cadrage ↔ `salon_digicode.cpp:camera(0)` : proche (poteau à gauche, voiture au centre, voisine à droite) ; caméra rapprochée après la 1re revue : pseudo en capitales de 23 px (test CPU ≥ 20 px) ; le poteau reste un peu plus loin que sur la maquette (vraie trame du parking : poteau à 4 m de l'axe de la place) ;
  - maquette 04, « Retour » et « Créer duel » ↔ hors périmètre (**WFS-557**, en cours : boutons non retouchés ici) ;
  - maquette 23 (salon) ↔ `salon.cpp:Salon::dessinerSalon` (inchangé, WFS-557 le refait) sur la scène floutée : conforme.
- **Comment c'est vérifié** :
  - `test_salon_digicode` (nouveau) : 31 caractères une fois chacun, 15 mm, pas de 19 mm, tout dans 120 × 180 mm, ⌫ / OK, vue rapprochée (digicode sur 98 % de la hauteur, touches 89 px), vue large (voiture, poteau à gauche, voisine à droite), glissé en 24 images à 60 img/s sans saut et sans traverser le poteau, retour en 0,4 s ; pseudo ≥ 20 px en vue large ; **tests CPU : 151 réussis** (`qa/pr/WFS-558/tests-cpu.log`).
  - Banc `menu2` (2400 × 1080) : parcours complet du salon : toucher du digicode → vue rapprochée → K, 7, Q, 2 tapés sur les touches 3D → « effacer » maintenu (un seul caractère retiré, WFS-545) → « Retour » → cadrage large, écran Salon gardé (« 0 écart ») → Créer, Rejoindre par Internet, hors ligne, Quitter, Retour → menu (« 0 écart ») ; tuile « Rejoindre Nadia » du panneau touchée → salon rejoint (« 0 écart ») ; derniers adversaires (WFS-122) : toucher dans le panneau OK. Lignes du journal du banc : `qa/pr/WFS-558/banc-menu2.journal`.
  - Captures EN (banc `salon`, `WFS_LANGUE=en`) ajoutées à la main : `apres-en-*.jpg` (accueil, code tapé, retour au cadrage large), refaites avec le code final.
- **Limites et risques** :
  - **Temps d'image sur le S21 FE non mesuré** (RG-C5 ≤ 33 ms) : pas de téléphone ici ; la scène = le parking du menu + 6 400 triangles + une 2e voiture ; contrairement au menu, l'image n'est pas figée (la caméra glisse). À regarder par l'utilisateur (ticket 🟡).
  - La voiture d'un adversaire **est chargée quand il arrive** dans le salon (hors de l'image, dans `update`, comme `DuelSystem`), pas au chargement de la partie : impossible de connaître sa voiture avant ; la voiture garée d'attente, elle, est chargée au chargement.
  - Légendes des touches, code de l'afficheur et pseudo de la plaque : textes 2D projetés sur leur pièce 3D à chaque image (pas de gravure en géométrie).
  - 2 erreurs **déjà présentes sur `main`** dans le parcours « duel entre amis » du banc (`écran de fin : classement… et temps`, `vrais temps…`, temps 0,0 s) : mêmes lignes dans le journal « avant » (main 68a71e26) et « après » ; sans lien avec ce ticket.
  - Conflit attendu avec **WFS-557** (même fichier `salon.cpp`, boutons et salon 2D) : je synchronise avec `main` dès sa fusion.

### Captures EN (après seulement)

| Capture | Après (branche, anglais) |
|---|---|
| `salon-accueil` | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/apres-en-salon-accueil.jpg?raw=true) |
| `salon-digicode-retour` | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/apres-en-salon-digicode-retour.jpg?raw=true) |
| `salon-code-saisi` | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/apres-en-salon-code-saisi.jpg?raw=true) |

## Tests CPU

`bash tools/qa/tests_cpu.sh build` sur la branche (journal complet : [`qa/pr/WFS-558/tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/tests-cpu.log)) :

```
[OK] test_salon_digicode (0 s)
tests CPU : 151 test(s) réussi(s) en 502 s
```

## Captures pour le lead dev (14 après)

Dossier : [`qa/pr/WFS-558`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558) — produit par `tools/qa/captures_pr.sh`.

Captures « avant » : `origin/main` = `73b7f6fd4f8368c80576d04d6c2ccf1e9127e84e` ; cache : 0 banc(s) relu(s), 1 banc(s) rejoué(s) ; main compilé : oui.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `salon-accueil` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/avant-salon-accueil.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/apres-salon-accueil.jpg?raw=true) |
| `salon-amis-fin` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/avant-salon-amis-fin.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/apres-salon-amis-fin.jpg?raw=true) |
| `salon-amis` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/avant-salon-amis.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/apres-salon-amis.jpg?raw=true) |
| `salon-code-efface` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/avant-salon-code-efface.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/apres-salon-code-efface.jpg?raw=true) |
| `salon-code-saisi` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/avant-salon-code-saisi.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/apres-salon-code-saisi.jpg?raw=true) |
| `salon-code` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/avant-salon-code.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/apres-salon-code.jpg?raw=true) |
| `salon-derniers` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/avant-salon-derniers.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/apres-salon-derniers.jpg?raw=true) |
| `salon-digicode-glisse` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/apres-salon-digicode-glisse.jpg?raw=true) |
| `salon-digicode-proche` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/apres-salon-digicode-proche.jpg?raw=true) |
| `salon-digicode-retour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/apres-salon-digicode-retour.jpg?raw=true) |
| `salon-hors-ligne` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/avant-salon-hors-ligne.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/apres-salon-hors-ligne.jpg?raw=true) |
| `salon-hote` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/avant-salon-hote.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/apres-salon-hote.jpg?raw=true) |
| `salon-internet` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/avant-salon-internet.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/apres-salon-internet.jpg?raw=true) |
| `salon-pret` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/avant-salon-pret.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-558/qa/pr/WFS-558/apres-salon-pret.jpg?raw=true) |
