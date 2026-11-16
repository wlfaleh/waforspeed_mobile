Agent : Claude, agent dev3d (01UY1Vv4Fd)

## Demande de l'utilisateur
« Une seule tâche pour vérifier le travail qui a été mergé jusqu'à maintenant : est-ce qu'il a respecté à 100 % les maquettes 19 ou non ; si non, il corrige » (WFS-557, épic E23 : WFS-548, 549, 552).

## Arbitrages du PO cités (`regles-de-gestion/WFS-557.md`)
- **RG-06 à RG-09** : maquette à 100 %, forme, place et taille des boutons comprises ; zone de toucher ≥ 44 px ; élément sans place → PO.
- **RG-10 à RG-13** : Inter (OFL 1.1, `rsms/inter` v4.1) en 2e police, dans le même atlas que Rajdhani, pour le texte courant seulement.
- **RG-14** : plus de champ « Plaque » à part (la plaque suit le pseudo ; toucher l'aperçu la rend modifiable). **RG-15** : « · record » calculé à l'affichage ; aide de la liste retirée. **RG-16** : menu **transféré à WFS-550** (non touché ici). **RG-17** : « CP n » sans total en Checkpoint. **RG-18** : Market (onglets, ✕, pack débutant en 3 états, liens du bas). **RG-19** : CGU sans case ni « Refuser » (« J'accepte » après lecture des 2 onglets, refus par la touche Retour). **RG-20** : scènes 3D 04, 09, 10, 11 hors périmètre (WFS-558 à 561), seuls leurs éléments 2D ici. **RG-21 / RG-22** : choix du mode, distance et difficulté dans la carte Course à 5 ; images des modes provisoires (WFS-562). **RG-23** : choix de la carte (maquettes 03, 18a à 18d) traité ici (WFS-565 annulé sur ordre de l'utilisateur).

## Ce qui change
Tableau maquette ↔ écran ↔ fichier:fonction ↔ verdict : [`qa/pr/WFS-557/verification.md`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/verification.md) ; images maquette / jeu côte à côte : `qa/pr/WFS-557/cote-a-cote/`.
- **Police** : `assets/fonts/Inter-Regular.ttf`, `Inter-Medium.ttf` (+ licence, crédits, `verifier_licences.py`) ; `src/ui/text.*` : plusieurs faces dans le même atlas (Rajdhani en premier, pixels inchangés ; atlas rempli à 711 px sur 1024), `ui::AvecPolice` ; Inter légèrement épaissie d'un demi-pixel d'atlas (sinon « i », « l » pâles aux petites tailles) ; `ui::chrono` avec virgule en français.
- **Boutons communs** (`src/ui/boutons.*`) : icônes au trait, bouton de verre à icône, bouton « plaque » ; **bague chrome des boutons ronds (Start / Stop : Reprendre, Rouler, Rejouer…) mesurée au pixel sur la maquette 19** (`bagueChrome` : anneau 0x55585E → 0x2A2E33 plus épais, reflet blanc en haut, perspective décalée vers le bas, centre noir, liseré orange fin plus clair en haut, icône pêche, texte remonté).
- **Écrans** : pause (19), Réglages (14, 24, 25 : une seule mise en page téléphone + PC), profil (21), création du profil (20), chargement (08, 08b), CGU (22), Market (12), choix du mode (02), salon (23) et accueil du duel (04, boutons), fins Course à 5 / Poursuite / Duel (31, 32, 33 : mise en page commune `panneauFinR19` dans `course_system.cpp`), fin Checkpoint (10 : plaque « MENU », « Rejouer » rond), **choix de la carte (18a, 18b, 18c : `terrain.cpp : Terrain::drawUi`, `paquets.cpp : dessinerPaquetCarte, etiquette, boutonVerre`)**, cadrage 3D du menu de WFS-552 (voiture ≈ 21 % plus grande, comme la maquette 01 ; `test_menu_parking` vert).
- **Accroches** : `src/platform/android_main.cpp` (1 ligne : Retour = refus des CGU au 1er lancement) ; `src/render/renderer.cpp` (4 lignes : limite de 8 192 quadrilatères de l'interface notée au journal, elle avait coupé le bas du Market pendant le travail).
- **Hors périmètre** (WFS-550 / 551) : garage, achat (27, 28), HUD (17), étape des paquets (18d, déjà faite), cadeau (05), prêt, gains, messages en course, menu (01).

## Vérification
- Banc PC (lavapipe) en 1600 × 720 (format des maquettes), chaque écran comparé côte à côte à sa maquette, puis corrigé jusqu'à l'identique.
- Bancs fonctionnels : `paquets` 0 écart (choix de la carte : à télécharger, progression, installée, suppression), `cgu` 0 écart (nouveau parcours RG-19), `creation` 0 écart, `market` 0 écart (FR + EN), `menu3d` 0 écart, `profil`, `pause`, `reglages-onglets`, `chargement` passés.
- Tests CPU : 150 / 150 verts (`bash tools/qa/tests_cpu.sh build`) ; licences : 0 manque.

## Limites et risques
- Maquette 20 : touches de 62 × 52 px (≈ 4,7 mm sur un téléphone 2400 × 1080) au lieu des 9 mm de WFS-481 (règle remplacée par l'arbitrage RG-07, zones jointives ≥ 44 px) ; le banc `creation` vérifie maintenant ≥ 44 px.
- Écrans 16:9 : création, Market, choix du mode, salon réduits pour tenir en largeur et centrés.
- Images des modes (02) et des cartes (18) provisoires (WFS-562 pour les modes ; vignettes dessinées pour les cartes) ; scènes 3D 04, 09, 10, 11 : WFS-558 à 561.
- Texte « Sans pub » du Market : celui du jeu est gardé (il décrit le vrai comportement de WFS-512 ; la maquette dit le contraire).
- Pas compilé pour Android en local (accroche d'une ligne dans `android_main.cpp`).

Planches maquette / avant / après : [`qa/pr/WFS-557/cote-a-cote/`](https://github.com/wlfaleh/waforspeed_mobile/tree/feature/waforspeed-557/qa/pr/WFS-557/cote-a-cote) (dont 18a, 18b, 18c pour le choix de la carte et le bouton Start / Stop).

## Captures pour le lead dev (22 paires, 1600 × 720)

Dossier : [`qa/pr/WFS-557`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557) — produit par `tools/qa/captures_pr.sh`.

- Écrans R19 : `origin/main` = `088f306c6d010358c3109646831b9363bdb12f26` ; cache : 0 banc(s) relu(s), 10 banc(s) rejoué(s) ; main compilé : oui.
- Choix de la carte (RG-23) : `origin/main` = `9c6c9a25c43067a04b7a8728e4fe1275137154e5` ; cache : 0 banc(s) relu(s), 2 banc(s) rejoué(s) ; main compilé : oui.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `cgu-coche` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/avant-cgu-coche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/apres-cgu-coche.jpg?raw=true) |
| `cgu-premier` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/avant-cgu-premier.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/apres-cgu-premier.jpg?raw=true) |
| `chargement-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/avant-chargement-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/apres-chargement-1.jpg?raw=true) |
| `chargement-erreur` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/avant-chargement-erreur.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/apres-chargement-erreur.jpg?raw=true) |
| `creation-fr-rempli` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/avant-creation-fr-rempli.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/apres-creation-fr-rempli.jpg?raw=true) |
| `market-fr-market` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/avant-market-fr-market.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/apres-market-fr-market.jpg?raw=true) |
| `market-fr-sans-pub` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/avant-market-fr-sans-pub.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/apres-market-fr-sans-pub.jpg?raw=true) |
| `duel-resultat-4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/avant-duel-resultat-4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/apres-duel-resultat-4.jpg?raw=true) |
| `salon-pret` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/avant-salon-pret.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/apres-salon-pret.jpg?raw=true) |
| `menu3d-1600x720-sfx` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/avant-menu3d-1600x720-sfx.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/apres-menu3d-1600x720-sfx.jpg?raw=true) |
| `pause-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/avant-pause-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/apres-pause-0.jpg?raw=true) |
| `profil-fr-plein` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/avant-profil-fr-plein.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/apres-profil-fr-plein.jpg?raw=true) |
| `reglages-onglet-affichage-menu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/avant-reglages-onglet-affichage-menu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/apres-reglages-onglet-affichage-menu.jpg?raw=true) |
| `reglages-onglet-conduite-pause` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/avant-reglages-onglet-conduite-pause.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/apres-reglages-onglet-conduite-pause.jpg?raw=true) |
| `textes-fr-duel-fin` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/avant-textes-fr-duel-fin.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/apres-textes-fr-duel-fin.jpg?raw=true) |
| `textes-fr-fin-course5` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/avant-textes-fr-fin-course5.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/apres-textes-fr-fin-course5.jpg?raw=true) |
| `textes-fr-fin-poursuite` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/avant-textes-fr-fin-poursuite.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/apres-textes-fr-fin-poursuite.jpg?raw=true) |
| `terrain-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/avant-terrain-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/apres-terrain-0.jpg?raw=true) |
| `terrain-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/avant-terrain-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/apres-terrain-1.jpg?raw=true) |
| `paquets-terrain-installee` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/avant-paquets-terrain-installee.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/apres-paquets-terrain-installee.jpg?raw=true) |
| `paquets-terrain-progression-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/avant-paquets-terrain-progression-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/apres-paquets-terrain-progression-2.jpg?raw=true) |
| `paquets-terrain-telecharger` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/avant-paquets-terrain-telecharger.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-557/qa/pr/WFS-557/apres-paquets-terrain-telecharger.jpg?raw=true) |
