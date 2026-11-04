# WFS-80 — Manette : garage (peintures) et réglages joignables au focus
Épic : E8 (+ accroche E5 dans `src/ui/garage.cpp`) · Priorité : P4 Basse · Dépend de : WFS-22 ✅, WFS-75 (en cours sur `reglages.*`)

## Besoin (qui, quoi, pourquoi)
WFS-22 a branché la manette et le focus des menus (cadre cyan, A valide, B retour), mais les pastilles de peinture du
garage et toutes les lignes de l'écran Réglages ne déclarent pas de zone de focus : à la manette, on ne peut ni choisir une
peinture ni changer un réglage. Ce ticket rend tout le parcours faisable sans toucher l'écran.

## Règles de gestion
- RG-01 : chaque pastille de peinture du garage (16) est une zone de focus (`manette::focus`, comme les boutons
  `choisir`, `retour`, `prev`, `next` de `garage.cpp` ligne 377) ; A sur une pastille = même effet que le toucher.
- RG-02 : chaque choix de chaque ligne de l'écran Réglages (qualité, caméra, son, musique, commandes, vibrations, direction
  … tout ce que WFS-75 aura livré) est une zone de focus ; A = choisir ; B = retour, comme le bouton retour actuel.
- RG-03 : ordre de déplacement du focus = celui déjà calculé par `manette_jeu.cpp` (position à l'écran) ; aucune règle
  nouvelle de navigation.
- RG-04 : au toucher, le mode manette se coupe comme aujourd'hui (WFS-22) ; aucun changement visuel en mode tactile.
- RG-05 (zone) : `src/ui/ecrans/reglages.*` (E8), à commencer **après la fusion de WFS-75** ; accroche ≤ 10 lignes dans
  `src/ui/garage.cpp` (E5) annoncée au Journal ; `src/platform/manette_jeu.*` seulement si la déclaration de focus manque
  d'un paramètre (≤ 10 lignes, annoncé).

## À réutiliser
- Natif : `manette::focus(zone, retour)` et le banc `WFS_BANC=manette` de WFS-22 (ajouter les deux écrans au scénario).
- Jeu web (`wlfaleh/waaps`, `waforspeed/`) : `js/ui/` gère le focus clavier / manette des menus par l'ordre des boutons ;
  **rien à porter** : le natif a déjà son propre calcul de focus, plus simple à étendre.

## Critères d'acceptation (banc PC, `WFS_BANC=manette`)
- [ ] `manette-garage-peinture` : cadre cyan sur une pastille ; après A, la voiture prend cette peinture (journal)
- [ ] `manette-reglages` : cadre cyan sur un choix ; après A, le réglage change (journal) ; B revient au menu
- [ ] captures tactiles du garage et des réglages inchangées

## Hors périmètre
Remappage des boutons, manettes iOS, aide à l'écran des boutons (icônes A / B).
