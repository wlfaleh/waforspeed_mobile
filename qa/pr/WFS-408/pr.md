## Agent : Claude, agent juriste (01UY1Vv2)

**Ticket** : WFS-408 — règles : `regles-de-gestion/WFS-408.md`

## Description

- **Demande de l'utilisateur** : « Tu doit corriger les autres anomalies » (après l'avis juridique du 2026-11-18), avec ses décisions « 1re version avec achats » et « tout en place pour gagner l'argent » : documents légaux en version **achats + pub** (WFS-408 élargi par le PO, RG-E1 à RG-E5).
- **Ce qui change** (seulement `store/**`, aucun code : l'écran des CGU lit directement `store/cgu*.md` et `store/confidentialite*.md`) :
  - `store/cgu.md`, `store/cgu-en.md` : plus de « sans achat » ; nouvelles parties Éditeur, Achats intégrés (étoiles sans valeur hors du jeu, Google Play, prix TTC, renonciation à la rétractation L221-28 13°, remboursements, mineurs), Vidéos publicitaires (AdMob, facultatives, consentement), Duel en ligne (règles du pseudo, exclusion), Responsabilité « dans la limite permise par la loi », Droit applicable (droit français + protections du pays du consommateur).
  - `store/confidentialite.md`, `-en.md` : responsable, données gardées / données qui quittent le téléphone (relais Cloudflare, GitHub, Google Play, AdMob et identifiant publicitaire), bases légales (RGPD 6.1.b, 6.1.a + art. 82 LIL), destinataires, transferts hors UE, durées, mineurs, droits, CNIL ; les notes des stores renvoient au nouveau `store/play-console-declarations.md`.
  - `store/play-console-declarations.md` (nouveau) : réponses préparées pour la Console (contenu, Sécurité des données, IARC, public cible en 2 variantes), chaque réponse avec sa source.
  - `store/fiche-fr.md` : « 17 voitures inspirées de la route », Corniche seule (WFS-405), achats et annonces annoncés, plus de licence NC citée. `store/checklist.md` à jour.
  - Valeurs non données par l'utilisateur : repères uniques `[À REMPLIR PAR L'UTILISATEUR : nom de l'éditeur | e-mail de contact | adresse publique de la politique]` (EN : `[TO BE FILLED IN BY THE PUBLISHER: …]`), aucune valeur inventée ; ils sont **visibles dans l'écran des CGU** (RG-E2 de l'élargissement, qui prime sur RG-00).
- **Comment c'est vérifié** : `test_cgu` OK (même plan FR / EN, aucune phrase française dans l'anglais, version = `VERSION_CGU`, mentions obligatoires) ; `verifier_licences.py` : 0 manque ; `grep` : plus de « sans achat », « 20 vraies voitures », « no in-game purchases » dans `store/` ; banc `cgu` (parcours premier lancement, refus, Réglages → lecture) et banc `textes` (CGU FR et EN) capturés avant / après.
- **Limites et risques connus** :
  - Le texte décrit la **pub**, qui n'existe pas encore dans le jeu (WFS-512, 514) : ne pas publier avant. Libellé EN « Ad privacy » de la ligne des Réglages supposé (WFS-514 le fixera).
  - **Renonciation expresse** à la rétractation : le texte ne suffit pas, l'écran Market doit la faire confirmer au moment de l'achat (code hors zone : signalé au PO).
  - `VERSION_CGU` reste à 1 (`cgu.h` hors zone) : à passer à 2 le jour de la publication pour faire réaccepter les testeurs (signalé au PO).
  - Chiffrement du relais du duel (`wss://`) non vérifié : noté dans `play-console-declarations.md`.
  - Relecture d'un avocat avant la vente (WFS-516).

## Tests CPU

`bash tools/qa/tests_cpu.sh build` sur la branche synchronisée avec `main` : **154 test(s) réussi(s) en 384 s**, 0 échec (dont `test_cgu`, `test_voitures_retirees`). Journal complet : [`qa/pr/WFS-408/tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/tests-cpu.log).

## Captures pour le lead dev (9 + 8 après)

Dossier : [`qa/pr/WFS-408`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408) — produit par `tools/qa/captures_pr.sh`.

Captures « avant » : `origin/main` = `539399fc1f198447b431d0f06ee202d2f5adf18f` ; cache : 0 banc(s) relu(s), 1 banc(s) rejoué(s) ; main compilé : oui.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `cgu-accepter-appuye` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-cgu-accepter-appuye.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-cgu-accepter-appuye.jpg?raw=true) |
| `cgu-coche` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-cgu-coche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-cgu-coche.jpg?raw=true) |
| `cgu-defile` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-cgu-defile.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-cgu-defile.jpg?raw=true) |
| `cgu-premier` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-cgu-premier.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-cgu-premier.jpg?raw=true) |
| `cgu-refus` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-cgu-refus.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-cgu-refus.jpg?raw=true) |
| `cgu-reprise` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-cgu-reprise.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-cgu-reprise.jpg?raw=true) |
| `cgu-retour-jeu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-cgu-retour-jeu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-cgu-retour-jeu.jpg?raw=true) |
| `reglages-cgu-bouton` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-reglages-cgu-bouton.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-reglages-cgu-bouton.jpg?raw=true) |
| `reglages-cgu-lecture` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-reglages-cgu-lecture.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-reglages-cgu-lecture.jpg?raw=true) |
| `textes-fr-cgu-haut` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-textes-fr-cgu-haut.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-textes-fr-cgu-haut.jpg?raw=true) |
| `textes-fr-cgu-milieu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-textes-fr-cgu-milieu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-textes-fr-cgu-milieu.jpg?raw=true) |
| `textes-fr-cgu-bas` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-textes-fr-cgu-bas.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-textes-fr-cgu-bas.jpg?raw=true) |
| `textes-fr-cgu-lecture` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-textes-fr-cgu-lecture.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-textes-fr-cgu-lecture.jpg?raw=true) |
| `textes-en-cgu-haut` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-textes-en-cgu-haut.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-textes-en-cgu-haut.jpg?raw=true) |
| `textes-en-cgu-milieu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-textes-en-cgu-milieu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-textes-en-cgu-milieu.jpg?raw=true) |
| `textes-en-cgu-bas` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-textes-en-cgu-bas.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-textes-en-cgu-bas.jpg?raw=true) |
| `textes-en-cgu-lecture` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-textes-en-cgu-lecture.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-textes-en-cgu-lecture.jpg?raw=true) |

