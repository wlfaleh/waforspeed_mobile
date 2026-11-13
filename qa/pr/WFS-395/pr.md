Agent : Claude, agent dev3d (01UY1Vv4vT)

**Ticket** : WFS-395 — règles : `regles-de-gestion/WFS-395.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : la vue Cockpit doit montrer l'habitacle des nouvelles voitures de WFS-359 (Corvette C7, id « mustang », et Vitara Brezza 2022, id « tcross ») et non plus ceux de la Mustang S550 et du T-Cross, à leur vraie largeur, sans marque ni forme nue.
- **Ce qui change** (fichiers, comportement, à l'écran) :
  - `src/render/vehicule_cockpit_defs.cpp` (entrées `mustang` et `tcross` seulement) : Corvette : volant rond de 358 mm (14,1 po), palettes, combiné à compte-tours rond en barres (vitesse et rapport dedans, sans aiguille sur les chiffres), écran de 8 po sous la casquette, tourné de 15° vers le conducteur, assise basse (0,241 m), largeur 1,44 m ; Brezza : volant à méplat de 365 mm, palettes, combiné à 2 cadrans et écran au milieu, écran flottant de 9 po, planche deux tons, assise de SUV (0,575 m), largeur 1,45 m ; coussins des volants de série adaptés (Corvette 200 × 145, Brezza 205 × 150 mm).
  - `src/render/vehicule_cockpit_details.cpp/.h` : pièces propres des deux voitures (mécanisme du lot 1 de WFS-325) : Corvette : console haute, aérateurs verticaux de part et d'autre de l'écran, climatisation laquée à 2 molettes, démarreur, **poignée de maintien du passager tenue par deux pieds alu (haut sur la face de la planche, bas sur le flanc droit de la console : `poigneeMaintien`)**, sélecteur gainé et molette des modes, portières alu et cuir avec tweeter ; Brezza : aérateurs chromés, jonc argent sur toute la planche, climatisation automatique (afficheur, 2 molettes), **levier de boîte automatique modelé (grille P R N D M dans sa garniture chromée et ses 5 repères, jupe plissée, poignée haute gainée penchée vers l'arrière, dessus bombé, touche de déverrouillage, jonc argent)**, porte-gobelets, accoudoir, portières, plafonnier. Toutes les pièces sont modelées : aucune forme nue.
  - `tests/test_cockpit.cpp` : valeurs des deux voitures, lot des pièces propres porté à 6, Corvette exemptée de « GT86 la plus basse hors supercars » (C7 de 1,24 m) ; **nouveau cas : pieds de la poignée collés à la planche et à la console (dos du pied à −2,0 mm, enfoncé, aucun jour ; barre dans chaque pied)**.
  - Rien d'autre : aucune autre voiture ne change (empreintes des volants de main vérifiées), ni les modèles 3D, ni `camera_vue.*`.
- **Comment c'est vérifié** (tests CPU, parcours, bancs, mesures) : tests CPU 115 / 115 (partie ci-dessous) ; banc `cockpit` (Ville jour et nuit sous la pluie, virage, Plage de nuit) : fenêtre de la rue cachée 0,0 % pour les deux voitures ; banc `cockpit-vitrine` : vues centre, console et portière des deux voitures.
- **Limites et risques connus** : cotes de mémoire ou estimées (statut AC / E noté à chaque valeur : pages des constructeurs bloquées par le réseau des agents) ; Brezza : bas de planche brun très sombre, qui teinte aussi portières et plancher (couleur commune de WFS-349) ; pas de lumière d'ambiance pour les deux ; le style « FordDigital » n'est plus utilisé par aucune voiture (gardé). Écran de 8 po de la Corvette : son haut est sous la casquette de la planche (vu de la vitrine), il ne la traverse pas (haut du cadre à y −0,189, dessus de la planche à y ≈ −0,148 à sa profondeur).

## Tests CPU

Commit testé : 63b62b78 ; journal complet : [`qa/pr/WFS-395/tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/tests-cpu.log).

```
[OK] test_version_chargement (0 s)
[OK] test_ville_lieux (0 s)
[OK] test_ville_sol (3 s)
[OK] test_virages_fleches (0 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 115 test(s) réussi(s) en 388 s
```

## Captures pour le lead dev (18 après)

Dossier : [`qa/pr/WFS-395`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `vitrine-mustang-centre` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/avant-vitrine-mustang-centre.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/apres-vitrine-mustang-centre.jpg?raw=true) |
| `vitrine-mustang-console` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/avant-vitrine-mustang-console.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/apres-vitrine-mustang-console.jpg?raw=true) |
| `vitrine-mustang-portiere` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/avant-vitrine-mustang-portiere.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/apres-vitrine-mustang-portiere.jpg?raw=true) |
| `vitrine-tcross-centre` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/avant-vitrine-tcross-centre.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/apres-vitrine-tcross-centre.jpg?raw=true) |
| `vitrine-tcross-console` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/avant-vitrine-tcross-console.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/apres-vitrine-tcross-console.jpg?raw=true) |
| `vitrine-tcross-portiere` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/avant-vitrine-tcross-portiere.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/apres-vitrine-tcross-portiere.jpg?raw=true) |
| `cockpit-mustang-carte` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/avant-cockpit-mustang-carte.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/apres-cockpit-mustang-carte.jpg?raw=true) |
| `cockpit-mustang-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/avant-cockpit-mustang-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/apres-cockpit-mustang-jour.jpg?raw=true) |
| `cockpit-mustang-nuit-virage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/avant-cockpit-mustang-nuit-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/apres-cockpit-mustang-nuit-virage.jpg?raw=true) |
| `cockpit-mustang-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/avant-cockpit-mustang-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/apres-cockpit-mustang-nuit.jpg?raw=true) |
| `cockpit-mustang-plage-nuit-depart` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/avant-cockpit-mustang-plage-nuit-depart.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/apres-cockpit-mustang-plage-nuit-depart.jpg?raw=true) |
| `cockpit-mustang-virage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/avant-cockpit-mustang-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/apres-cockpit-mustang-virage.jpg?raw=true) |
| `cockpit-tcross-carte` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/avant-cockpit-tcross-carte.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/apres-cockpit-tcross-carte.jpg?raw=true) |
| `cockpit-tcross-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/avant-cockpit-tcross-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/apres-cockpit-tcross-jour.jpg?raw=true) |
| `cockpit-tcross-nuit-virage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/avant-cockpit-tcross-nuit-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/apres-cockpit-tcross-nuit-virage.jpg?raw=true) |
| `cockpit-tcross-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/avant-cockpit-tcross-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/apres-cockpit-tcross-nuit.jpg?raw=true) |
| `cockpit-tcross-plage-nuit-depart` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/avant-cockpit-tcross-plage-nuit-depart.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/apres-cockpit-tcross-plage-nuit-depart.jpg?raw=true) |
| `cockpit-tcross-virage` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/avant-cockpit-tcross-virage.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-395/qa/pr/WFS-395/apres-cockpit-tcross-virage.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv4vTNG2pkFZowyb4r7
