Agent : Claude, agent 2 (01UY1Vv5)

**Ticket** : WFS-290 — règles : `regles-de-gestion/WFS-290.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : « le bouton Réglages et le bouton Fermer des réglages, parfois ça ne passe pas, que ce soit dans le jeu ou dans le menu » (2026-11-08, ANO-159).
- **Ce qui change** :
  - `src/ui/ecrans/reglages.cpp` (zone E8) :
    - **cible d'appui de 48 dp** de haut au moins, et 4 dp de plus de chaque côté, pour « Réglages » et « Fermer » (règle Android, RG-03) ; **le dessin ne change pas, aucun bouton ne bouge** (sur téléphone : « Réglages » dessiné 82 × 25 dp, « Fermer » 88 × 29 dp) ;
    - **anti-rebond** : un appui qui ouvre ou ferme moins de 0,3 s après le précédent est ignoré (sur téléphone, « Fermer » est presque à la place de « Réglages » : un 2e appui rapide refermait ou rouvrait aussitôt) ;
    - la zone du bouton **n'est plus effacée au changement d'écran** (un appui avant la 1re image dessinée comptait pour rien) ;
    - **RG-04, retour visible à l'appui** (1er refus) : comme les choix du menu (`presse_`), le bouton **s'éclaircit tant que le doigt est posé** (« Réglages » : voile cyan ; « Fermer » : orange éclairci #FF9A4D), l'action se fait **au relâcher** dans la cible, un doigt qui glisse hors de la cible annule ;
    - nouveau banc `reglages-appuis` : 100 appuis par cas, au menu et en pause (`WFS_APPUIS_ESSAIS=0` : captures seules).
  - `src/ui/ecrans/reglages_appui.h` (nouveau) : calculs seuls de la cible et de l'anti-rebond ; `src/ui/ecrans/menu.cpp`, `pause.cpp` : `toucheReglages(x, y, doigt)` laisse aussi passer le doigt posé sur « Réglages » (un glissé ne déclenche pas un choix du menu).
  - `src/platform/retour_systeme.h`, `.cpp` (**accroche**, annoncée au journal) : `actionRetourReglages` : **touche Retour du téléphone, réglages ouverts en pause → « Fermer »** (RG-04) ; avant, la course reprenait.
  - `src/platform/desktop_main.cpp` (accroche de 1 ligne) : banc `reglages-appuis`.
  - **Tests CPU** (1er refus) : `tests/test_reglages_appui.cpp` (nouveau : cible de 48 dp, marges de 4 dp, anti-rebond refusé à 0,29 s et accepté à 0,30 s) ; `tests/test_retour_systeme.cpp` (pause avec réglages ouverts → « Fermer », réglages fermés → reprise, et dans le parcours modèle).
- **Comment c'est vérifié** : 80 tests CPU verts ; parcours complet 0 écart (et vue Cockpit 0 écart) ; branche synchronisée avec main (0c669429) ; banc `reglages-appuis` en 2340 × 1080 (2,62 px/dp), **avant** (main + le banc seul, sans les corrections) / **après** (relancé après les corrections du 1er refus) :

| Cas (100 appuis) | Menu avant | Menu après | Pause avant | Pause après |
|---|---|---|---|---|
| centre du dessin | 100 / 100 | 100 / 100 | 100 / 100 | 100 / 100 |
| n'importe où sur le dessin | 100 / 100 | 100 / 100 | 100 / 100 | 100 / 100 |
| **doigt dans une cible de 48 dp** | **ouvrir 36, fermer 21 / 36** | 100 / 100 | **ouvrir 51, fermer 20 / 51** | 100 / 100 |
| appui long (0,5 s) | 100 / 100 | 100 / 100 | 100 / 100 | 100 / 100 |
| **double appui (2 images d'écart)** | **0 / 100** | 100 / 100 | **0 / 100** | 100 / 100 |
| **appui juste après le changement d'écran** | **0 / 100** | 100 / 100 | **0 / 100** | 100 / 100 |
| autre doigt posé | 100 / 100 | 100 / 100 | 100 / 100 | 100 / 100 |
| **touche Retour (réglages ouverts)** | — | — | **0 / 100 (la course reprenait)** | 100 / 100 |

- **Limites et risques connus** :
  - L'action passe de l'appui au relâcher (comme les choix du menu, RG-04) : un doigt qui glisse hors de la cible de 48 dp n'ouvre plus rien (voulu).
  - La cible élargie dépasse le dessin de quelques dp : en haut à droite du menu et de la pause, rien d'autre n'est touchable à cet endroit ; dans le panneau, la cible de « Fermer » (25 px de plus en haut et en bas) s'arrête avant la 1re ligne de choix et loin du « Regard cockpit ».
  - L'anti-rebond suppose qu'on n'ouvre et ne ferme jamais exprès en moins de 0,3 s.
  - Le banc mesure des appuis injectés au PC ; la confirmation finale se fait sur le téléphone de l'utilisateur (🟡).

## Captures pour le lead dev (8 après)

Dossier : [`qa/pr/WFS-290`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-290/qa/pr/WFS-290) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `reglages-appuis-menu-ferme` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-290/qa/pr/WFS-290/avant-reglages-appuis-menu-ferme.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-290/qa/pr/WFS-290/apres-reglages-appuis-menu-ferme.jpg?raw=true) |
| `reglages-appuis-menu-fermer-appuye` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-290/qa/pr/WFS-290/avant-reglages-appuis-menu-fermer-appuye.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-290/qa/pr/WFS-290/apres-reglages-appuis-menu-fermer-appuye.jpg?raw=true) |
| `reglages-appuis-menu-ouvert` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-290/qa/pr/WFS-290/avant-reglages-appuis-menu-ouvert.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-290/qa/pr/WFS-290/apres-reglages-appuis-menu-ouvert.jpg?raw=true) |
| `reglages-appuis-menu-reglages-appuye` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-290/qa/pr/WFS-290/avant-reglages-appuis-menu-reglages-appuye.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-290/qa/pr/WFS-290/apres-reglages-appuis-menu-reglages-appuye.jpg?raw=true) |
| `reglages-appuis-pause-ferme` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-290/qa/pr/WFS-290/avant-reglages-appuis-pause-ferme.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-290/qa/pr/WFS-290/apres-reglages-appuis-pause-ferme.jpg?raw=true) |
| `reglages-appuis-pause-fermer-appuye` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-290/qa/pr/WFS-290/avant-reglages-appuis-pause-fermer-appuye.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-290/qa/pr/WFS-290/apres-reglages-appuis-pause-fermer-appuye.jpg?raw=true) |
| `reglages-appuis-pause-ouvert` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-290/qa/pr/WFS-290/avant-reglages-appuis-pause-ouvert.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-290/qa/pr/WFS-290/apres-reglages-appuis-pause-ouvert.jpg?raw=true) |
| `reglages-appuis-pause-reglages-appuye` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-290/qa/pr/WFS-290/avant-reglages-appuis-pause-reglages-appuye.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-290/qa/pr/WFS-290/apres-reglages-appuis-pause-reglages-appuye.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv5pG5V2148UwYk53fJ
