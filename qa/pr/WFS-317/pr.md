Agent : Claude, agent 1 (01UY1Vv2)

**Ticket** : WFS-317 — règles : `regles-de-gestion/WFS-317.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : toujours 21 img/s en Basse (APK 908) ; avant de corriger, voir sur le téléphone le budget de chaque image (I9) : appels de dessin et triangles, par famille.
- **Ce qui change** (fichiers, comportement, à l'écran) — **partie 1 seulement (I9, comptage)**, découpage accepté par le PO : (a) I9, (b) mesure par l'utilisateur sur l'APK de `main`, (c) RG-02 dans une 2e PR :
  - **`Renderer`** :
    - chaque dessin en attente porte sa famille : décor (`draw(Mesh)`), voitures (`draw(Model)`) ou personnes (`drawSkinned`) ;
    - `flushDraw` compte les appels et les triangles de chaque famille, toutes passes (ombres, reflet, capture, scène) ;
    - `comptesImage()` rend les chiffres de l'image précédente.
  - **`qualite_systeme.cpp`** :
    - `texteDessin()` ;
    - la ligne « Rendu » (Réglages, journal) se termine par « N appels · N k triangles (décor x %, voitures y %, personnes z %) » ;
    - le fichier de mesure partageable `rendu-mesure.txt` donne le détail de chaque famille.
  - **Rien ne change au rendu** : comptage seulement, aucune allocation en course. Aucun bouton ne bouge, et le texte se réduit s'il ne tient pas.
- **Comment c'est vérifié** (tests CPU, parcours, bancs, mesures) :
  - 71 tests CPU verts, parcours complet 0 écart ;
  - captures des Réglages en 2340×1080, avant / après (mise en page du téléphone) ;
  - premier relevé sur le banc (Ville de nuit, Basse, toutes passes) : 419 appels et 684 k triangles. Les voitures en font 41 %, les personnes 40 %, le décor 20 %.
- **Limites et risques connus** :
  - Le banc tourne en rendu logiciel : seuls les chiffres du téléphone désigneront les 2 postes à corriger (RG-02, 2e PR).
  - Voitures au loin : relèvent de WFS-265 (décision du PO).
  - La ligne « Images (20 s) » des captures affiche les valeurs d'essai du banc (`WFS_MESURE_ESSAI`).

## Captures pour le lead dev (3 après)

Dossier : [`qa/pr/WFS-317`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `reglages-menu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-reglages-menu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-reglages-menu.jpg?raw=true) |
| `reglages-pause` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-reglages-pause.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-reglages-pause.jpg?raw=true) |
| `reglages` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/avant-reglages.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-reglages.jpg?raw=true) |

### Ville de nuit, Basse, 2340×1080 (banc des parties, après)

![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-317/qa/pr/WFS-317/apres-ville-nuit-basse-reglages.jpg?raw=true)

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01UY1Vv2MVfXZF3cPf1r8qRq
