# WFS-96 — Menus : Showroom et « Ta voiture » ouvrent le garage, quartiers sur la carte du terrain, focus manette, voile sans bandes, sons d'interface
Épic : E8 · Priorité : P2 Haute · Dépend de : — (WFS-11, WFS-41, WFS-80 ✅) · Taille : 1 à 2 jours

## Besoin
Sur le téléphone, le 3e choix du menu « Showroom » affiche encore « Bientôt » (`src/ui/ecrans/menu.cpp`, RG-04 de
WFS-37) alors que le garage existe (WFS-11) : dans le jeu web, « Showroom » et « Ta voiture » ouvrent le garage pour
regarder et changer de voiture (`openGarage('look')`). Autres écarts visibles des menus : la carte du terrain n'affiche pas
le nombre de quartiers ni leurs noms (`mapCard` du web) ; le voile sombre du menu est fait de 16 bandes qui se voient
(rayures verticales sur `manette-menu-focus`) ; à la manette, le focus ne démarre pas sur le 1er choix quand un panneau
s'ouvre sur le même écran (Réglages : l'indice du focus est gardé, `manette_jeu.cpp` ne le remet à 0 qu'au changement
d'écran) ; les petits sons d'interface du web (changement de voiture, de carte) manquent.

## Règles de gestion
- RG-01 : « Showroom » (menu) et l'appui sur « Ta voiture » (pied du menu) ouvrent le garage en mode **regarder** : le
  bouton principal s'appelle « Choisir » et ramène au **menu** (pas au terrain) en gardant la voiture et la peinture ;
  « Retour » ramène au menu. « Course solo » garde son parcours garage → terrain → course. Plus de « Bientôt ».
- RG-02 : carte du terrain : sous « Longueur », un fait « Quartiers » (nombre) et la liste des noms en pastilles
  (`zoneNames` du web : noms distincts de `ZONES[ground]`, ex. Ville : 6 noms), dans la carte, rien qui déborde à 1280 ni
  à 2400 px.
- RG-03 : voile du menu : dégradé continu (bandes ≥ 64 ou dégradé par sommets si l'API d'interface le permet), plus aucune
  rayure visible ; mêmes opacités (.94 → .78 à 42 % → .12).
- RG-04 : manette : quand l'ensemble des zones de focus change sur le même écran (ouverture / fermeture d'un panneau,
  changement du nombre de zones ou de la 1re zone), le focus va à la 1re zone déclarée, qui est le 1er choix de l'écran
  (« Course solo », 1er choix de la 1re ligne des réglages, « Choisir » au garage, « Rouler » au terrain).
- RG-05 : sons d'interface du web : changement de voiture au garage `noiseHit(0.18, 1400, 1.5, 0.25, 500)`, changement de
  carte au terrain `noiseHit(0.12, 1600, 1.4, 0.2, 500)` ; journal « SFX … » au banc.

## À réutiliser du jeu web
`js/ui/screens.js` : `openGarage`, `leaveGarage` (l. 160-173, `garageFor = 'look'`), `#garageBtn` / `#myCarBtn` (l. 145, 447),
`mapCard` et `zoneNames` (l. 198, 320-336), `selectCar` (l. 158) et `stepMap` (l. 384) pour les sons ; `index.html`
`#garageGo` « Choisir ». Natif : `src/ui/garage.cpp` (`choisi_`), `src/ui/ecrans/terrain.cpp`, `src/world/zones.h`
(lecture seule), `src/audio/sfx.*` (`noiseHit` déjà porté), `src/platform/manette_jeu.cpp` (`g_focus`).

## Critères d'acceptation
- [ ] nouvelles captures `menu-showroom` (garage ouvert depuis Showroom) et `showroom-choisir` (retour au menu, nouvelle
  voiture sous « Ta voiture ») ; `menu-bientot` supprimée ou remplacée ; parcours Course solo inchangé (`menu-depart-*`)
- [ ] `terrain-0` / `terrain-1` : « Quartiers 6 » et les noms des quartiers sur la carte
- [ ] `menu-0`, `manette-menu-focus` : voile sans rayures ; `manette-reglages` : focus sur le 1er choix à l'ouverture
- [ ] journal du banc : « SFX garage » au changement de voiture, « SFX carte » au changement de carte ; tests CPU verts

## Hors périmètre
Lobby du duel et garage du duel (WFS-89), nouveaux écrans, textes du garage autres que le bouton principal.

## Zone de code
`src/ui/ecrans/**` sauf `salon.*` (WFS-89). Accroches ≤ 10 lignes chacune, annoncées au Journal : `src/ui/garage.cpp`
(mode regarder), `src/platform/manette_jeu.cpp` (RG-04 ; fichier non touché par WFS-91), `src/audio/sfx.*` (2 sons).
Interdit : `src/platform/desktop_main.cpp` et `.github/**` (WFS-91 : captures dans les bancs déjà branchés de `menu.cpp`,
`terrain.cpp`, `reglages.cpp`), `src/render/**` (WFS-97), `src/ui/text.*` et `toasts.*` (WFS-95).

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| P2 Haute | WFS-96 | Menus : Showroom et « Ta voiture » ouvrent le garage, quartiers sur la carte du terrain, focus manette, voile sans bandes, sons d'interface | E8 | ✅ Fusionné (PR #79, revue WFS-103 ; ANO-15 et ANO-34 corrigées ; QA de nuit à faire) | Claude (session trafic), 2026-11-04 | — | `feature/waforspeed-96` |  |  |

Critères et notes de suivi :

**WFS-96 — Menus : Showroom, quartiers du terrain, focus manette, voile, sons** · E8 · P2 Haute · ✅ Fusionné (PR #79, 6863763, revue WFS-103) — Claude (session trafic), 2026-11-04, branche `feature/waforspeed-96` (règles : `regles-de-gestion/WFS-96.md`). Zone : `src/ui/ecrans/**` sauf `salon.*` ; accroches ≤ 10 lignes annoncées : `src/ui/garage.cpp`, `src/platform/manette_jeu.cpp`, `src/audio/sfx.*` ; interdit : `desktop_main.cpp`, `.github/**`, `src/render/**`, `src/ui/text.*`, `toasts.*`
- [ ] `menu-showroom`, `showroom-choisir` : Showroom ouvre le garage, « Choisir » ramène au menu ; `terrain-0` : « Quartiers » et noms ; voile du menu sans rayures ; focus manette sur le 1er choix ; « SFX garage » / « SFX carte » au journal
