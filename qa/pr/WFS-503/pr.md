## Agent : Claude, agent dev3d (01Us6uCwe9)

**Ticket** : WFS-503 — règles : `regles-de-gestion/WFS-503.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : le cadeau du jour donne pour de vrai ce qu'il annonce : les étoiles vont au portefeuille, le coloris et la skin gagnés appartiennent au joueur (épic E19, `regles-de-gestion/WFS-503.md`, RG-01 à RG-05 ; arbitrage du PO du 2026-11-15 23:17 UTC : données du « Nouveau » ici, badge au garage dans WFS-556).
- **Ce qui change** (fichiers, comportement, à l'écran) :
  - `src/game/cadeau.*` : nouvelle fonction `donner(s, r)`, appelée par `ouvrir()` **avant la même écriture de la sauvegarde** que le cadeau (RG-08 de WFS-500) : étoiles par `boutique::crediter(n, "cadeau")` (RG-01) ; coloris ajouté à `possede-<voiture>` sans être posé (RG-02) ; skin par `skins::donnerSkin` (RG-03, « cadeau seulement » comprise) ; voiture : prêt de WFS-502, inchangé. Si l'écriture échoue : état du cadeau, solde et possessions remis, rien donné, le cadeau reste disponible. `estNouveau()` / `vu()` : objets gagnés pas encore vus (arbitrage du PO).
  - `src/game/sauvegarde.*` : clé nouvelle `cadeau-nouveaux=<voiture>:<objet>|…` (absente tant que rien n'attend ; ancienne sauvegarde lue sans erreur ; éléments mal écrits ignorés un à un ; aucune clé ancienne retirée, RC-E19-03).
  - RG-04 : `entreeDepuis()` (WFS-500) lisait déjà le catalogue de WFS-487 et les voitures de WFS-498 (hors retirées) : réutilisé tel quel, vérifié par les tests.
  - `src/game/cadeau_gains_banc.cpp` (nouveau) : banc `WFS_BANC=cadeau-gains`, vraie sauvegarde et **vrai tirage** de l'écran du cadeau (rien de forcé à l'affichage), FR puis EN. Accroche d'1 ligne (+ 1 include) dans `src/platform/desktop_main.cpp`, annoncée au journal.
  - `CMakeLists.txt` : `skin_offerte.cpp` ajouté aux 6 tests qui compilent `cadeau.cpp` (aucun nouveau test exécutable).
  - Aucun fichier `src/ui/**` ni `assets/textes/**` modifié : l'écran du cadeau (WFS-501) affiche déjà les textes du gain.
- **Comment c'est vérifié** :
  - `tests/test_cadeau.cpp`, 21 cas nouveaux (RG-05) : étoiles créditées une seule fois (2e ouverture le même jour et relance : rien de plus) ; coloris possédé après le cadeau et après relance, pas appliqué (aspect de course inchangé) ; skin possédée, pas posée, après relance, sortie du tirage suivant ; skins « cadeau seulement » dans le tirage ; tout possédé → étoiles ≥ 10 sur 60 jours (garantie comprise), toutes créditées ; garantie avec toutes les skins possédées → voiture prêtée ; sauvegarde impossible → ni étoile ni objet (40 graines) ; `cadeau-nouveaux` : nouveau → `vu` → plus nouveau, gardé après relance, ancienne sauvegarde sans la clé, éléments mal écrits.
  - Suite complète `bash tools/qa/tests_cpu.sh build` : voir ci-dessous.
  - Parcours (banc `cadeau-gains`, 0 écart en FR et en EN) : menu (0 ★) → cadeau → +28 ★ → menu (28 ★) → 2e appui : attente, rien de plus → cadeau coloris (Cyan pour la 208) → garage, pastille sans cadenas → « Choisir » (0 ★ débité) → terrain → course de jour et de nuit en Cyan → cadeau skin (Camouflage) → garage, skin sans cadenas → relance (sauvegarde relue) : solde, Cyan choisi, Camouflage et 3 cadeaux gardés.
- **Limites et risques connus** :
  - Pas de badge « NOUVEAU » au garage : décision du PO, ticket WFS-556 (les fichiers du garage sont tenus par WFS-551). Les données (`cadeau-nouveaux`) sont prêtes.
  - Pour obtenir chaque type de gain, le banc ne garde que la chance de ce type dans `config().cadeau` (valeurs du jeu remises ensuite) et possède les finitions d'avance (le coloris tiré est alors une couleur) ; le tirage lui-même n'est pas changé.
  - Banc `cadeau` (WFS-501) : la sauvegarde inactive du banc reçoit maintenant le vrai gain ; les captures « avant / après » de ses menus sont identiques (le tirage du banc, graine 501, donne un coloris : solde inchangé à 0 ★).
  - Captures « avant » : le banc `cadeau-gains` n'existe pas sur `main` (« — » dans le tableau).
  - Ligne `[ERREUR] éclairage (WFS-213) : capture abandonnée` au journal du banc pendant la course de nuit : message du moteur d'éclairage, pas lié à ce ticket.

## Captures pour le lead dev (24 après)

Dossier : [`qa/pr/WFS-503`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `cadeau-gains-en-coloris-choisi-terrain` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/apres-cadeau-gains-en-coloris-choisi-terrain.jpg?raw=true) |
| `cadeau-gains-en-coloris-gain` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/apres-cadeau-gains-en-coloris-gain.jpg?raw=true) |
| `cadeau-gains-en-coloris-garage` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/apres-cadeau-gains-en-coloris-garage.jpg?raw=true) |
| `cadeau-gains-en-etoiles-gain` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/apres-cadeau-gains-en-etoiles-gain.jpg?raw=true) |
| `cadeau-gains-en-etoiles-menu-apres` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/apres-cadeau-gains-en-etoiles-menu-apres.jpg?raw=true) |
| `cadeau-gains-en-etoiles-menu-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/apres-cadeau-gains-en-etoiles-menu-avant.jpg?raw=true) |
| `cadeau-gains-en-relance-garage` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/apres-cadeau-gains-en-relance-garage.jpg?raw=true) |
| `cadeau-gains-en-skin-gain` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/apres-cadeau-gains-en-skin-gain.jpg?raw=true) |
| `cadeau-gains-en-skin-garage` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/apres-cadeau-gains-en-skin-garage.jpg?raw=true) |
| `cadeau-gains-fr-coloris-choisi-terrain` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/apres-cadeau-gains-fr-coloris-choisi-terrain.jpg?raw=true) |
| `cadeau-gains-fr-coloris-gain` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/apres-cadeau-gains-fr-coloris-gain.jpg?raw=true) |
| `cadeau-gains-fr-coloris-garage` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/apres-cadeau-gains-fr-coloris-garage.jpg?raw=true) |
| `cadeau-gains-fr-course-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/apres-cadeau-gains-fr-course-jour.jpg?raw=true) |
| `cadeau-gains-fr-course-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/apres-cadeau-gains-fr-course-nuit.jpg?raw=true) |
| `cadeau-gains-fr-etoiles-gain` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/apres-cadeau-gains-fr-etoiles-gain.jpg?raw=true) |
| `cadeau-gains-fr-etoiles-menu-apres` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/apres-cadeau-gains-fr-etoiles-menu-apres.jpg?raw=true) |
| `cadeau-gains-fr-etoiles-menu-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/apres-cadeau-gains-fr-etoiles-menu-avant.jpg?raw=true) |
| `cadeau-gains-fr-relance-garage` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/apres-cadeau-gains-fr-relance-garage.jpg?raw=true) |
| `cadeau-gains-fr-skin-gain` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/apres-cadeau-gains-fr-skin-gain.jpg?raw=true) |
| `cadeau-gains-fr-skin-garage` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/apres-cadeau-gains-fr-skin-garage.jpg?raw=true) |
| `cadeau-menu-attente-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/avant-cadeau-menu-attente-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/apres-cadeau-menu-attente-en.jpg?raw=true) |
| `cadeau-menu-attente-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/avant-cadeau-menu-attente-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/apres-cadeau-menu-attente-fr.jpg?raw=true) |
| `cadeau-menu-pastille-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/avant-cadeau-menu-pastille-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/apres-cadeau-menu-pastille-en.jpg?raw=true) |
| `cadeau-menu-pastille-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/avant-cadeau-menu-pastille-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-503/qa/pr/WFS-503/apres-cadeau-menu-pastille-fr.jpg?raw=true) |
