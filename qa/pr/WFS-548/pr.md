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
  - `bash tools/qa/tests_cpu.sh build` : **145 tests réussis** (voir « Tests CPU »), dont le nouveau `test_verre` (dégradé de la marque, couleurs d'état distinctes, sur-titres gris, contraste ≥ 4,5:1 sur le verre sombre).
  - Bancs PC (lavapipe, même rendu que la CI) avant / après avec `captures_pr.sh` : `pause`, `profil`, `creation`, `cgu`, `market`, `chargement`, `reglages-onglets`, `course5` et `menu2` (fins de course à 5, Poursuite et duel, salon), `garage-voitures` (fenêtre d'achat, au garage et sur la fin d'essai). Après seulement : pause **de nuit** (`WFS_MOMENT=nuit`).
  - Parcours (RG-09) : chaque écran du parcours (menu → course → pause → reprise → fin → menu, Réglages, Profil, Market, CGU, salon) regardé sur les captures des bancs ci-dessus, FR et EN : aucun écran cassé ni texte coupé vu. Le banc `parcours` complet n'a pas été lancé en local (bancs du matin).
  - RG-08 : aucun flou en course (capture `pause-reprise` nette) ; à l'ouverture, une seule image refait la scène et la chaîne du bloom (même coût qu'une image de course avec bloom), puis l'image est relue.
- **Limites et risques connus** :
  - Seule la **scène 3D** est floutée. Quand une fenêtre de verre est ouverte sur un autre écran (Réglages, CGU, Profil, Market, création du profil sur le menu ; Market ou fenêtre d'achat sur le garage ; fenêtre d'achat sur la fin de course ; Réglages sur la pause), **l'écran du dessous ne dessine plus son interface** (`verre::fenetreSurEcran` / `verre::couvert()`, système `verre-couvre` d'ordre 1) : derrière le verre, il ne reste que la scène floutée sous un voile (0,55 ; 0,50 sous la fenêtre d'achat). Les zones de toucher du dessous ne changent pas (les fenêtres gardent déjà leurs touchers).
  - Premier lancement (CGU avant toute partie) et chargement : pas de scène derrière, fond `--bg` gardé.
  - Les gouttes de pluie (effet d'écran) restent nettes sur la fin de la Poursuite de nuit.
  - Les bancs de course ne sont pas déterministes au dixième près : sur `salon-amis-fin`, le temps et le bonus de chocs diffèrent entre avant et après (course rejouée), sans lien avec ce ticket.
  - **Banc `score-fin` : 40 échecs identiques sur `main` et sur la branche** (déjà là avant ce ticket : ANO-274, ticket WFS-553) ; ses captures ont été retirées ; les fins de course sont prises avec les bancs `course5` et `menu2`, avant **et** après.
  - Pas encore vu sur un vrai téléphone (GPU mobile) : à valider par l'utilisateur (🟡).
  - WFS-549 touche les mêmes écrans : le 2e fusionné se synchronise et refait ses captures.

## Tests CPU

- `bash tools/qa/tests_cpu.sh build` sur le commit `897f8704` : **145 tests réussis, 0 échec** (journal complet : [`qa/pr/WFS-548/tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/tests-cpu.log)).
- Nouveau `test_verre` : dégradé de la marque, couleurs d'état distinctes, sur-titres gris, contraste ≥ 4,5:1, `contourArrondi` (rayon 14 : points dans le rectangle, sens, aire ±1 % ; rayon 0 ; rayon 500 borné ; taille négative sans NaN).
- `test_audio_chocs` a échoué une fois sur sa mesure de coût pendant une compilation parallèle ; il passe seul et dans la série complète sans charge (rien à voir avec ce ticket).

## Captures pour le lead dev

Dossier complet : [`qa/pr/WFS-548`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548) (188 paires avant / après, 2 après seules, 1280x720, branche au commit `77b0d190`) — produit par `tools/qa/captures_pr.sh`, tableau complet dans `README.md`.

### Captures clés (avant `main` / après branche)

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `pause-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-pause-0.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-pause-0.jpg?raw=true) |
| `pause-quitter` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-pause-quitter.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-pause-quitter.jpg?raw=true) |
| `reglages-onglet-affichage-menu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-reglages-onglet-affichage-menu.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-reglages-onglet-affichage-menu.jpg?raw=true) |
| `reglages-onglet-conduite-pause` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-reglages-onglet-conduite-pause.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-reglages-onglet-conduite-pause.jpg?raw=true) |
| `reglages-cgu-bouton` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-reglages-cgu-bouton.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-reglages-cgu-bouton.jpg?raw=true) |
| `reglages-cgu-lecture` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-reglages-cgu-lecture.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-reglages-cgu-lecture.jpg?raw=true) |
| `profil-fr-plein` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-profil-fr-plein.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-profil-fr-plein.jpg?raw=true) |
| `creation-fr-rempli` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-creation-fr-rempli.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-creation-fr-rempli.jpg?raw=true) |
| `cgu-premier` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-cgu-premier.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-cgu-premier.jpg?raw=true) |
| `market-fr-market` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-market-fr-market.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-market-fr-market.jpg?raw=true) |
| `market-fr-garage-market` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-market-fr-garage-market.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-market-fr-garage-market.jpg?raw=true) |
| `market-en-garage-market` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-market-en-garage-market.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-market-en-garage-market.jpg?raw=true) |
| `garage-voitures-fin-essai-confirmation` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-garage-voitures-fin-essai-confirmation.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-garage-voitures-fin-essai-confirmation.jpg?raw=true) |
| `garage-voitures-m4-confirmation` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-garage-voitures-m4-confirmation.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-garage-voitures-m4-confirmation.jpg?raw=true) |
| `chargement-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-chargement-1.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-chargement-1.jpg?raw=true) |
| `course5-fin` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-course5-fin.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-course5-fin.jpg?raw=true) |
| `salon-amis-fin` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-salon-amis-fin.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-salon-amis-fin.jpg?raw=true) |
| `poursuite-arrete` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-poursuite-arrete.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-poursuite-arrete.jpg?raw=true) |
| `salon-hote` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-salon-hote.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-salon-hote.jpg?raw=true) |
| `salon-pret` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-salon-pret.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-salon-pret.jpg?raw=true) |
| `pause-reprise` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-pause-reprise.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-pause-reprise.jpg?raw=true) |
| `course5-course` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/avant-course5-course.jpg?raw=true) | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-course5-course.jpg?raw=true) |

### Après seulement (pause de nuit)

| Capture | Après (branche) |
|---|---|
| `nuit-pause-0` | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-nuit-pause-0.jpg?raw=true) |
| `nuit-pause-reprise` | ![apres](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-548/qa/pr/WFS-548/apres-nuit-pause-reprise.jpg?raw=true) |
