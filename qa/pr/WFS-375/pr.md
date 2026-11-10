Agent : Claude, agent dev M (01UY1Vv4mu)

**Ticket** : WFS-375 (P1 urgent, ANO-232) — règles : `regles-de-gestion/WFS-375.md`

## Description
- **Demande de l'utilisateur** : sur « Choisis ton terrain », garder seulement deux modes simples, JOUR et NUIT ; avec JOUR, la course doit être en plein jour, quelle que soit l'heure du téléphone (vidéo v0.1.969 à 22 h : JOUR donnait une course noire).
- **Ce qui change** :
  - `src/ui/ecrans/terrain.cpp` : JOUR = `Moment::Midi`, NUIT = `Moment::Nuit` (fonction `modeSimple`) ; un 2e appui ne fait plus rien défiler (fin de `JOURS[5]` / `NUITS[2]` / `suivant` / `etiquette`) ; l'étiquette est toujours « JOUR » ou « NUIT » (RG-01, RG-02) ; à l'entrée de l'écran, un ancien moment de la sauvegarde se range dans l'un des deux (RG-03).
  - `src/game/sauvegarde.cpp` (+ commentaire de `sauvegarde.h`) : à la lecture, `moment=` aube, matin, midi, doree, reel ou inconnu → `midi` ; bleue, nuit → `nuit` (RG-03). Cela couvre aussi l'éclairage, qui lit la sauvegarde au lancement avant l'écran Terrain (`eclairage_physique.cpp:504`, non modifié).
  - Code des moments (`atmosphere_physique.*`, `eclairage_*`, bancs WFS-208) **inchangé** (RG-04).
  - Banc `benchTerrain` : nouveau scénario (deux appuis sur JOUR puis Campagne, deux appuis sur NUIT puis Corniche), remise à zéro du moment à la fin.
- **Comment c'est vérifié** :
  - `tests/test_sauvegarde.cpp` : nouveau bloc WFS-375 (6 anciens moments → midi, bleue / nuit → nuit, sans moment → tod, aller-retour gardé) ; `tools/qa/tests_cpu.sh build` : **94/94 verts** (après synchronisation avec main 03ccc2e).
  - Banc `menu1` (parcours complet accueil → garage → terrain → course → retour, RG-05) en 1280x720 avec l'horloge réglée au **2026-11-09 22:15** (`faketime`) : journal « moment choisi » = `doree` / `bleue` sur main, `midi` / `nuit` sur la branche. Captures regardées une à une.
  - Avant = `origin/main` 03ccc2e + seulement le même scénario de banc (pour montrer le défaut) ; après = branche.
- **Limites et risques** : l'heure réelle du téléphone n'est plus jamais utilisée par le choix du joueur (voulu). Le haut du ciel de Campagne reste un peu sombre au milieu du jour : rendu du ciel (E13), hors périmètre. Deux « capture abandonnée (WFS-213) » dans le journal du banc, présentes aussi sur main. Pas de construction Android en local (APK par la CI de main). Après la fusion : 🟡 À valider (utilisateur).

## Captures pour le lead dev (8 avant / 8 après)

Dossier : [`qa/pr/WFS-375`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-375/qa/pr/WFS-375). À regarder : `terrain-jour-2appuis` (avant « DORÉE », après « JOUR »), `terrain-campagne-jour` (avant coucher du soleil, après plein jour à 22:15), `terrain-nuit-2appuis` (après « NUIT »), `terrain-corniche-nuit` (avant heure bleue, après nuit).

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `terrain-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-375/qa/pr/WFS-375/avant-terrain-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-375/qa/pr/WFS-375/apres-terrain-0.jpg?raw=true) |
| `terrain-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-375/qa/pr/WFS-375/avant-terrain-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-375/qa/pr/WFS-375/apres-terrain-1.jpg?raw=true) |
| `terrain-rouler` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-375/qa/pr/WFS-375/avant-terrain-rouler.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-375/qa/pr/WFS-375/apres-terrain-rouler.jpg?raw=true) |
| `terrain-jour-2appuis` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-375/qa/pr/WFS-375/avant-terrain-jour-2appuis.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-375/qa/pr/WFS-375/apres-terrain-jour-2appuis.jpg?raw=true) |
| `terrain-campagne-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-375/qa/pr/WFS-375/avant-terrain-campagne-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-375/qa/pr/WFS-375/apres-terrain-campagne-jour.jpg?raw=true) |
| `terrain-nuit-2appuis` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-375/qa/pr/WFS-375/avant-terrain-nuit-2appuis.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-375/qa/pr/WFS-375/apres-terrain-nuit-2appuis.jpg?raw=true) |
| `terrain-corniche-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-375/qa/pr/WFS-375/avant-terrain-corniche-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-375/qa/pr/WFS-375/apres-terrain-corniche-nuit.jpg?raw=true) |
| `parcours-terrain` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-375/qa/pr/WFS-375/avant-parcours-terrain.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-375/qa/pr/WFS-375/apres-parcours-terrain.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv4muvF1iSQB2kbyP6G
