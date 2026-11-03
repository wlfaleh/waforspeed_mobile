# WFS-63 — Point d'extension : maillages animés par squelette (skinning)
Épic : E0 · Priorité : P3 Moyenne · Dépend de : — · Débloque : WFS-19 (piétons et conducteurs)

## Besoin (qui, quoi, pourquoi)
Les personnages du jeu web sont des maillages à squelette avec des animations. Le renderer natif ne sait dessiner que des
maillages rigides : sans ce point d'extension, l'épic E2 ne peut pas porter les piétons (WFS-19) sans toucher au noyau.

## Règles de gestion
- RG-01 : API du renderer (`src/render/renderer.h`) : maillage à squelette (positions, normales, UV, 4 indices d'os + 4 poids par sommet) et dessin d'une instance avec sa palette de matrices d'os (≤ 64 os) ; les maillages rigides existants ne changent pas.
- RG-02 : skinning dans le vertex shader (variante de `scene.vert` / `shadow.vert`) : mêmes matériaux PBR, ombres et brouillard que les voitures.
- RG-03 : outil de conversion (même principe que `tools/convert_car.mjs`) des fichiers `models/people/homme.json`, `femme.json`, `animations.json` du jeu web vers un format binaire natif ; **aucun nouveau personnage créé**, licence `license.txt` copiée.
- RG-04 : lecture d'animation sur le CPU (interpolation linéaire des positions, sphérique des rotations, boucle), sans allocation par image, appelable depuis un thread de travail.
- RG-05 : ≤ 8 instances complètes par image visées (RG-03 de WFS-19) ; coût mesuré au journal du banc.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Modèles : `models/people/homme.json`, `femme.json`, `animations.json`, `license.txt`.
- Code : `js/world/people.js` (`pedClone`, `updatePeopleReal` : mixeur d'animation et noms des clips `Walk_Loop`, `Idle_Loop`…) pour l'ordre des os et les clips.
- Outil : `tools/convert_car.mjs` du dépôt natif comme modèle de conversion.

## Critères d'acceptation (captures du banc PC)
- [ ] `squelette-essai` : `homme` debout (`Idle_Loop`), proportions justes, éclairé comme les voitures
- [ ] `squelette-marche-0` / `squelette-marche-1` (instants différents) : poses de `Walk_Loop` différentes
- [ ] captures existantes identiques à `main`, banc `0 écart(s)` ; doc du point d'extension dans le skill `waforspeed-natif`

## Hors périmètre
Placement des piétons dans les cartes, apparence (WFS-19), conducteurs, mélange de plusieurs animations.
