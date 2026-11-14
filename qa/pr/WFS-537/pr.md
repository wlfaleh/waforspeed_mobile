## Agent : Claude, agent dev3d (01UY1Vv4Hv)

**Ticket** : WFS-537 — règles : `regles-de-gestion/WFS-537.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : en fin de course, montrer le détail du score et les étoiles gagnées (« +N ★ ») dans les 4 modes, en français et en anglais, et afficher le solde d'étoiles en haut du menu principal, à côté de « Réglages » (épic E22).
- **Ce qui change** :
  - **Écran de fin des 4 modes** : carte « SCORE » à droite du panneau existant (RG-01) : une ligne par élément du score avec ses points, le total, « +N ★ » compté de 0 à N en 0,8 s (son court existant `bip-checkpoint` à la fin), le nouveau solde (« 1 étoile » au singulier) et « Nouveau record ! » (RG-02). Course quittée : pas de carte (RG-03). « Rejouer » / « Menu » : même place, même taille, jamais couverts (RG-04). Course solo en 16:9 : panneau un peu moins large (boutons centrés inchangés), libellés des statistiques par `ui::textBoite`.
  - **Menu principal** : compteur ★ + solde à gauche de « Réglages », même hauteur et même style (RG-07) ; « 12 450 » / « 12,450 », relu à chaque image (RG-08) ; appui → garage (RG-09). Le compteur est un petit système (461) dessiné **juste après « Réglages » à la même image** et seulement si ce bouton a été dessiné (donc caché sous « Crée ton pilote », le choix du mode, les Crédits et l'écran du cadeau du jour), avec le fondu d'entrée du menu (0,3 s).
  - **Fichiers nouveaux** : `src/game/score_fin.{h,cpp}`, `src/game/score_fin_calc.h`, `src/game/score_fin_banc.cpp`, `src/ui/ecrans/compteur_etoiles.{h,cpp}`, `tests/test_score_fin.cpp` ; textes `fin.score.*`, `score.*`, `menu.etoiles.lecteur(.un)` FR / EN.
  - **Accroches ≤ 10 lignes (journal)** : `profil_system.cpp`, `course_system.cpp`, `course5_system.cpp`, `poursuite_system.cpp`, `duel_system.cpp` (+ paramètre de banc `gagne`), `menu.cpp`, `reglages.{h,cpp}` (`boutonReglages`, lu une fois par image), `desktop_main.cpp`, `CMakeLists.txt`.
- **Comment c'est vérifié** (branche synchronisée avec `main`, WFS-481, WFS-501 et WFS-484 compris) :
  - Banc **`WFS_BANC=score-fin`** vert en 2400 × 1080 et 1280 × 720 : 1er lancement → « Crée ton pilote » ouvert et compteur caché (capture `score-menu-creation-fr`), puis profil du banc créé comme WFS-481 → compteur dessiné ; menu à 0 / 250 / 12 450 ★ ; course solo, course à 5 gagnée, Poursuite perdue, duel gagné en FR et EN (détail présent, solde augmenté exactement des ★ montrées, carte dans l'écran, « Rejouer » / « Menu » hors de la carte) ; parcours accueil → course → fin → menu (compteur à jour) ; appui sur le compteur → garage ; course quittée sans carte ; garde-fous WFS-504 : 0 échec.
  - `test_score_fin` (compte, nombres FR / EN, singulier) ; tests CPU 136 / 136 ; bancs `textes-course` et `textes-menus` VERTS pendant la passe de captures ; banc `cadeau` (WFS-501) : code 0, compteur caché sous l'écran du cadeau (`apres-cadeau-etoiles-fr-1280x720.jpg`).
  - Menu : avec WFS-501, le menu a `CADEAU = 5` et `COMPTEUR = 6` ; le bouton « Cadeau du jour » et le compteur sont tous deux visibles (`textes-fr-menu-principal`).
- **Limites et risques connus** :
  - Pas encore de lien avec le lecteur d'écran d'Android (TalkBack) : texte prêt (`compteur_etoiles::texteLecteur`) et vérifié au banc, pas encore envoyé à Android.
  - En 1280 × 720, la carte est réduite (80 %) pour tenir à côté du panneau.
  - Défis des modes pas encore comptés (WFS-535 / 536 : 0 point) ; ils apparaîtront tout seuls.
  - Remarques hors zone pour le PO (vues par le lead dev) : chocs de la Poursuite, « NOUVEAU RECORD » contre « 1re course », séparateur de « BEST » en anglais.

## Tests CPU (branche synchronisée avec `main`)

```
[OK] test_ville_sol (3 s)
[OK] test_virages_fleches (0 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 136 test(s) réussi(s) en 325 s
code de sortie : 0
```
Journal complet : `qa/pr/WFS-537/tests-cpu.log`.

## Captures pour le lead dev (40 après)

Dossier : [`qa/pr/WFS-537`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `cadeau-etoiles-fr-1280x720` (banc `cadeau` : compteur caché) | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-cadeau-etoiles-fr-1280x720.jpg?raw=true) |
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
| `score-menu-creation-fr-1280x720` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-menu-creation-fr-1280x720.jpg?raw=true) |
| `score-menu-creation-fr-2400x1080` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-537/qa/pr/WFS-537/apres-score-menu-creation-fr-2400x1080.jpg?raw=true) |
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
