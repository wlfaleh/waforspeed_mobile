## Agent : Claude, agent dev3d (01UY1Vv5BA)

**Ticket** : WFS-567 — règles : `regles-de-gestion/WFS-567.md`

## Description (à remplir par le développeur avant d'ouvrir la PR)

- **Demande de l'utilisateur** (en une phrase) : dans le jeu, les messages de course (checkpoint, quasi-contact, combo, accident, police, surprises de la Poursuite, conseil de départ) en aplat cyan, orange ou rouge empilés au milieu de la route sont laids et illisibles : il les veut propres, lisibles, sans cyan et sans cacher la route (ANO-315, ANO-290).
- **Ce qui change** (fichiers, comportement, à l'écran) :
  - `src/ui/toasts.h` : nouveau `namespace messages` : style (texte `TEXTE` sur fond `SOMBRE` à 72 %, filet orange `ACCENT` pour les gains, rouge `DANGER` pour accident / police / perte, blanc pour l'information ; aucun cyan), taille (lettres 3,75 % de H), durées (vie 2 s, fondus 0,18 / 0,20 s), place (`place`, `haut`, `largeurMax`, `zoneRoute`) et géométrie de l'encart SCORE / POLICE (`poursuiteEncartBas`, `poursuiteEncartDemi`).
  - `src/ui/toasts.cpp` : pile de **2 messages au plus**, en haut sous la rangée du HUD (centrés ; en Poursuite, alignés à gauche sous le chrono, à côté de l'encart) ; plus d'aplat plein : fond sombre au bord gauche incliné + filet de couleur ; la série de quasi-contacts se **regroupe** en un seul message (« QUASI-CONTACT ×3 +750 ») au lieu de « QUASI-CONTACT » + « COMBO ×N » empilés ; trop large : police réduite jusqu'à 3,5 % de H, puis valeur retirée.
  - `src/game/poursuite_system.cpp` / `poursuite.h` : le bandeau des surprises (« CAMION EN TRAVERS », « BARRAGE DE POLICE »…) et le compte « ARRESTATION N » ne sont plus des aplats rouges collés à gauche : `poursuiteMessage()` les passe au système des messages (même style, même place, comptés dans les 2). Logique des annonces inchangée (durées, rappel des travaux).
  - `src/world/portique_depart.cpp` : conseil de départ dans le même style (filet blanc), jamais sous la rangée du HUD ni sous l'encart SCORE / POLICE, jamais dans la zone de la route ; police 2,8 % de H (avant 2,25 %) ; réduite seulement si l'écran est trop étroit (ANO-290 : entier en 1280x720 et en 20:9).
  - `src/game/trace_cartes_systeme.cpp` : « Nouveaux tracés » en information (filet blanc).
  - `assets/textes/fr.txt`, `en.txt` : titres des surprises ≤ 18 caractères (RG-03) : « VOITURE DÉBOÎTE », « TRAVAUX DEVANT » ; « CAR PULLING OUT », « ROADWORKS AHEAD », « TRUCK ACROSS ROAD ».
  - `tests/test_messages.cpp` (nouveau, `CMakeLists.txt`) et banc `WFS_BANC=messages` (`src/ui/messages_banc.cpp`, accroche d'une ligne dans `desktop_main.cpp`).
  - Maquette : sans objet (pas de maquette R19 pour les messages ; RG du ticket et skill `design-street-racing`).
- **Comment c'est vérifié** (tests CPU, parcours, bancs, mesures) :
  - `bash tools/qa/tests_cpu.sh build` : **156 tests réussis**, dont `test_messages` (aucun cyan ; contraste ≥ 4,5:1 sur fond blanc, gris et noir ; 2 messages au plus ; rang 0 et 1, court et le plus large permis, hors de la zone centrale de la route (50 % × 40 %), sous le HUD, hors de l'encart de la Poursuite, hors de la mini-carte, en 1280x720, 1600x720, 2400x1080, avec et sans encoche ; lettres ≥ 3,5 % de H ; vie 1,5 à 2,5 s, fondus ≤ 0,2 s ; titres ≤ 18 caractères en FR et EN).
  - Parcours complet (RG-08) : banc `textes-course` en FR **et** en EN : Checkpoint, Course à 5, Poursuite, duel : course → fin → « Rejouer » → fin → « Menu » → accueil : **0 écart de parcours, 0 échec du garde-fou** (aucune phrase française en anglais, aucun texte qui dépasse) ; aucune erreur de validation Vulkan au journal.
  - Captures du banc `messages` ci-dessous, avant (main) / après, jour et nuit, en 1280x720 et en 2400x1080 (20:9) pour le conseil de départ (RG-05). Les captures « avant » viennent de `main` (même commit) + le seul fichier du banc, rangées dans le cache du script.
- **Limites et risques connus** :
  - Le titre « COMBO ×N » séparé n'est plus montré : le combo est dans le titre regroupé « QUASI-CONTACT ×N » (RG-03) ; la clé `toast.combo` reste dans les tables.
  - La raison des pertes de la Poursuite (« -5 CHOC AVEC LE TRAFIC », sous l'encart) n'est pas listée par le ticket : elle ne change pas (pas de cyan).
  - Zone de code : la consigne de départ est dessinée par `src/world/portique_depart.cpp` (WFS-559) et le bandeau des surprises par `poursuite_system.cpp`, pas par `hud.cpp` / `poursuite.cpp` : seul leur dessin 2D change (noté au journal).
  - Pas encore vu sur téléphone : statut 🟡 À valider (utilisateur) après la fusion.

## Tests CPU

`bash tools/qa/tests_cpu.sh build` sur la branche synchronisée avec `main` (commit testé `11a0ab9e`) : **156 test(s) réussi(s) en 421 s**, 0 échec, dont le nouveau `test_messages` (aucun cyan, contraste ≥ 4,5:1, 2 messages au plus hors de la zone de la route en 1280x720, 1600x720 et 2400x1080 avec ou sans encoche, lettres ≥ 3,5 % de H, durées, titres ≤ 18 caractères FR / EN). Journal complet : [`qa/pr/WFS-567/tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/tests-cpu.log).

Fin du journal :
```
[OK] test_messages (0 s)
…
[OK] test_voitures_retirees (0 s)
tests CPU : 156 test(s) réussi(s) en 421 s
```

## Captures pour le lead dev (8 après)

Dossier : [`qa/pr/WFS-567`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567) — produit par `tools/qa/captures_pr.sh`.

Captures « avant » : `origin/main` = `63293244c3af75c993054d1213233ab698ed3de0` ; cache : 1 banc(s) relu(s), 0 banc(s) rejoué(s) ; main compilé : non.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `messages-checkpoint-depart` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/avant-messages-checkpoint-depart.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/apres-messages-checkpoint-depart.jpg?raw=true) |
| `messages-checkpoint-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/avant-messages-checkpoint-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/apres-messages-checkpoint-jour.jpg?raw=true) |
| `messages-poursuite-depart` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/avant-messages-poursuite-depart.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/apres-messages-poursuite-depart.jpg?raw=true) |
| `messages-poursuite-renfort` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/avant-messages-poursuite-renfort.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/apres-messages-poursuite-renfort.jpg?raw=true) |
| `messages-poursuite-surprise-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/avant-messages-poursuite-surprise-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/apres-messages-poursuite-surprise-jour.jpg?raw=true) |
| `messages-poursuite-surprise-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/avant-messages-poursuite-surprise-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/apres-messages-poursuite-surprise-nuit.jpg?raw=true) |
| `messages-serie-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/avant-messages-serie-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/apres-messages-serie-jour.jpg?raw=true) |
| `messages-serie-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/avant-messages-serie-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/apres-messages-serie-nuit.jpg?raw=true) |

### Écran 20:9 (2400x1080) — RG-05 (conseil de départ, ANO-290) et contrôle de la place

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `messages-checkpoint-depart-2400x1080` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/avant-messages-checkpoint-depart-2400x1080.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/apres-messages-checkpoint-depart-2400x1080.jpg?raw=true) |
| `messages-poursuite-depart-2400x1080` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/avant-messages-poursuite-depart-2400x1080.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/apres-messages-poursuite-depart-2400x1080.jpg?raw=true) |
| `messages-serie-jour-2400x1080` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/avant-messages-serie-jour-2400x1080.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/apres-messages-serie-jour-2400x1080.jpg?raw=true) |
| `messages-poursuite-surprise-nuit-2400x1080` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/avant-messages-poursuite-surprise-nuit-2400x1080.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-567/qa/pr/WFS-567/apres-messages-poursuite-surprise-nuit-2400x1080.jpg?raw=true) |
