## Agent : Claude, agent dev3d (01UY1Vv4Hv)

**Ticket** : WFS-537 — règles : `regles-de-gestion/WFS-537.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : en fin de course, montrer le détail du score et les étoiles gagnées (« +N ★ ») dans les 4 modes, en français et en anglais, et afficher le solde d'étoiles en haut du menu principal, à côté de « Réglages » (épic E22, `regles-de-gestion/WFS-537.md`).
- **Ce qui change** :
  - **Écran de fin des 4 modes** (course solo, course à 5, Poursuite, duel) : une carte « SCORE » à droite du panneau existant (RG-01) : une ligne par élément du score avec ses points (place ou issue, temps / record, conduite propre, défis), le total, puis **« +N ★ » qui compte de 0 à N en 0,8 s** avec le son court existant `bip-checkpoint` à la fin, le **nouveau solde** et « Nouveau record ! » si le record est battu (RG-02). Course quittée : pas de carte (RG-03). Les boutons « Rejouer » / « Menu » gardent leur place et leur taille ; la carte ne les couvre jamais (RG-04, vérifié au banc en 2400 × 1080 et 1280 × 720). Course solo en 16:9 seulement : le panneau est un peu moins large (les boutons, centrés, ne bougent pas), et ses 4 libellés de statistiques passent par `ui::textBoite` (réduits de 20 % au plus si besoin).
  - **Menu principal** : compteur ★ + solde à gauche de « Réglages », même hauteur et même style vitré ; « Réglages » et les autres boutons ne bougent pas (RG-07) ; « 12 450 » en français, « 12,450 » en anglais, relu à chaque image (RG-08) ; appui → garage (RG-09, le Market viendra avec WFS-513).
  - **Fichiers nouveaux** : `src/game/score_fin.{h,cpp}` (système ScoreFin, 405), `src/game/score_fin_calc.h` (calculs purs), `src/game/score_fin_banc.cpp` (banc), `src/ui/ecrans/compteur_etoiles.{h,cpp}`, `tests/test_score_fin.cpp`. Textes `fin.score.*`, `score.*`, `menu.etoiles.lecteur` dans `assets/textes/fr.txt` et `en.txt` (RG-05).
  - **Accroches ≤ 10 lignes, annoncées au journal** : `profil_system.cpp` (le score déjà calculé par WFS-536 est publié pour l'écran avec les ★ vraiment reçues et le nouveau solde, effacé au départ et si la course est quittée), `course_system.cpp`, `course5_system.cpp`, `poursuite_system.cpp`, `duel_system.cpp` (`score_fin::poser`, + paramètre de banc `gagne` de `benchDuelTextes`), `menu.cpp`, `reglages.{h,cpp}` (accesseur `boutonReglages`), `desktop_main.cpp` (banc), `CMakeLists.txt` (test). Remarque : l'écran de fin n'est pas dans `modes_ecran.cpp` (c'est le choix du mode) mais dans chaque système de mode.
- **Comment c'est vérifié** :
  - Nouveau banc **`WFS_BANC=score-fin`**, vert en 2400 × 1080 et en 1280 × 720 : vraies courses avec sauvegarde active, en français puis en anglais : course solo, course à 5 gagnée, Poursuite perdue, duel gagné → à chaque fois détail présent, solde augmenté exactement des ★ montrées, carte dans l'écran, centres de « Rejouer » et « Menu » hors de la carte ; menu à 0 / 250 / 12 450 ★ ; parcours accueil → course → fin → « Menu » avec compteur = nouveau solde ; appui sur le compteur → garage ; course quittée → pas de carte, solde inchangé ; garde-fous de WFS-504 : 0 échec.
  - `test_score_fin` (compte < 1 s, croissant, borné ; « 12 450 » / « 12,450 », solde maximal, sans débordement). Banc `textes-course` (WFS-506) : VERT après le changement. Banc `parcours` (1280 × 720) : « parcours : 0 écart(s) », parcours complet en vue Cockpit : 0 écart.
  - Banc `textes-menus` : 4 échecs de parcours dans le salon / Crédits, **identiques sur `main`** (passe « avant » du même script) : déjà là avant ce ticket (CI de `main` #1054 rouge sur le groupe texte), pas de mon fait.
- **Limites et risques connus** :
  - Le jeu natif n'a pas encore de lien avec le lecteur d'écran d'Android (TalkBack) : le texte à lire (« 12 450 étoiles » / « 12,450 stars ») est prêt (`compteur_etoiles::texteLecteur`) et vérifié au banc, mais rien ne l'envoie encore à Android.
  - En 1280 × 720 (16:9), la carte est réduite (80 %) pour tenir à côté du panneau : texte plus petit qu'en 2400 × 1080.
  - Les défis des modes ne sont pas encore comptés par les modes (WFS-535 / 536 : 0 point) : ils apparaîtront tout seuls dans la carte quand ils le seront.
  - Les captures déjà existantes des écrans de fin (autres bancs) montrent maintenant la carte « SCORE » (sans solde quand le banc n'a pas de sauvegarde) : changement attendu.

## Tests CPU (branche synchronisée avec `main`, commit à jour de la PR)

```
[OK] test_ville_lieux (0 s)
[OK] test_ville_sol (3 s)
[OK] test_virages_fleches (0 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 131 test(s) réussi(s) en 386 s
code de sortie : 0
```
Journal complet : `qa/pr/WFS-537/tests-cpu.log`.

## Captures pour le lead dev (38 après)

Dossier : [`qa/pr/WFS-537`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `score-fin-en-course5-1280x720` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-fin-en-course5-1280x720.jpg?raw=true) |
| `score-fin-en-course5-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-fin-en-course5-2400x1080.jpg?raw=true) |
| `score-fin-en-duel-1280x720` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-fin-en-duel-1280x720.jpg?raw=true) |
| `score-fin-en-duel-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-fin-en-duel-2400x1080.jpg?raw=true) |
| `score-fin-en-poursuite-1280x720` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-fin-en-poursuite-1280x720.jpg?raw=true) |
| `score-fin-en-poursuite-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-fin-en-poursuite-2400x1080.jpg?raw=true) |
| `score-fin-en-solo-1280x720` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-fin-en-solo-1280x720.jpg?raw=true) |
| `score-fin-en-solo-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-fin-en-solo-2400x1080.jpg?raw=true) |
| `score-fin-fr-course5-1280x720` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-fin-fr-course5-1280x720.jpg?raw=true) |
| `score-fin-fr-course5-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-fin-fr-course5-2400x1080.jpg?raw=true) |
| `score-fin-fr-duel-1280x720` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-fin-fr-duel-1280x720.jpg?raw=true) |
| `score-fin-fr-duel-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-fin-fr-duel-2400x1080.jpg?raw=true) |
| `score-fin-fr-poursuite-1280x720` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-fin-fr-poursuite-1280x720.jpg?raw=true) |
| `score-fin-fr-poursuite-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-fin-fr-poursuite-2400x1080.jpg?raw=true) |
| `score-fin-fr-solo-1280x720` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-fin-fr-solo-1280x720.jpg?raw=true) |
| `score-fin-fr-solo-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-fin-fr-solo-2400x1080.jpg?raw=true) |
| `score-menu-en-0-1280x720` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-menu-en-0-1280x720.jpg?raw=true) |
| `score-menu-en-0-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-menu-en-0-2400x1080.jpg?raw=true) |
| `score-menu-en-12450-1280x720` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-menu-en-12450-1280x720.jpg?raw=true) |
| `score-menu-en-12450-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-menu-en-12450-2400x1080.jpg?raw=true) |
| `score-menu-en-250-1280x720` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-menu-en-250-1280x720.jpg?raw=true) |
| `score-menu-en-250-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-menu-en-250-2400x1080.jpg?raw=true) |
| `score-menu-fr-0-1280x720` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-menu-fr-0-1280x720.jpg?raw=true) |
| `score-menu-fr-0-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-menu-fr-0-2400x1080.jpg?raw=true) |
| `score-menu-fr-12450-1280x720` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-menu-fr-12450-1280x720.jpg?raw=true) |
| `score-menu-fr-12450-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-menu-fr-12450-2400x1080.jpg?raw=true) |
| `score-menu-fr-250-1280x720` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-menu-fr-250-1280x720.jpg?raw=true) |
| `score-menu-fr-250-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-menu-fr-250-2400x1080.jpg?raw=true) |
| `score-parcours-en-menu-1280x720` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-parcours-en-menu-1280x720.jpg?raw=true) |
| `score-parcours-en-menu-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-parcours-en-menu-2400x1080.jpg?raw=true) |
| `score-parcours-fr-menu-1280x720` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-parcours-fr-menu-1280x720.jpg?raw=true) |
| `score-parcours-fr-menu-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-parcours-fr-menu-2400x1080.jpg?raw=true) |
| `textes-fr-duel-fin` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/avant-textes-fr-duel-fin.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-textes-fr-duel-fin.jpg?raw=true) |
| `textes-fr-fin-checkpoint` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/avant-textes-fr-fin-checkpoint.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-textes-fr-fin-checkpoint.jpg?raw=true) |
| `textes-fr-fin-course5` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/avant-textes-fr-fin-course5.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-textes-fr-fin-course5.jpg?raw=true) |
| `textes-fr-fin-poursuite-arrete` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/avant-textes-fr-fin-poursuite-arrete.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-textes-fr-fin-poursuite-arrete.jpg?raw=true) |
| `textes-fr-fin-poursuite` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/avant-textes-fr-fin-poursuite.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-textes-fr-fin-poursuite.jpg?raw=true) |
| `textes-fr-menu-principal` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/avant-textes-fr-menu-principal.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-textes-fr-menu-principal.jpg?raw=true) |
