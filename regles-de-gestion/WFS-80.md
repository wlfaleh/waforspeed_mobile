# WFS-80 — Manette au focus (garage, réglages) + réglages « Direction » et « Réduire les mouvements »
Épic : E8 (+ accroche E5 dans `src/ui/garage.cpp`) · Priorité : P3 Moyenne · Dépend de : WFS-22 ✅, WFS-75 ✅ (PR #65)

## Besoin (qui, quoi, pourquoi)
WFS-22 a branché la manette et le focus des menus (cadre cyan, A valide, B retour), mais les pastilles de peinture du
garage et les lignes de l'écran Réglages ne déclarent pas de zone de focus : à la manette, on ne peut ni choisir une
peinture ni changer un réglage. Par ailleurs, WFS-70 a livré deux réglages sans ligne à l'écran : le mode de direction
(glisser / flèches) et « réduire les mouvements ». Les deux sujets touchent le même panneau `reglages.cpp` et les mêmes
boutons : un seul ticket (décision PO 2026-11-04, pas de micro-ticket).

## Règles de gestion
- RG-01 : ligne **« Direction »** dans le panneau, 2 choix **« Glisser » / « Flèches »** (steerMode du jeu web). Le choix
  écrit `sauvegarde().steer` (`"glisser"` / `"fleches"`, clé `steer` existante de WFS-24) **et** applique tout de suite
  `reglagesRessenti().direction` (`ModeDirection::Glisser` / `Fleches`, `src/game/ressenti.h`), car `ressenti.cpp` ne lit la
  clé qu'une fois par lancement. Défaut : Glisser.
- RG-02 : ligne **« Réduire les mouvements »**, 2 choix **« Non » / « Oui »** (reduceMotion du jeu web). Le choix applique
  `reglagesRessenti().reduireMouvements` ; il est mémorisé dans le fichier des réglages de `reglages.cpp` (nouvelle clé
  `mouvements=0/1`, même écriture que `gaucher` / `vibrations`) et réappliqué à la lecture des réglages au lancement.
  Défaut : Non. Aucune modification de `src/game/sauvegarde.*` ni de `ressenti.*` (leurs réglages sont déjà publics).
- RG-03 : le tableau `LIGNES` passe de 6 à 8 lignes ; si le panneau ne tient plus en hauteur en paysage, réduire
  l'interligne (pas de défilement à inventer).
- RG-04 : chaque pastille de peinture du garage (16) est une zone de focus (`manette::focus`, comme les boutons
  `choisir`, `retour`, `prev`, `next` de `garage.cpp`) ; A sur une pastille = même effet que le toucher.
- RG-05 : chaque choix de chaque ligne de l'écran Réglages (les 8 lignes) est une zone de focus ; A = choisir ;
  B = retour, comme le bouton retour actuel.
- RG-06 : ordre de déplacement du focus = celui déjà calculé par `manette_jeu.cpp` (position à l'écran) ; aucune règle
  nouvelle de navigation.
- RG-07 : au toucher, le mode manette se coupe comme aujourd'hui (WFS-22) ; aucun changement visuel en mode tactile
  (hors les deux lignes nouvelles).
- RG-08 (zone) : `src/ui/ecrans/reglages.*` (E8). Accroche ≤ 10 lignes dans `src/ui/garage.cpp` (E5), annoncée au
  Journal ; `src/platform/manette_jeu.*` seulement si la déclaration de focus manque d'un paramètre (≤ 10 lignes, annoncé).
  Interdit : `src/game/**` (dont `ressenti.*`, `sauvegarde.*`), `src/world/meteo.*` (WFS-18), `src/audio/**` (WFS-79),
  `ios/**`, `ios_main.mm`, `vk.cpp` (WFS-78), noyau (`src/core`, `src/game/game.*`, CMake).

## À réutiliser
- Natif : `manette::focus(zone, retour)` et le banc `WFS_BANC=manette` de WFS-22 (ajouter les deux écrans au scénario) ;
  `reglagesRessenti()` et `ModeDirection` de WFS-70 ; lecture / écriture du fichier de réglages de WFS-75 dans `reglages.cpp`.
- Jeu web (`wlfaleh/waaps`, `waforspeed/`) : libellés et valeurs de `index.html` (`#pause .settings`, steerMode,
  reduceMotion) ; le focus clavier de `js/ui/` : **rien à porter**, le natif a déjà son calcul de focus.

## Critères d'acceptation (banc PC)
- [ ] capture `reglages` : lignes « Direction » et « Réduire les mouvements » visibles, lisibles, rien qui déborde
- [ ] journal : changer « Direction » donne `reglagesRessenti().direction` = Flèches tout de suite et `steer=fleches`
      sauvegardé ; « Réduire les mouvements » = Oui est relu après relancement
- [ ] `manette-garage-peinture` : cadre cyan sur une pastille ; après A, la voiture prend cette peinture (journal)
- [ ] `manette-reglages` : cadre cyan sur un choix ; après A, le réglage change (journal) ; B revient au menu
- [ ] captures tactiles du garage inchangées

## Hors périmètre
Remappage des boutons, manettes iOS, aide à l'écran des boutons (icônes A / B), défilement du panneau.
