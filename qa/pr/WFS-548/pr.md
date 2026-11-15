## Agent : Claude, agent dev3d (01Us6uCucP)

**Ticket** : WFS-548 — règles : `regles-de-gestion/WFS-548.md`, épic `regles-de-gestion/E23-interface-r19.md`, maquettes `references/maquettes-R19/`

## Description

- **Demande de l'utilisateur** : les écrans à fenêtres passent au modèle R19 : panneaux en verre dépoli, fond flouté derrière les fenêtres, plus aucun petit titre bleu (interface R19, 1/5).
- **Ce qui change** :
  - **`src/ui/verre.{h,cpp}` (nouveau, RG-01, RG-03 à RG-05)** : un seul dessin de fenêtre pour tous les écrans (`verre::panneau` : dégradé radial plus clair au centre-haut, liseré clair ≈ 20 %, coins arrondis, ombre douce ; tuiles plus légères ; liseré de couleur d'état), `verre::surTitre` (gris, capitales espacées), couleurs d'état communes (`TOI` orange, `PERTE` rouge, `PRET` vert, `INTERTITRE` orange clair des CGU), dégradé de la marque cyan → orange (`degradeMarque`, `barreMarque`, mêmes valeurs que l'ancienne barre de chargement).
  - **Fond flouté des écrans arrêtés (RG-02)** : `verre::fondFloute(r)` demande le flou pour l'image suivante. Le moteur réutilise la chaîne du bloom (`post.frag` modes 1 à 4) **sans seuil** et la passe finale lit cette image floutée à la place de la scène ; l'image floutée est **calculée une fois** puis **relue telle quelle** (même mécanisme que le fond figé de WFS-329 : aucun dessin 3D tant qu'elle reste valable). **Jamais en course** : garde dans `Game::render` (1re image après « Reprendre » comprise) ; l'écran de fin, lui, est flouté.
  - **Accroche du rendu annoncée au journal** : `renderer.h` / `renderer.cpp` **8 lignes** (≤ 10) ; `shaders/post.frag` : mode 1 sans seuil si `params.y = 1`, passe finale : `params.z < 0` = lire le bloom flouté (halo des zones lumineuses sauté). Rien ne change quand aucun écran ne demande le flou.
  - **Écrans (RG-06)** : pause (19), profil (21), création du profil (20), CGU (22 : intertitres orange clair, titre blanc), réglages (14, 24, 25), Market (12), chargement (08, 08b), salon du duel (23 : ta ligne bordée et écrite en orange, « Prêt » en vert), fenêtre d'achat du garage (27, 28), fins de course Checkpoint (10), Course à 5 (31 : chaque ligne sur une tuile, la tienne en orange), Poursuite (32 : « Arrêté » en rouge), duel (33) et bloc « Score » (11). « Quitter la course » bordé et écrit en rouge (RG-05).
  - **Accroches hors de `src/ui/`** (écrans de fin listés par RG-06, dessinés dans `src/game/`) : `course_system.cpp`, `course5_system.cpp`, `poursuite_system.cpp`, `duel_system.cpp`, `score_fin.cpp` (voile → fond flouté, surface → `verre::panneau`, sur-titre cyan → gris), `game.cpp` (1 ligne : pas de flou en course). Annoncées au journal.
  - **Inchangés** : textes, options, disposition, taille et dessin des boutons (RG-07, WFS-549), HUD et jauges (WFS-550, 551).
- **Comment c'est vérifié** :
  - `bash tools/qa/tests_cpu.sh build` : **145 tests réussis**, dont le nouveau `test_verre` (dégradé de la marque, couleurs d'état distinctes, sur-titres gris, contraste ≥ 4,5:1 sur le verre sombre).
  - Bancs PC (lavapipe, même rendu que la CI) avant / après avec `captures_pr.sh` : `pause`, `profil`, `creation`, `cgu`, `market`, `chargement`, `reglages-onglets`, `score-fin` (FR et EN). Après seulement : fins de course à 5, Poursuite et duel (`course5`, `menu2`), salon (`menu2`), fenêtre d'achat (`garage-voitures`), pause **de nuit** (`WFS_MOMENT=nuit`).
  - Parcours (RG-09) : chaque écran du parcours (menu → course → pause → reprise → fin → menu, Réglages, Profil, Market, CGU, salon) regardé sur les captures des bancs ci-dessus, FR et EN : aucun écran cassé ni texte coupé vu. Le banc `parcours` complet n'a pas été lancé en local (bancs du matin).
  - RG-08 : aucun flou en course (capture `pause-reprise` nette) ; à l'ouverture, une seule image refait la scène et la chaîne du bloom (même coût qu'une image de course avec bloom), puis l'image est relue.
- **Limites et risques connus** :
  - Seule la **scène 3D** est floutée : l'interface de l'écran dessous (menu, écran de fin) est cachée par un voile sombre (0,72 sur le menu, 0,78 sous la fenêtre d'achat), pas floutée ; elle reste à peine visible sous la fenêtre.
  - Premier lancement (CGU avant toute partie) et chargement : pas de scène derrière, fond `--bg` gardé.
  - La fenêtre de pause n'est plus dessinée quand les Réglages sont ouverts dessus, ni le panneau des Réglages sous les CGU (ils se voyaient à travers le verre).
  - Les gouttes de pluie (effet d'écran) restent nettes sur la fin de la Poursuite de nuit.
  - **Banc `score-fin` : 40 échecs identiques sur `main` et sur la branche** (déjà là avant ce ticket : l'appui sur le compteur d'étoiles du menu n'ouvre pas le garage, puis le banc reste au garage) ; ses captures Course à 5, Poursuite et duel montrent donc le garage et ont été retirées. Signalé au PO.
  - Pas encore vu sur un vrai téléphone (GPU mobile) : à valider par l'utilisateur (🟡).
  - WFS-549 touche les mêmes écrans : le 2e fusionné se synchronise et refait ses captures.

## Captures pour le lead dev

Dossier complet : [`qa/pr/WFS-548`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548) (71 paires avant / après, 10 après seules) — produit par `tools/qa/captures_pr.sh`, tableau complet dans `README.md`.

### Captures clés (avant `main` / après branche)

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `pause-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-pause-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-pause-0.jpg?raw=true) |
| `pause-reprise` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-pause-reprise.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-pause-reprise.jpg?raw=true) |
| `profil-fr-apres-course` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-profil-fr-apres-course.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-profil-fr-apres-course.jpg?raw=true) |
| `profil-fr-plein` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-profil-fr-plein.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-profil-fr-plein.jpg?raw=true) |
| `creation-fr-rempli` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-creation-fr-rempli.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-creation-fr-rempli.jpg?raw=true) |
| `cgu-premier` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-cgu-premier.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-cgu-premier.jpg?raw=true) |
| `reglages-cgu-lecture` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-reglages-cgu-lecture.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-reglages-cgu-lecture.jpg?raw=true) |
| `reglages-onglet-conduite-pause` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-reglages-onglet-conduite-pause.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-reglages-onglet-conduite-pause.jpg?raw=true) |
| `reglages-onglet-affichage-menu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-reglages-onglet-affichage-menu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-reglages-onglet-affichage-menu.jpg?raw=true) |
| `market-fr-market` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-market-fr-market.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-market-fr-market.jpg?raw=true) |
| `market-en-achat` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-market-en-achat.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-market-en-achat.jpg?raw=true) |
| `chargement-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-chargement-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-chargement-1.jpg?raw=true) |
| `chargement-erreur` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-chargement-erreur.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-chargement-erreur.jpg?raw=true) |
| `score-fin-fr-solo-1280x720` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-score-fin-fr-solo-1280x720.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-score-fin-fr-solo-1280x720.jpg?raw=true) |

### Après seulement (fins de course, salon, achat, nuit)

| Capture | Après (branche) |
|---|---|
| `achat-confirmation` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-achat-confirmation.jpg?raw=true) |
| `achat-fin-essai` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-achat-fin-essai.jpg?raw=true) |
| `achat-il-manque` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-achat-il-manque.jpg?raw=true) |
| `fin-course5` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-fin-course5.jpg?raw=true) |
| `fin-duel` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-fin-duel.jpg?raw=true) |
| `fin-poursuite-arrete` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-fin-poursuite-arrete.jpg?raw=true) |
| `nuit-pause-0` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-nuit-pause-0.jpg?raw=true) |
| `nuit-pause-reprise` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-nuit-pause-reprise.jpg?raw=true) |
| `salon-hote` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-salon-hote.jpg?raw=true) |
| `salon-pret` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-salon-pret.jpg?raw=true) |
