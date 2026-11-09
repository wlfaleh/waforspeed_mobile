Agent : Claude, agent 2 (01UY1Vv5)

**Ticket** : WFS-318 — règles : `regles-de-gestion/WFS-318.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : « je vois une voiture fantôme dans le mode Checkpoint, c'est quoi ? » → la mettre dans les Réglages (2026-11-08 14:00 UTC) : c'est le fantôme de son record (WFS-125), ni expliqué à l'écran ni réglable.
- **Ce qui change** :
  - `src/game/fantome.h`, `.cpp` (E3) — **RG-01, RG-02** : `fantome::affiche()` (Affiché par défaut) ; **RG-03** : `tempsRecord` (« 1:12,4 ») et `opaciteEtiquette` (pleine de 0 à 80 m, fondu jusqu'à 200 m ; derrière le joueur, fondu de 0 à −10 m).
  - `src/game/fantome_etiquette.cpp` (nouveau, E3) — **étiquette « VOTRE RECORD · 1:12,4 » au-dessus du fantôme** : même dessin que les noms des adversaires du duel (fond sombre, liseré cyan, plus petite au loin), 12 à 16 dp pour rester lisible sur téléphone, opacité selon la place du fantôme ; rien quand le fantôme est caché. Le temps affiché est la durée de la course du record (trace du fantôme). Banc `fantome` : Checkpoint, Ville de nuit, record de 72,4 s, fantôme devant ; puis la même course fantôme caché.
  - `src/ui/ecrans/reglages.cpp` (E8) — **ligne « Fantôme du record : Affiché / Caché »** en dernière ligne de l'onglet Affichage (place prévue par WFS-291) ; enregistrée (clé `fantome` du fichier des réglages) ; même style que les autres lignes ; grand panneau : hauteur comptée pour 13 lignes comme avant (« Fermer » ne bouge pas).
  - `src/ui/ecrans/reglages_onglets.h` (nouveau) : table des lignes et des onglets sortie du `.cpp` pour être testée (remarque du lead dev sur WFS-291).
  - **Accroches annoncées au journal** : `src/game/course_system.cpp` / `.h` (10 lignes : fantôme non dessiné quand il est caché, `fantomeCourant` pour l'étiquette) ; `src/platform/desktop_main.cpp` (2 lignes : banc `fantome`). **Aucun changement à `hud.*`** (occupé par WFS-338) ni au fantôme lui-même (RG-04 : transparent, sans choc, inchangé). **RG-02** : caché, l'enregistrement de la trace du record continue (seul le dessin est coupé).
  - Tests CPU : `tests/test_fantome_etiquette.cpp` (Affiché par défaut, « 1:12,4 », arrondis, fondu) ; `tests/test_reglages_onglets.cpp` (chaque ligne dans un seul onglet, ordre validé par le PO, « Fantôme du record » en dernière ligne d'Affichage).
- **Comment c'est vérifié** : 87 tests CPU verts ; parcours complet 0 écart (et vue Cockpit 0 écart) ; branche synchronisée avec main ; banc `fantome` en 2340 × 1080 : fantôme affiché 53 m devant, opacité de l'étiquette 1,00, « Votre record · 1:12,4 » ; caché : ni fantôme ni étiquette ; banc `reglages-onglets` en 2400 × 1080 (avant = main, qui a déjà les onglets) et la grande mise en page en 1920 × 1440 (remarque du lead dev sur WFS-291). L'« avant » du fantôme est main + le banc seul (pas d'étiquette, pas de réglage : le fantôme reste dessiné dans la capture « caché »).
- **Limites et risques connus** :
  - Le temps de l'étiquette est la durée de la course du record (le record du Checkpoint est un score : celui-ci n'est pas affiché sur l'étiquette, RG-03 demande le temps).
  - L'étiquette est dessinée sous le HUD (ordre 419) : elle peut passer derrière un panneau du HUD quand le fantôme est en haut de l'écran.

## Captures pour le lead dev (6 après)

Dossier : [`qa/pr/WFS-318`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-318/qa/pr/WFS-318) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `fantome-cache-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-318/qa/pr/WFS-318/avant-fantome-cache-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-318/qa/pr/WFS-318/apres-fantome-cache-nuit.jpg?raw=true) |
| `fantome-etiquette-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-318/qa/pr/WFS-318/avant-fantome-etiquette-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-318/qa/pr/WFS-318/apres-fantome-etiquette-nuit.jpg?raw=true) |
| `reglages-onglet-affichage-menu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-318/qa/pr/WFS-318/avant-reglages-onglet-affichage-menu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-318/qa/pr/WFS-318/apres-reglages-onglet-affichage-menu.jpg?raw=true) |
| `reglages-onglet-conduite-pause` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-318/qa/pr/WFS-318/avant-reglages-onglet-conduite-pause.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-318/qa/pr/WFS-318/apres-reglages-onglet-conduite-pause.jpg?raw=true) |
| `reglages-onglet-son-gaucher` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-318/qa/pr/WFS-318/avant-reglages-onglet-son-gaucher.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-318/qa/pr/WFS-318/apres-reglages-onglet-son-gaucher.jpg?raw=true) |
| `reglages-onglet-affichage-grand-1920x1440` (grande mise en page) | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-318/qa/pr/WFS-318/apres-reglages-onglet-affichage-grand-1920x1440.jpg?raw=true) |
| `reglages-onglet-son-pause` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-318/qa/pr/WFS-318/avant-reglages-onglet-son-pause.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-318/qa/pr/WFS-318/apres-reglages-onglet-son-pause.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv5pG5V2148UwYk53fJ
