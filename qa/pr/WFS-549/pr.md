Agent : Claude, agent dev3d (01Us6uCvCC)

**Ticket** : WFS-549 — Interface R19 (2/5) : boutons — règles : `regles-de-gestion/WFS-549.md`, épic `regles-de-gestion/E23-interface-r19.md`, maquettes `references/maquettes-R19/`.

## Description

- **Demande de l'utilisateur** (en une phrase) : tous les boutons passent au modèle R19 (Start/Stop, autocollants, pilule chrome, prix orange, sélecteurs de verre, onglets) **sans changer leur taille**, sauf Menu et Caméra du HUD, agrandis comme sur la maquette 17b.
- **Ce qui change** :
  - **Nouveau dessin commun** `src/ui/boutons.{h,cpp}` (interface 2D, primitives lissées `dessin2d` existantes, aucun shader nouveau) : `startStop` / `startStopFond` (bague chrome, centre sombre, liseré orange allumé, icône marche), `autocollant` (étiquette blanche de travers, bord blanc, ombre), `piluleChrome` (liseré cyan, petite voiture au trait), `prix` (orange, texte sombre), `selecteurFond` + `selecteurChoix` (rangée de verre, choix actif = pilule blanche, texte sombre), `verreOrange` (« ✕ Fermer »), `verre` (boutons secondaires). Contour arrondi en calcul seul (`contourArrondi`, en ligne dans `boutons.h`).
  - **RG-01 Start/Stop** : Course solo et Duel en ligne (menu), Reprendre (pause), J'accepte (CGU), C'est parti / Valider (« Crée ton pilote »), Prêt (salon ; enfoncé quand on est prêt), Choisir / Acheter N ★ (garage, étoile dessinée comme avant), Confirmer (fenêtre d'achat), Prendre (skin offerte), Acheter (fin d'essai), Rouler (terrain), Continuer (choix du mode = « Suivant » 02), Rejouer (fins Checkpoint, Course à 5, Poursuite), Revanche (fin du duel).
  - **RG-02 Autocollant** : Retour (garage, terrain, modes, salon, Market, profil, crédits, cadeau, fin du duel), Quitter le salon, Annuler (achat, profil, paquets), Plus tard (skin offerte, paquets).
  - **RG-03** : « Showroom » du menu (ouvre le garage) en pilule chrome, liseré cyan, texte inchangé.
  - **RG-04** : prix du Market (0,99 € … 19,99 €, Sans pub) et « Inviter » du salon en boutons orange ; inactif (hors ligne, achat en cours) : même bouton grisé.
  - **RG-05 / RG-06** : sélecteurs de verre pour tous les choix des Réglages (deux mises en page), leurs onglets Affichage / Conduite / Son, Jour / Nuit (terrain), Principale / Toit et rétros (garage), distance du salon (grisée pour l'invité). « Fermer » des Réglages en verre cerclé d'orange (enfoncé à l'appui, WFS-290). « Crédits » ajouté à gauche de « Conditions d'utilisation » au menu principal : ouvre l'écran Crédits actuel par-dessus les Réglages (fermer les crédits ramène aux Réglages). Onglets du garage (rangées Finition / Skin) gardés.
  - **RG-07** : chaque bouton est dessiné **dans le même rectangle** qu'avant, zones de toucher calculées par le même code (aucune ligne de zone changée) ; seule exception : `hud::BOUTON_ECHELLE` 1,05 → 1,5 (Menu et Caméra : 42 → 60 px de haut à 720 p, mesuré sur la maquette 17b : 62 px), toujours centrés sur le score, sans recouvrement (`test_hud_boutons`).
  - **RG-08** : état pressé visible là où l'écran le suit déjà (menu, Réglages, Market, profil, crédits, cadeau, CGU) : centre plus clair, liseré plus vif ; aucun déclenchement changé (mêmes `onTouch`).
  - **Accroches hors zone annoncées au journal** (≤ 10 lignes chacune, dessin seul) : `src/game/course_system.cpp`, `course5_system.cpp`, `poursuite_system.cpp`, `duel_system.cpp`, `modes_ecran.cpp`.
- **Comment c'est vérifié** :
  - construction complète (Release, lavapipe) sans erreur ni nouvel avertissement ; **tests CPU 145 / 145** (`bash tools/qa/tests_cpu.sh build`), dont `test_hud_boutons` (échelle 1,5, rien ne se recouvre à 2400×1080, 1600×720, encoche, gaucher, poursuite) et le nouveau `test_boutons` (contour : dans le rectangle, horaire, convexe, ≤ 64 sommets, aire juste) ;
  - bancs `menu1` (menu, appui, garage, terrain, modes, pause, course, fin, retour au menu : même parcours qu'avant, les touchers du banc tombent dans les mêmes zones), `garage-couleurs`, `pause`, `reglages-onglets`, `market` (FR et EN), `cgu`, `hud` : captures avant / après ci-dessous, toutes regardées.
  - **Parcours complet (RG-09)** fait par les bancs : menu → Course solo → modes (Continuer) → garage (Choisir) → terrain (Rouler) → course → pause (Reprendre, Recommencer, Quitter) → fin (Rejouer, Menu) → Showroom → Retour ; garage Acheter / Annuler / il manque ; Réglages (onglets, choix, Fermer, CGU) au menu et en pause ; Market FR / EN (achat, attente, hors ligne, restauration, Retour) ; CGU au 1er lancement (cocher, Accepter, Refuser). Journaux des bancs : aucun bouton n'a changé d'effet.
- **Limites et risques connus** :
  - le **Market n'a pas d'onglets** aujourd'hui (Étoiles / Offres / Sans pub de la maquette 12) : non créés, ce serait un changement de disposition (hors RG-07) ; à trancher par le PO si voulu ;
  - icônes des onglets des Réglages (écran, volant, haut-parleur) laissées à WFS-551 (icônes) ;
  - « Crédits » seulement au **menu principal** : l'écran Crédits n'existe que là (en pause le bouton n'apparaît pas) ;
  - boutons secondaires non cités par les RG (Recommencer / Quitter la course de la pause, Menu des fins, Créer / Rejoindre du salon, Modifier du profil) : inchangés (WFS-548 / 550 / 551) ;
  - WFS-548 (verre dépoli) touche les mêmes écrans : le 2e fusionné se synchronise et refait ses captures ; `selecteurFond` pourra alors utiliser `verre.*` ;
  - rendu sur téléphone (vrai GPU) non vu : 🟡 jusqu'à l'essai de l'utilisateur.

## Captures pour le lead dev (56 après dans le dossier ; les 15 plus parlantes ici, toutes dans [`README.md`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/README.md))

Dossier : [`qa/pr/WFS-549`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549) — produit par `tools/qa/captures_pr.sh` (bancs `menu1:1280x720 garage-couleurs pause reglages-onglets market cgu hud:1280x720`).

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `menu-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/avant-menu-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/apres-menu-0.jpg?raw=true) |
| `menu-appui` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/avant-menu-appui.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/apres-menu-appui.jpg?raw=true) |
| `garage-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/avant-garage-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/apres-garage-0.jpg?raw=true) |
| `garage-couleurs-p208-il-manque` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/avant-garage-couleurs-p208-il-manque.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/apres-garage-couleurs-p208-il-manque.jpg?raw=true) |
| `garage-couleurs-p208-toit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/avant-garage-couleurs-p208-toit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/apres-garage-couleurs-p208-toit.jpg?raw=true) |
| `pause-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/avant-pause-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/apres-pause-0.jpg?raw=true) |
| `reglages-onglet-affichage-menu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/avant-reglages-onglet-affichage-menu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/apres-reglages-onglet-affichage-menu.jpg?raw=true) |
| `reglages-onglet-conduite-pause` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/avant-reglages-onglet-conduite-pause.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/apres-reglages-onglet-conduite-pause.jpg?raw=true) |
| `market-fr-market` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/avant-market-fr-market.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/apres-market-fr-market.jpg?raw=true) |
| `market-fr-achat` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/avant-market-fr-achat.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/apres-market-fr-achat.jpg?raw=true) |
| `cgu-coche` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/avant-cgu-coche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/apres-cgu-coche.jpg?raw=true) |
| `terrain-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/avant-terrain-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/apres-terrain-0.jpg?raw=true) |
| `parcours-mode` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/avant-parcours-mode.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/apres-parcours-mode.jpg?raw=true) |
| `fin-menu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/avant-fin-menu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/apres-fin-menu.jpg?raw=true) |
| `hud-arriere-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/avant-hud-arriere-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-549/qa/pr/WFS-549/apres-hud-arriere-nuit.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01Us6uCvCCKjrpBx7JNmrkCa
