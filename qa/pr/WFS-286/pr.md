## Agent : Claude, agent 1 (01UY1Vv2)

**Ticket** : WFS-286 — règles : `regles-de-gestion/WFS-286.md`

## Description (à remplir par le développeur avant d'ouvrir la PR)

- **Demande de l'utilisateur** (en une phrase) : corriger deux défauts trouvés par la campagne de test (VABF) : le bouton « FERMER » des Réglages qui coupait la liste (ANO-156) et le duel à 4 qui affichait 0:00.0 et +0,0 s (ANO-157), sans toucher à la taille des boutons.
- **Ce qui change** (fichiers, comportement, à l'écran) :
  - `src/ui/ecrans/reglages.cpp`, mise en page grand écran (1280×720, tablette) : les 13 lignes se suivent avec le même pas, « FERMER » est sous la dernière ligne, à droite, et la ligne « Rendu » est à sa gauche. Avant, « Aide à la direction » et « Regard cockpit » étaient sous le bouton. Tailles des boutons et des textes inchangées, hauteur du panneau inchangée. Mêmes Réglages pour le menu, la pause et la manette. La mise en page du téléphone (deux colonnes, « FERMER » en haut à droite) n'a pas changé.
  - `src/ui/ecrans/salon.cpp`, banc seulement : la manche du duel à 4 dure 12 s de course avant l'arrivée forcée. Avant, l'arrivée était forcée dès le départ : mon temps valait 0, et les temps des 3 amis simulés, calculés à partir du mien, aussi. Un vrai duel n'avait pas ce défaut : le temps d'arrivée est le vrai temps de course (`Session::raceTick`). Le banc vérifie maintenant que le vainqueur a au moins 10 s et que les écarts sont croissants. Aucune modification dans `src/game/duel*`.
- **Comment c'est vérifié** (tests CPU, parcours, bancs, mesures) :
  - 63 tests CPU verts ;
  - banc `menu2` : parcours duel conforme (0 écart, 22 vérifications). Temps de la manche 1 : 81,9 / 83,4 / 84,9 / 86,4 s ; manche 2 (Revanche) : 79,5 / 81,0 / 82,5 / 84,0 s. Ils comptent le temps réel du banc, ralenti par le rendu logiciel. Ensuite « Retour » ramène au menu des modes ;
  - captures avant / après ci-dessous en 1280×720 : en 2400×1080, le banc prend la mise en page du téléphone, déjà correcte et identique avant / après.
- **Limites et risques connus** : aucune modification du jeu en duel réel. WFS-290 (même fichier `reglages.cpp`) attend cette fusion.

## Captures pour le lead dev (6 après)

Dossier : [`qa/pr/WFS-286`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-286/qa/pr/WFS-286) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `duel-resultat-4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-286/qa/pr/WFS-286/avant-duel-resultat-4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-286/qa/pr/WFS-286/apres-duel-resultat-4.jpg?raw=true) |
| `manette-reglages` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-286/qa/pr/WFS-286/avant-manette-reglages.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-286/qa/pr/WFS-286/apres-manette-reglages.jpg?raw=true) |
| `reglages-menu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-286/qa/pr/WFS-286/avant-reglages-menu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-286/qa/pr/WFS-286/apres-reglages-menu.jpg?raw=true) |
| `reglages-pause` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-286/qa/pr/WFS-286/avant-reglages-pause.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-286/qa/pr/WFS-286/apres-reglages-pause.jpg?raw=true) |
| `reglages` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-286/qa/pr/WFS-286/avant-reglages.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-286/qa/pr/WFS-286/apres-reglages.jpg?raw=true) |
| `salon-amis-fin` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-286/qa/pr/WFS-286/avant-salon-amis-fin.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-286/qa/pr/WFS-286/apres-salon-amis-fin.jpg?raw=true) |
