Agent : agent arwa (Claude, session trafic)

**Ticket** : WFS-284 — règles : `regles-de-gestion/WFS-284.md`

## Description (à remplir par le développeur avant d'ouvrir la PR)

- **Demande de l'utilisateur** (en une phrase) : ne plus jamais voir « WaForSpeed isn't responding » pendant un chargement (ANO-153, VABF du 2026-11-07).
- **Cause** : le fil du jeu (`android_main`) ne lit la file des entrées Android qu'entre deux images ; un chargement le bloque d'un seul tenant (émulateur, `emu-logcat.txt` : « Construction de la route 47,62 s », « Construction de la ville 10,13 s », sans aucun rappel de progression entre deux) et une image du GPU logiciel dure 3 à 10 s (0,1-0,3 img/s) : un appui reste > 5 s sans réponse → ANR. Rappeler `Chargement::progression` plus souvent n'aurait pas suffi (étapes de 47 s, code de chargement hors zone).
- **Ce qui change** : `src/platform/android_main.cpp` seul (aucune accroche dans `chargement.*`). Un petit fil « veilleur des entrées » lit la file à la place du fil du jeu quand celui-ci est occupé : pendant tout chargement (premier, changement de carte, « Réessayer ») toutes les 50 ms, appuis et Retour pris et ignorés (RG-01, RG-03 : rien n'est rejoué), volume et autres touches laissés à Android ; hors chargement, seulement si le fil du jeu n'a pas lu la file depuis 1 s (jamais à 30-60 img/s) : appuis et Retour gardés et rejoués dans l'ordre au début de l'image suivante. Exclusion par verrou (fil du jeu autour de `process_input`) et `a->mutex` de la colle (changement de file). Fin de chaque chargement : ligne logcat `chargement : entrées lues, plus long écart N ms (K événements ignorés)` (RG-05). Rien ne change à l'écran (RG-04).
- **Comportement Android choisi (RG-02)** : un fil à part, que la RG n'autorise que s'il est indispensable : c'est le cas, vu les étapes de 47 s. Annoncé au journal pour le PO.
- **Comment c'est vérifié** : CI de branche `build.yml` run 42431712817 (une seule) : compilation PC, **android (APK) verte**, publication de l'APK verte ; tests CPU locaux `tools/qa/tests_cpu.sh` : 62 réussis ; banc PC « chargement » avant / après : 5 captures identiques au pixel près (`ImageChops.difference` vide), `android_main.cpp` n'étant pas compilé sur PC ; code du veilleur vérifié à part avec g++ et clang++ `-Wall -Wextra` (en-têtes NDK simulés) : sans avertissement.
- **Limites et risques connus** : pas d'émulateur ni de NDK ici : la preuve « aucun ANR » et la ligne RG-05 (N attendu ≈ 50-100 ms) viendront des **bancs du matin** (`matin.yml`, émulateur, parcours mvp jusqu'à course-solo) sur `main` ; à regarder par la QA. Hors chargement, si une image dure > 1 s, une commande de manette arrivée pendant ce temps est laissée à Android, et un Retour « laissé à Android » (menu principal) reste sans effet. Une pause / un écran éteint **pendant** le chargement bloque toujours le fil Java de la colle jusqu'à la fin du chargement (cycle de vie, WFS-285).

Captures demandées (RG) : 1. écran de chargement pendant le chargement de la Ville (`chargement-1`) ; 2. fin du chargement, fondu sur la scène (`chargement-fondu`, pas d'écran ouvert par un appui) ; 3. écran d'erreur « Réessayer » (`chargement-erreur`). Menu et course solo à l'émulateur : bancs du matin (le banc PC ne passe pas par `android_main`).

## Captures pour le lead dev (5 après)

Dossier : [`qa/pr/WFS-284`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-284/qa/pr/WFS-284) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `chargement-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-284/qa/pr/WFS-284/avant-chargement-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-284/qa/pr/WFS-284/apres-chargement-0.jpg?raw=true) |
| `chargement-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-284/qa/pr/WFS-284/avant-chargement-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-284/qa/pr/WFS-284/apres-chargement-1.jpg?raw=true) |
| `chargement-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-284/qa/pr/WFS-284/avant-chargement-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-284/qa/pr/WFS-284/apres-chargement-2.jpg?raw=true) |
| `chargement-erreur` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-284/qa/pr/WFS-284/avant-chargement-erreur.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-284/qa/pr/WFS-284/apres-chargement-erreur.jpg?raw=true) |
| `chargement-fondu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-284/qa/pr/WFS-284/avant-chargement-fondu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-284/qa/pr/WFS-284/apres-chargement-fondu.jpg?raw=true) |
