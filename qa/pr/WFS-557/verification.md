# WFS-557 — Vérification des écrans R19 fusionnés (WFS-548, 549, 552) contre les maquettes

Agent : Claude, agent dev3d (01UY1Vv4Fd). Maquettes : `references/maquettes-R19/`. Arbitrages du PO : `regles-de-gestion/WFS-557.md`
(RG-06 à RG-22 : maquette à 100 %, boutons compris ; Inter en 2e police ; éléments sans place ; scènes 3D et images des modes hors périmètre).

Méthode (RG-01) : chaque écran capturé au banc PC (lavapipe, même rendu que la CI) en **1600 × 720** (même format 20:9 que les
maquettes, 2/3 de 2400 × 1080), puis mis côte à côte avec la maquette : maquette en haut, jeu en bas (`cote-a-cote/`). Colonne
« Avant » : écart trouvé sur `main` ; colonne « Après » : état de la branche.

| Maquette | Écran du jeu | Fichier : fonction | Avant (écarts sur `main`) | Après (branche) |
|---|---|---|---|---|
| 01 menu | menu principal | `src/ui/ecrans/menu.cpp` | colonne Market / Profil / Crédits, sous-titres, « Ta voiture », aide, choix en pilules | **menu : WFS-550** (arbitrage du PO, RG-16 transféré). Le bouton rond « Réglages » du menu est aussi à WFS-550 (`reglages.cpp : dessinerEngrenage` sur sa branche). Reste ici : le cadrage 3D de WFS-552 (`menu_parking_cadrage.cpp : cadrage`, voiture ≈ 21 % plus grande, plus basse et plus à gauche, comme la maquette) |
| 02 choix du mode | « Choisis ton mode » | `src/game/modes_ecran.cpp : ChoixMode::drawUi` | titre centré, cartes plates sans image, rangées Distance / Difficulté sous les cartes, « Continuer » | conforme : titre et « Étape 1 sur 3 » en haut à gauche, 3 cartes inclinées, nom du mode choisi en orange, « Retour / Menu principal » autocollant, « Suivant » rond ; Distance / Difficulté dans la carte « Course à 5 » seulement (RG-21). **Images des modes : provisoire (scène floutée), WFS-562** (RG-22) |
| 03 choix de la carte | terrain | `src/ui/ecrans/terrain.cpp` | — | hors périmètre : fichier de WFS-551 (en cours, contrôlé à sa PR) |
| 04 duel en ligne | accueil du duel | `src/ui/ecrans/salon.cpp : dessinerAccueil` | boutons en biseau | « Retour / Menu principal » autocollant et « Créer / Duel » rond, aux places de la maquette (RG-20). **Scène 3D : hors périmètre, WFS-558** |
| 05 cadeau du jour | cadeau | `src/ui/ecrans/cadeau_jour.cpp` | — | hors périmètre : WFS-550 |
| 06, 07, 13, 15, 16 garage | garage | `src/ui/garage_*.cpp` | — | hors périmètre : WFS-550 / 551 |
| 08 chargement | écran de chargement | `src/ui/ecrans/chargement.cpp : dessinerChargement` | logo droit sur 2 lignes et trop grand, barre plate, pas d'étapes, pourcentage dans le libellé, texte en Rajdhani | conforme : logo italique, fenêtre 687 × 197, étape en Inter, gros pourcentage, barre arrondie au dégradé de la marque, 4 étapes (Moteur, Modèles 3D, Voitures, Ville) avec coches, 2 lignes Inter grises |
| 08b chargement, erreur | écran d'erreur | `chargement.cpp : dessinerChargement` (erreur) | message sous la barre, « Réessayer » en biseau orange | conforme : message rouge dans la fenêtre, pourcentage rouge, barre et étape en cours vers le rouge, « Réessayer » rond en bas à droite |
| 09 départ | décompte | — | — | **scène 3D : hors périmètre, WFS-559** (aucun élément 2D touché par 548 / 549) |
| 10 fin de course | fin Checkpoint | `src/game/course_system.cpp : drawOver` | « Rejouer » / « Menu » dans le panneau | « MENU » en plaque à bande orange en bas à gauche, « Rejouer » rond en bas à droite (RG-20). **Scène 3D (portique, score 3D) : hors périmètre, WFS-560** |
| 11 résultats | panneau du score | `src/game/score_fin.cpp` | — | panneau « Score » inchangé (déjà au modèle) ; **scène 3D (tableau, parking) : hors périmètre, WFS-561** |
| 12 Market | Market | `src/ui/ecrans/market_ecran.cpp : dessiner, carte, carteDebutant` | grande fenêtre, 4 médailles, « Retour », pas d'onglets ni de pack débutant affiché | conforme : « Boutique / MARKET », onglets Étoiles / Offres / Sans pub (filtres, RG-18), solde en pilule, ✕ (= Retour), carte Pack débutant (proposé / acheté / « Après ta 3e course »), 4 cartes (Or et Coffre bordées d'orange, « Populaire », « Meilleure valeur »), ligne Sans pub, liens du bas. Texte « Sans pub » : celui du jeu gardé (il décrit le vrai comportement) |
| 14 Réglages, Affichage | Réglages | `src/ui/ecrans/reglages.cpp : dessinerPanneau, dessinerOnglets, boutonCgu` | fenêtre centrée en 1 colonne, onglets pleine largeur sans icônes, Crédits / CGU en haut, ligne technique cyan, « Fermer » en bas | conforme : page pleine, onglets avec icônes au milieu du haut, « ✕ Fermer » en haut à droite, 2 colonnes, ligne technique Inter grise en bas à gauche, « ☆ Crédits » et « 🗎 Conditions d'utilisation » en bas à droite |
| 24 Réglages, Conduite | Réglages | même | même | conforme (icônes des onglets gardées comme sur la maquette 14) |
| 25 Réglages, Son | Réglages | même | même | conforme |
| 17, 17b HUD | HUD | `src/ui/hud.cpp` | — | hors périmètre : WFS-551 |
| 18, 18a à 18d cartes | terrain, paquets | `terrain.cpp`, `paquets*.cpp` | — | hors périmètre : WFS-551 |
| 19 pause | pause | `src/ui/ecrans/pause.cpp : dessinerPause, dessinerStats` ; `reglages.cpp : dessinerBouton` | 3 boutons en rangée, pas de stats, sur-titre sans mode ni carte, « Réglages » cyan en biseau | conforme : fenêtre 830 × 390, « Course en pause · Checkpoint · Ville néon », ligne de stats (temps, CP n sans total en Checkpoint : RG-17, points), « Reprendre » rond, « ↺ Recommencer » et « ⇥ Quitter la course » (bord rouge) en verre, « ⚙ Réglages » en verre |
| 20 création du profil | « Crée ton pilote » | `src/ui/ecrans/creation_profil.cpp : dessiner, champ, dessinerApercuPlaque` | champ « Plaque » à part, clavier 10 colonnes, « C'est parti » en biseau, champ cyan | conforme : « Première course », champ Pseudo bordé d'orange, « Ta plaque » (aperçu orange, touchable pour la modifier : RG-14), aide, « C'est parti » rond, clavier de verre 3 × 13 + ⌫ + Espace |
| 21 profil | Profil | `src/ui/ecrans/profil.cpp : dessiner` | grande fenêtre, plaque à droite, records en liste, « Modifier » orange et « Retour » en bas | conforme : en-tête et plaque à côté du nom, « ✎ Modifier » en verre en haut à droite, 6 tuiles, panneau Records, panneau Dernières courses en cartes (« · record » : RG-15), autocollant « Retour / Menu principal » |
| 22 CGU | conditions d'utilisation | `src/ui/ecrans/cgu.cpp : Cgu::dessiner, mettreEnPage, toucher, refuser` | case à cocher, « Refuser », textes à la suite, Rajdhani | conforme (RG-19) : « Avant de rouler », onglets Conditions / Confidentialité, texte en Inter (intertitres orange clair), « J'accepte » rond grisé jusqu'à la lecture des 2 onglets, phrase d'aide ; refus par la touche Retour (`android_main.cpp`, 1 ligne) ; lecture : « Fermer » rond |
| 23 salon du duel | salon | `src/ui/ecrans/salon.cpp : dessinerSalonR19` | titre « Duel », « Prêt » / « Quitter » en bas à gauche, lien d'invitation affiché, 2 mises en page | conforme : « Salon du duel », code en grand, « Inviter » orange avec icône, voiture et terrain à droite, distance en verre, 4 rangées de pilotes (pastilles Prêt / En attente, places libres), note, « Quitter / Le salon » autocollant, « Prêt » rond. Le lien d'invitation n'est plus écrit (il part par « Inviter ») |
| 26 voiture prêtée, 29 gain d'étoiles, 30 messages en course | — | — | — | hors périmètre : WFS-550 |
| 27, 28 achat | fenêtre d'achat | `src/ui/garage_achat*.cpp` | — | hors périmètre : fichier de WFS-551 (en cours) |
| 31 fin Course à 5 | fin Course à 5 | `src/game/course5_system.cpp : resultat` ; `course_system.cpp : panneauFinR19, enTeteFinR19, ligneFinR19, boutonsFinR19` | fenêtre étroite, « Rejouer » / « Menu » dans le panneau | conforme : fenêtre 640 × 556, en-tête, 5 tuiles (ta ligne en orange), « Retour / Menu principal » autocollant et « Rejouer » rond sous le score |
| 32 fin Poursuite | fin Poursuite | `src/game/poursuite_system.cpp : resultat` | même | conforme : « VICTOIRE », « Échappé · 0:10,2 », 3 tuiles (police semée en orange) |
| 33 fin Duel | fin Duel | `src/game/duel_system.cpp : resultat` | même, « 1re place » | conforme : « VICTOIRE » au 1er, lignes en tuiles, « Revanche » rond |

## Communs (toutes les maquettes)
- **Police Inter** (RG-10 à RG-13) : Regular et Medium dans le même atlas que Rajdhani (`src/ui/text.cpp : load`, `ui::AvecPolice`) ;
  sur-titres en Inter Medium (`verre.cpp : surTitre`). Temps avec virgule en français (`ui::chrono`, maquettes 10, 19, 21, 31).
- **Boutons** (`src/ui/boutons.cpp`) : icônes au trait (`icone`), bouton de verre à icône (`verreIcone`), plaque (`plaque`), bague
  chrome plus sombre et liseré plus fin (maquettes 01, 08b, 19).

## Limites signalées
- Maquette 20 : touches de 62 × 52 px (≈ 4,7 mm sur le téléphone) au lieu des 9 mm de WFS-481 ; zones de toucher jointives ≥ 44 px (RG-07).
- Écrans 16:9 (1280 × 720) : création du profil, Market, choix du mode et salon sont réduits pour tenir en largeur et centrés.
- Interface : limite de 8 192 quadrilatères par image ; un dépassement est maintenant noté au journal (`renderer.cpp`, accroche de 4 lignes).