### Captures ciblées (2e revue)

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `cible-fr-cgu-p30` (CGU : achats, rétractation) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-cible-fr-cgu-p30.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-cible-fr-cgu-p30.jpg?raw=true) |
| `cible-fr-cgu-p60` (CGU : duel en ligne) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-cible-fr-cgu-p60.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-cible-fr-cgu-p60.jpg?raw=true) |
| `cible-fr-conf-p00` (confidentialité : responsable, adresse) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-cible-fr-conf-p00.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-cible-fr-conf-p00.jpg?raw=true) |
| `cible-fr-conf-p40` (données qui quittent le téléphone) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-cible-fr-conf-p40.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-cible-fr-conf-p40.jpg?raw=true) |
| `cible-fr-conf-p60` (pourquoi, sur quelle base) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-cible-fr-conf-p60.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-cible-fr-conf-p60.jpg?raw=true) |
| `cible-en-cgu-p30` (CGU : achats, rétractation) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-cible-en-cgu-p30.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-cible-en-cgu-p30.jpg?raw=true) |
| `cible-en-cgu-p60` (CGU : duel en ligne) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-cible-en-cgu-p60.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-cible-en-cgu-p60.jpg?raw=true) |
| `cible-en-conf-p00` (confidentialité : responsable, adresse) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-cible-en-conf-p00.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-cible-en-conf-p00.jpg?raw=true) |
| `cible-en-conf-p40` (données qui quittent le téléphone) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-cible-en-conf-p40.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-cible-en-conf-p40.jpg?raw=true) |
| `cible-en-conf-p60` (pourquoi, sur quelle base) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/avant-cible-en-conf-p60.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-408/qa/pr/WFS-408/apres-cible-en-conf-p60.jpg?raw=true) |
