Agent : Claude, agent dev (01UY1Vv5vk)

**Ticket** : WFS-406 — règles : `regles-de-gestion/WFS-406.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : préparer la sortie grand public (ordre du 2026-11-10 13:06 UTC, rapport du juriste J-03, J-06, J-13) : un écran « Crédits » complet sans marque et des noms de voitures sans ressemblance avec les vrais.
- **Ce qui change** (fichiers, comportement, à l'écran) :
  - **Écran Crédits** (`src/ui/credits.*`, `credits_texte.cpp`, système 470) : bouton « CRÉDITS » en bas à droite du menu, style du bouton « Réglages » (`menu.cpp`, accroche E8, aucun bouton existant changé) ; panneau plein écran, texte défilant au doigt avec ascenseur, « Retour » orange qui ramène au menu. Contenu lu dans `licences/CREDITS.txt` de l'APK (une seule source).
  - **Crédits (RG-02, RG-03, RG-07)** : `tools/qa/verifier_licences.py` fabrique `assets/licences/CREDITS.txt` et `store/credits.md` sans titre d'origine ni marque : 17 voitures (noms inventés, auteur, licence, lien), 3 deux-roues, personnes, animaux (texte MIT du goéland), routes, sons (« gros » : titre et auteurs Mike Koenig, Stephan, SpeedJunkie), police, stb, android_native_app_glue avec le texte complet de la licence Apache 2.0 (`assets/licences/apache-2.0.txt`, neuf). Le script signale tout actif livré sans crédit et toute marque ou modèle de `cars.h` ou des deux-roues dans les crédits.
  - **Noms (RG-01, liste du PO)** : golf Norvik / Lume, m4 Draven / Vantor GT, giulia Alvar / Selene TS (`configgame.json`, noms par défaut de `configgame.h`, README).
  - **RG-06** : champ neuf `confidentialite_url` (vide par défaut, https seulement) ; vide → aucune ligne de confidentialité à l'écran.
  - Accroches hors zone (≤ 3 lignes chacune, au journal) : `configgame.h/.cpp`, `desktop_main.cpp` (banc `credits`), `CMakeLists.txt` (`test_credits`).
- **Comment c'est vérifié** (tests CPU, parcours, bancs, mesures) : `python3 tools/qa/verifier_licences.py` vert (17 voitures, 3 retirées, 140 actifs, 0 manque ; essai avec « Volkswagen Golf, Kawasaki Ninja, GT86 » ajoutés → 5 marques signalées) ; tests CPU 100/101 (`test_credits` et `test_configgame` compris ; le seul rouge est le rouge connu de WFS-404, voir limites) ; banc `WFS_BANC=credits` 0 erreur : accueil → menu → Crédits → glisser 300 px (le texte suit) → fin au doigt → « Retour » → menu ; garage avec les 3 noms. `grep` des anciens noms dans le code et les actifs : 0.
- **Limites et risques connus** : (1) `test_course5_ordinateurs` rouge (circuit 3/8 dans les 3 premiers) : rouge connu de WFS-404 (RG-11), donné à WFS-394, sans lien avec ce ticket. (2) Version de la licence CC BY de nhaudio non trouvée (source lue : « CC-BY » sans version ; freesound.org non autorisé) : écrite « non indiquée par la source », jamais inventée. (3) Nouveaux noms non vérifiés dans une base de marques (aucune accessible). (4) Captures « avant » du banc `credits` absentes : l'écran n'existe pas sur `main` ; l'avant / après des noms est dans `garage-0` et `garage-m4`.

## Captures pour le lead dev (24 après)

Dossier : [`qa/pr/WFS-406`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `credits-debut` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/apres-credits-debut.jpg?raw=true) |
| `credits-deux-roues` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/apres-credits-deux-roues.jpg?raw=true) |
| `credits-fin` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/apres-credits-fin.jpg?raw=true) |
| `credits-garage-giulia` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/apres-credits-garage-giulia.jpg?raw=true) |
| `credits-garage-golf` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/apres-credits-garage-golf.jpg?raw=true) |
| `credits-garage-m4` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/apres-credits-garage-m4.jpg?raw=true) |
| `credits-menu` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/apres-credits-menu.jpg?raw=true) |
| `credits-milieu` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/apres-credits-milieu.jpg?raw=true) |
| `credits-native-app-glue` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/apres-credits-native-app-glue.jpg?raw=true) |
| `credits-retour-menu` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/apres-credits-retour-menu.jpg?raw=true) |
| `course-p911-gris` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/avant-course-p911-gris.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/apres-course-p911-gris.jpg?raw=true) |
| `garage-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/avant-garage-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/apres-garage-0.jpg?raw=true) |
| `garage-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/avant-garage-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/apres-garage-1.jpg?raw=true) |
| `garage-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/avant-garage-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/apres-garage-2.jpg?raw=true) |
| `garage-course` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/avant-garage-course.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/apres-garage-course.jpg?raw=true) |
| `garage-m4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/avant-garage-m4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/apres-garage-m4.jpg?raw=true) |
| `garage-peinture-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/avant-garage-peinture-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/apres-garage-peinture-0.jpg?raw=true) |
| `garage-peinture-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/avant-garage-peinture-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/apres-garage-peinture-1.jpg?raw=true) |
| `garage-peintures-p911` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/avant-garage-peintures-p911.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/apres-garage-peintures-p911.jpg?raw=true) |
| `garage-peintures` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/avant-garage-peintures.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/apres-garage-peintures.jpg?raw=true) |
| `garage-suivant` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/avant-garage-suivant.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/apres-garage-suivant.jpg?raw=true) |
| `showroom-suivante-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/avant-showroom-suivante-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/apres-showroom-suivante-1.jpg?raw=true) |
| `showroom-suivante-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/avant-showroom-suivante-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/apres-showroom-suivante-2.jpg?raw=true) |
| `showroom-suivante-3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/avant-showroom-suivante-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-406/qa/pr/WFS-406/apres-showroom-suivante-3.jpg?raw=true) |
