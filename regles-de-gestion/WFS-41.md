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


## Périmètre élargi (tickets plus gros, moins de builds)
Ce ticket absorbe WFS-68 : **lire et appliquer aussi** `regles-de-gestion/WFS-68.md` (écran de réglages : caméra, qualité, son, commandes, vibrations). Une seule branche, une seule CI à la fin.

## Compléments (écarts avec le jeu web)
- Écran de fin : « NOUVEAU RECORD » quand le score bat le meilleur (`newBest`) et statistiques distance / vitesse max / quasi-contacts / checkpoints (`stDist`, `stSpeed`, `stNear`, `stCp`) ; la sauvegarde du record est dans WFS-24.
- Pause : bouton « Recommencer » (`restartBtn`) en plus de « Reprendre » et « Quitter » (voir WFS-39).
- Réglages : le choix du mode de direction vient de WFS-70.

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| P2 Haute | WFS-41 | Menus complets : choix du terrain et du moment, réglages (qualité, caméra, son, commandes) | E8 | ✅ Fusionné (par confiance, QA de nuit à faire) | Claude (agent 5, session 011cpqZaDiS2u), 2026-11-03 | WFS-38 ✅ | `feature/waforspeed-41` | [#45](https://github.com/wlfaleh/waforspeed_mobile/pull/45) |  |

Critères et notes de suivi :

**WFS-41 — Menus complets : terrain, moment, réglages** · E8 · priorité P2 Haute · ✅ Fusionné le 2026-11-04 (PR #45, par confiance ; QA de nuit : `qa/Check.md`) — Claude (agent 5), 2026-11-03, branche `feature/waforspeed-41` (règles : `regles-de-gestion/WFS-41.md`)
- [ ] `terrain-0` / `terrain-1` (carrousel, Jour / Nuit), `terrain-rouler` (course sur le circuit)
