Agent : Claude, agent 2 (01UY1Vv5)

**Ticket** : WFS-291 — règles : `regles-de-gestion/WFS-291.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : « il faut un réglage avec tabulation, c'est plus clair, deux ou trois onglets » (2026-11-08 00:04 UTC).
- **Ce qui change** : `src/ui/ecrans/reglages.cpp` (zone E8), découpage **validé par le PO** (`regles-de-gestion/WFS-291.md`, 2026-11-08 23:04:56 UTC) :
  - **3 onglets** en haut du panneau : **Affichage** (Caméra, Graphismes, Regard cockpit, Réduire les mouvements, Boutons ; place prévue pour « Fantôme du record », WFS-318), **Conduite** (Direction, Commandes, Gyroscope, Zone morte gyro, Aide à la direction, Vibrations), **Son** (Son, Musique) ; chaque ligne dans un seul onglet (13 lignes, aucune oubliée ni en double) ;
  - **RG-02** : toutes les lignes gardent leurs valeurs et leur enregistrement ; seules les lignes de l'onglet ouvert sont dessinées et touchables ;
  - **RG-03** : style `design-street-racing` : onglet actif comme un choix actif (fond cyan léger, trait et texte cyan) ; sur téléphone, les onglets prennent la place de « Regard cockpit » dans l'en-tête (40 dp de haut), qui passe dans Affichage ; **dernier onglet gardé** (nouvelle clé `onglet` du fichier des réglages ; absente ou inconnue → Affichage) ;
  - **RG-04** : même panneau au menu et en pause, dans les deux mises en page (téléphone et grande) ; **taille du panneau et place de « Fermer » inchangées** ; « Fermer » et la touche Retour ferment le panneau depuis chaque onglet ; la **manette** passe d'un onglet à l'autre (onglets dans le focus, lignes de l'onglet ouvert seulement) ; aucun bouton hors du panneau ne change ;
  - remarque du lead dev sur WFS-290 : `onEcran` remet aussi `g_doigt` à zéro ;
  - bancs : nouveau `reglages-onglets` (accroche de 1 ligne dans `desktop_main.cpp`) ; le banc `reglages` touche l'onglet d'un choix avant le choix ; le banc manette passe à l'onglet « Conduite » avant « Direction » s'il le faut.
- **Comment c'est vérifié** : 83 tests CPU verts (`test_audio_chocs` échoue seulement quand un banc tourne en même temps, test de temps de calcul ; seul : vert) ; parcours complet 0 écart (et vue Cockpit 0 écart) ; banc `menu2` (réglages, manette) : « Direction : Flèches » par la manette, vue Capot, musique, gaucher, boutons grands, comme avant (l'erreur « pipelines WFS-262 » du début de ce banc existe aussi sur main) ; banc `reglages-onglets` en 2400 × 1080 : « Fermer » depuis Affichage → fermé ; touche Retour depuis Son → fermé, pause gardée ; **onglet relu après redémarrage : Son (Son attendu)** ; « avant » = main + le banc seul (sans onglets : le panneau entier à chaque capture).
- **Limites et risques connus** :
  - Sur téléphone, l'onglet Son n'a que 2 lignes : la place libre sous les lignes est gardée (taille du panneau inchangée, « Fermer » ne bouge pas) ;
  - un joueur qui cherche un réglage doit d'abord choisir l'onglet (découpage validé par le PO).

## Captures pour le lead dev (4 après)

Dossier : [`qa/pr/WFS-291`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-291/qa/pr/WFS-291) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `reglages-onglet-affichage-menu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-291/qa/pr/WFS-291/avant-reglages-onglet-affichage-menu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-291/qa/pr/WFS-291/apres-reglages-onglet-affichage-menu.jpg?raw=true) |
| `reglages-onglet-conduite-pause` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-291/qa/pr/WFS-291/avant-reglages-onglet-conduite-pause.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-291/qa/pr/WFS-291/apres-reglages-onglet-conduite-pause.jpg?raw=true) |
| `reglages-onglet-son-gaucher` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-291/qa/pr/WFS-291/avant-reglages-onglet-son-gaucher.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-291/qa/pr/WFS-291/apres-reglages-onglet-son-gaucher.jpg?raw=true) |
| `reglages-onglet-son-pause` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-291/qa/pr/WFS-291/avant-reglages-onglet-son-pause.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-291/qa/pr/WFS-291/apres-reglages-onglet-son-pause.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv5pG5V2148UwYk53fJ
