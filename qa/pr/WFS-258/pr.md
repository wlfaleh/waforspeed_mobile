## Agent : Claude, agent dev conduite

**Ticket** : WFS-258 (P1), **petite correction demandée par le PO** après la remarque du lead dev sur la PR #271 : plus de bascule de l'aide autour de 100 km/h (gaz coupé puis remis, feux stop qui clignotent) — règles : `regles-de-gestion/WFS-258.md` (Suivi). PR seule, rien d'autre.

## Description

- **Demande** (PO, d'après la remarque du lead dev sur #271) : doigt tenu dans un virage où l'aide veut moins de 100 km/h, la limite cédait sous 100 et reprenait au-dessus : gaz coupé puis remis, feux stop qui clignotent.
- **Mesure sur `main`** (aide activée, doigt tenu « Glisser », gaz tenu, un tour) : le frein de l'aide change d'état **en moins de 0,5 s 221 à 1 317 fois par tour** (presque tout entre 85 et 115 km/h, plus la bande de 3 % autour de la vitesse permise).
- **Ce qui change** (`src/game/player.cpp`, aide activée seulement) :
  - **Doigt relâché** (sous 0,35) : comme le point 3 : la limite cédera au doigt sous 100 km/h, pas au-dessus.
  - **Doigt tenu** : la décision prise quand le joueur braque est gardée ; elle ne change qu'au-delà d'un écart : la limite **reprend au-dessus de 110 km/h** et **recède sous 80 km/h**. Les seuils du PO (céder sous 95, reprendre au-dessus de 100) ne suffisaient pas : mesuré, la voiture faisait encore le va-et-vient entre 95 et 100 km/h (76 clignotements par tour à la Corniche) ; **écart élargi, à valider par le PO** (prévenu).
  - **Frein de l'aide** : serré au-dessus de 103 % de la vitesse permise, il **reste serré jusqu'à revenir à la vitesse permise** (avant : relâché dès 103 %, puis resserré).
  - Membre `cedeAuDoigt_` (remis à vrai au `reset()`), `freinAide` remis au `reset()`. Rien d'autre (direction, aide désactivée, ordinateur) ne change.
- **Comment c'est vérifié** :

| Tour, aide activée, doigt tenu | `main` : clignotements du frein de l'aide (< 0,5 s) | **branche** | sorties de route |
|---|---|---|---|
| Grand Prix, Golf / Gallardo | 221 / 390 | **7 / 3** | 0 / 0 |
| Campagne, Golf / Gallardo | 712 / 1064 | **3 / 2** | 0 / 0 |
| Ville, Golf / Gallardo | 267 / 463 | **1 / 3** | 0 / 0 |
| Corniche, Golf / Gallardo | 923 / 1317 | **2 / 1** | 0 / 0 |

  `test_aide_ralentit` exige maintenant **au plus 10 clignotements par tour** (doigt tenu et lâché) et toujours 0 sortie. 72 tests CPU OK.
- **Limites et risques** :
  - Doigt tenu depuis moins de 80 km/h, la limite reste levée jusqu'à 110 km/h : 10 km/h de plus qu'au point 3 avant de reprendre (0 sortie mesurée sur les 4 cartes).
  - Les images ne montrent presque pas de différence (la correction porte sur le va-et-vient dans le temps, pas sur la trajectoire) : la preuve est le tableau ci-dessus ; les captures confirment que rien d'autre n'a changé.
  - À confirmer par l'utilisateur sur son téléphone (🟡).

## Captures pour le lead dev (16 images : 8 de `main`, 8 de la branche)

`tools/qa/captures_pr.sh` avec `WFS_VIRAGES=vif`, `FILTRE='virages-rapide-tenu-*'` : **vraies images « avant » de `main`** (le banc `virages` y est). Virage rapide du Grand Prix (rayon 60 m), Golf, aide activée, doigt tenu, gaz tenu ; mêmes vitesses à ±1 km/h, même trajectoire.

| Avant (main) | Après (branche) |
|---|---|
| ![avant-00](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/avant-virages-rapide-tenu-apres-00.jpg?raw=true) | ![apres-00](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-rapide-tenu-apres-00.jpg?raw=true) |
| ![avant-02](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/avant-virages-rapide-tenu-apres-02.jpg?raw=true) | ![apres-02](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-rapide-tenu-apres-02.jpg?raw=true) |
| ![avant-03](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/avant-virages-rapide-tenu-apres-03.jpg?raw=true) | ![apres-03](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-rapide-tenu-apres-03.jpg?raw=true) |
| ![avant-04](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/avant-virages-rapide-tenu-apres-04.jpg?raw=true) | ![apres-04](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-rapide-tenu-apres-04.jpg?raw=true) |
| ![avant-05](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/avant-virages-rapide-tenu-apres-05.jpg?raw=true) | ![apres-05](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-rapide-tenu-apres-05.jpg?raw=true) |
| ![avant-07](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/avant-virages-rapide-tenu-apres-07.jpg?raw=true) | ![apres-07](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-258/qa/pr/WFS-258/apres-virages-rapide-tenu-apres-07.jpg?raw=true) |
