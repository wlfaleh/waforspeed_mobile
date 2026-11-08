Agent : Claude, agent 1 (01UY1Vv2)

**Ticket** : WFS-322 — règles : `regles-de-gestion/WFS-322.md`

## Description

- **Demande** (ANO-174, mesure de WFS-300) : en Ville de jour, à l'arrivée d'une sonde d'environnement, les façades grises passent d'un coup au bleuté (saut ΔE2000 5,9 sur une image) et restent bleues (b* jusqu'à −8,4). Une façade grise sous un ciel de jour ne doit être ni bleue ni changer de couleur d'un coup.
- **Causes (RG-01)**, trouvées au banc avec un diagnostic chiffré des sondes (`WFS_DIAG_SONDE`, journal du 2026-11-08) :
  1. **Le bleu** : la sonde est posée 1,5 m au-dessus de la voiture du joueur, et la capture dessinait cette voiture. Sa peinture (Golf bleue du banc) remplissait la moitié basse du cube : le « sol » vu par la sonde valait x 0,234 / y 0,213 (violet, ≈ 4 400 lx) au lieu de la chaussée au soleil. Ce reflet coloré éclairait toutes les façades de la rue ; une voiture rouge aurait teint la rue en rouge.
  2. **Le saut** : `installer()` remplaçait la sonde A sans fondu, et la 1re capture remplaçait la sonde du ciel d'un coup.
  3. **Le reste du saut** : après un saut de plus de 80 m dans la même scène, l'exposition se recalait d'un coup.
- **Ce qui change** (zone E13 + accroches annoncées) :
  - `src/render/environnement.cpp` : la capture d'environnement saute la voiture du joueur (opaques et vitres). Accroche `renderer.*` : `Pending::joueur`, rempli par `dessinJoueur`, 2 lignes. `passes.h` : `AccesRendu::joueur`.
  - `src/render/eclairage_unites.h` : `lisserSh9`, le diffus (harmoniques sphériques) rapproché de la cible avec une constante de 0,6 s. Au début et après une remise à zéro, la cible est prise telle quelle.
  - `src/render/eclairage_physique.cpp` : diffus lissé. Exposition et diffus recalés d'un coup seulement à la 1re capture d'une **nouvelle** scène (carte, moment, voiture : sous le chargement et le décompte). Diagnostic de banc `WFS_DIAG_SONDE` (part du ciel et des surfaces, teinte), sans effet en jeu.
  - Rien de retiré : sondes, rebonds, occlusion de WFS-269 gardés (RG-04). La capture dessine moins d'objets, et le lissage est un calcul de 27 nombres par image : le coût par image ne monte pas.
- **Comment c'est vérifié** :
  - **80 tests CPU verts**, dont `test_fidelite_lumiere` (lissage : pas plus de 1 % par image, cible atteinte, images au même temps) ; parcours complet 0 écart (vue Arrière et Cockpit), branche synchronisée avec `main` ;
  - **banc `qualite`** (Ville de jour, Haute) :

    | Mesure | Avant (`main`) | Après | Règle |
    |---|---|---|---|
    | Pire saut à l'arrivée d'une sonde (10 images, RG-05 de WFS-300) | **5,87** ΔE2000 | **0,41** | ≤ 2 (RG-02) |
    | Façade, `voiture-lod-passage-56` (zone RG-05) | 17,0 / 2,6 / −6,3 | 19,4 / 0,2 / **2,9** | b* ≥ −4 (RG-03) |
    | Façade, `voiture-lod-passage-60` | 17,2 / 2,5 / −6,2 | 19,3 / 0,1 / **1,8** | 56 et 60 pareilles (ΔE ≤ 2) |

  - **Sonde au-dessus de la voiture** (diagnostic) : sol x 0,234 / y 0,213 → x 0,33 / y 0,34 ; éclairement d'une façade x 0,25 / y 0,24 (violet) → x 0,32 / y 0,33 (blanc de jour).
  - Chaussée, ciel et voitures inchangés à l'œil sur les 33 paires.
- **Limites et risques connus** :
  - L'expert réalisme 3D attendait une façade à l'ombre légèrement froide (b* −2 à −3,5). Elle est maintenant légèrement chaude (b* +1,8 à +2,9). C'est dans la règle (b* ≥ −4, gris), mais la teinte fine du ciel et l'occlusion de WFS-269 comptée deux fois sur les captures pourraient faire l'objet d'un ticket de finition. L'avis est au journal.
  - Une recapture après un saut dans la même scène ne recale plus l'exposition d'un coup : elle s'adapte en quelques dixièmes de seconde, comme l'œil.
  - Le conducteur et l'ombre de contact de la voiture du joueur restent dans la capture (dessinés par `Game`, hors zone) : effet petit, à 1,5 m sous la sonde.

## Captures pour le lead dev (33 après)

Dossier : [`qa/pr/WFS-322`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `voiture-basse-poursuite` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-basse-poursuite.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-basse-poursuite.jpg?raw=true) |
| `voiture-course-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-course-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-course-basse.jpg?raw=true) |
| `voiture-course-haute` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-course-haute.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-course-haute.jpg?raw=true) |
| `voiture-course-moyenne` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-course-moyenne.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-course-moyenne.jpg?raw=true) |
| `voiture-course-proche` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-course-proche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-course-proche.jpg?raw=true) |
| `voiture-course-trafic` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-course-trafic.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-course-trafic.jpg?raw=true) |
| `voiture-distance-10-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-distance-10-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-distance-10-basse.jpg?raw=true) |
| `voiture-distance-10` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-distance-10.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-distance-10.jpg?raw=true) |
| `voiture-distance-120-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-distance-120-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-distance-120-basse.jpg?raw=true) |
| `voiture-distance-120` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-distance-120.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-distance-120.jpg?raw=true) |
| `voiture-distance-180-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-distance-180-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-distance-180-basse.jpg?raw=true) |
| `voiture-distance-180` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-distance-180.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-distance-180.jpg?raw=true) |
| `voiture-distance-25-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-distance-25-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-distance-25-basse.jpg?raw=true) |
| `voiture-distance-25` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-distance-25.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-distance-25.jpg?raw=true) |
| `voiture-distance-50-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-distance-50-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-distance-50-basse.jpg?raw=true) |
| `voiture-distance-50` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-distance-50.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-distance-50.jpg?raw=true) |
| `voiture-distance-60-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-distance-60-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-distance-60-basse.jpg?raw=true) |
| `voiture-distance-60` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-distance-60.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-distance-60.jpg?raw=true) |
| `voiture-distance-80-basse` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-distance-80-basse.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-distance-80-basse.jpg?raw=true) |
| `voiture-distance-80` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-distance-80.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-distance-80.jpg?raw=true) |
| `voiture-lod-passage-56` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-lod-passage-56.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-lod-passage-56.jpg?raw=true) |
| `voiture-lod-passage-60` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-lod-passage-60.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-lod-passage-60.jpg?raw=true) |
| `voiture-lod-passage-64` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-lod-passage-64.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-lod-passage-64.jpg?raw=true) |
| `voiture-sonde-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-sonde-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-sonde-0.jpg?raw=true) |
| `voiture-sonde-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-sonde-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-sonde-1.jpg?raw=true) |
| `voiture-sonde-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-sonde-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-sonde-2.jpg?raw=true) |
| `voiture-sonde-3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-sonde-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-sonde-3.jpg?raw=true) |
| `voiture-sonde-4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-sonde-4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-sonde-4.jpg?raw=true) |
| `voiture-sonde-5` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-sonde-5.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-sonde-5.jpg?raw=true) |
| `voiture-sonde-6` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-sonde-6.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-sonde-6.jpg?raw=true) |
| `voiture-sonde-7` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-sonde-7.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-sonde-7.jpg?raw=true) |
| `voiture-sonde-8` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-sonde-8.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-sonde-8.jpg?raw=true) |
| `voiture-sonde-9` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/avant-voiture-sonde-9.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-322/qa/pr/WFS-322/apres-voiture-sonde-9.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv2MVfXZF3cPf1r8qRq
