# WFS-41 — Choix du terrain et du moment (jour / nuit)
Épic : E8 (Écrans et menus) · Priorité : 0.8 · Dépend de : WFS-38

## Besoin
Avant de rouler, le joueur choisit la carte (ville, campagne, plage, circuit) et le jour ou la nuit, comme l'étape 2 du web.

## Règles de gestion
- RG-01 : carrousel des 4 cartes de `maps.js` (`MAPS` : nom, description, longueur en km via `mapLengthKm`), index « 01 / 04 »,
  sur-titre « Étape 2 sur 2 », titre « Choisis ton terrain ».
- RG-02 : glisser ou flèches = carte suivante / précédente ; boutons « Jour » / « Nuit » (nuit par défaut, web `TOD`).
- RG-03 : « Rouler » → course sur la carte choisie (la piste se reconstruit : écran de chargement WFS-35 si > 0,3 s) ; « Retour » → menu.
- RG-04 : vignette de carte : dessin simple (ciel + route) aux couleurs de la carte, pas d'image à créer (web `mapArt` en SVG).
  Une carte sans décor natif (campagne, plage avant WFS-14) reste jouable.

## À réutiliser
`js/world/maps.js` → `MAPS`, `envFor` ; `js/ui/screens.js` → `mapLengthKm`, `mapArt`, `mapCard`, `renderMapUI`, `stepMap`, `pickMap` ;
`index.html` `#mapSel` ; `css/style.css` `.maps-*`, `.map-*`, `.tod-btn`.

## Critères d'acceptation
- [ ] `terrain-0` : carrousel, carte ville sélectionnée, Jour / Nuit ; `terrain-1` : circuit sélectionné
- [ ] `terrain-rouler` : course lancée sur le circuit (journal : carte = circuit)

## Hors périmètre
Décors des autres cartes (WFS-14), éclairage de nuit (WFS-09).
