# WFS-376 — captures avant / après (tools/qa/captures_pr.sh)

Branche `feature/waforspeed-376` comparée à `origin/main` (67f20e1a, après WFS-379 ; « avant » : main + le seul banc `marina` ajouté, mêmes caméras) ; bancs : marina ; filtre : `*`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `marina-bassin` | ![avant](avant-marina-bassin.jpg) | ![après](apres-marina-bassin.jpg) |
| `marina-cockpit-120-doree` | ![avant](avant-marina-cockpit-120-doree.jpg) | ![après](apres-marina-cockpit-120-doree.jpg) |
| `marina-cockpit-120-midi` | ![avant](avant-marina-cockpit-120-midi.jpg) | ![après](apres-marina-cockpit-120-midi.jpg) |
| `marina-cockpit-80-doree` | ![avant](avant-marina-cockpit-80-doree.jpg) | ![après](apres-marina-cockpit-80-doree.jpg) |
| `marina-cockpit-80-midi` | ![avant](avant-marina-cockpit-80-midi.jpg) | ![après](apres-marina-cockpit-80-midi.jpg) |
| `marina-gros-plan-doree` | ![avant](avant-marina-gros-plan-doree.jpg) | ![après](apres-marina-gros-plan-doree.jpg) |
| `marina-gros-plan-midi` | ![avant](avant-marina-gros-plan-midi.jpg) | ![après](apres-marina-gros-plan-midi.jpg) |
| `marina-gros-plan-nuit` | ![avant](avant-marina-gros-plan-nuit.jpg) | ![après](apres-marina-gros-plan-nuit.jpg) |
| `marina-loin-detail` | ![avant](avant-marina-loin-detail.jpg) | ![après](apres-marina-loin-detail.jpg) |
| `marina-loin` | ![avant](avant-marina-loin.jpg) | ![après](apres-marina-loin.jpg) |
| `marina-poursuite-120-doree` | ![avant](avant-marina-poursuite-120-doree.jpg) | ![après](apres-marina-poursuite-120-doree.jpg) |
| `marina-poursuite-120-midi-basse` | ![avant](avant-marina-poursuite-120-midi-basse.jpg) | ![après](apres-marina-poursuite-120-midi-basse.jpg) |
| `marina-poursuite-120-midi` | ![avant](avant-marina-poursuite-120-midi.jpg) | ![après](apres-marina-poursuite-120-midi.jpg) |
| `marina-poursuite-80-doree` | ![avant](avant-marina-poursuite-80-doree.jpg) | ![après](apres-marina-poursuite-80-doree.jpg) |
| `marina-poursuite-80-midi` | ![avant](avant-marina-poursuite-80-midi.jpg) | ![après](apres-marina-poursuite-80-midi.jpg) |
| `marina-poursuite-80-nuit` | ![avant](avant-marina-poursuite-80-nuit.jpg) | ![après](apres-marina-poursuite-80-nuit.jpg) |
| `marina-suite-1` | ![avant](avant-marina-suite-1.jpg) | ![après](apres-marina-suite-1.jpg) |
| `marina-suite-2` | ![avant](avant-marina-suite-2.jpg) | ![après](apres-marina-suite-2.jpg) |
| `marina-suite-3` | ![avant](avant-marina-suite-3.jpg) | ![après](apres-marina-suite-3.jpg) |
| `marina-suite-4` | ![avant](avant-marina-suite-4.jpg) | ![après](apres-marina-suite-4.jpg) |
| `marina-suite-5` | ![avant](avant-marina-suite-5.jpg) | ![après](apres-marina-suite-5.jpg) |
| `marina-suite-6` | ![avant](avant-marina-suite-6.jpg) | ![après](apres-marina-suite-6.jpg) |

## Suite en roulant (retour du lead dev, point 1)
`marina-suite-1` à `-6` : vue Poursuite, 120 km/h, de 45 à 93 m dans le port, une image toutes les 0,5 s, la rangée du quai à l'écran.
Des blocs du décor (dont ceux du port, devant) passent au niveau détaillé (seuil de 200 m) entre `suite-1` et `suite-2`, `suite-2` et `-3`,
`suite-4` et `-5`, `suite-5` et `-6` (saut du nombre de triangles du décor > 2 000, journal du banc) : aucun bateau n'apparaît, ne disparaît
ni ne change de silhouette d'une image à l'autre.

## Nuit (retour du lead dev, point 2)
`marina-gros-plan-nuit` et `marina-poursuite-80-nuit` : coques, rouf et mâts lisibles sous la lune et les lampadaires du quai.
