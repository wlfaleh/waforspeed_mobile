Agent : Claude, agent dev3d (01Us6uCudS)

**Ticket** : WFS-508 — CGU et politique de confidentialité en anglais (épic E20) — règles : `regles-de-gestion/WFS-508.md`, `regles-de-gestion/E20-langue-anglaise.md` (RC-E20-01 à 07)

## Description

- **Demande de l'utilisateur** (en une phrase) : sur un téléphone qui n'est pas en français, tout le jeu est en anglais, y compris les conditions d'utilisation et la politique de confidentialité (épic E20, WFS-508).
- **Ce qui change** :
  - `store/cgu-en.md` et `store/confidentialite-en.md` (nouveaux) : traductions fidèles de `store/cgu.md` et `store/confidentialite.md`, même plan paragraphe par paragraphe ; en tête des 4 fichiers, une phrase dit que **la version française fait foi** (RG-02). `VERSION_CGU` reste à 1 : aucune nouvelle demande d'accord (RG-05, confirmé par le juriste).
  - Écran des CGU (`src/ui/ecrans/cgu.cpp`, `cgu.h`, `cgu_texte.cpp`) : texte long lu dans la langue active (`fichierTexteCgu`, repli sur le français si le fichier anglais manque) ; sur-titre, titre, case, boutons « Accept / Decline / Close », message du refus et « Closing the game… » passent par `ui::tr` (clés `cgu.*` dans `assets/textes/fr.txt` et `en.txt`, RC-E20-01) ; les notes des stores sont coupées aussi sous leur titre anglais (`sansNotesStore`). En français, l'écran est inchangé (mêmes textes), sauf la phrase « fait foi » en tête.
  - Banc `textes` (RG-06) : `benchCguTextes` (captures `textes-<langue>-cgu-haut / milieu / bas / lecture`, journal des textes vérifié sans phrase française, texte qui défile jusqu'en bas, aucune ligne plus large que le cadre, boutons sans chevauchement). Accroches d'1 ligne : `src/platform/desktop_main.cpp` (banc `textes`) et `android/app/build.gradle` (les 2 fichiers anglais copiés dans l'APK, assets `cgu/`), annoncées au journal.
  - `tests/test_cgu.cpp` : fichiers anglais présents, même version, phrase « fait foi » dans les 4 fichiers, **même plan que le français** (titres, sous-titres, puces), aucune phrase française dans la partie anglaise affichée, RG-05 (acceptation gardée avec `langue=en`).
- **Comment c'est vérifié** : compilation complète (wfs_bench, tests) ; **tests CPU 120/120 OK** (`bash tools/qa/tests_cpu.sh build`, log ci-dessous) ; banc `textes` **VERT** en `WFS_LANGUE=fr` et `WFS_LANGUE=en` (0 échec, 0 dépassement) ; banc `cgu` (parcours WFS-308/355) passé dans les deux langues ; captures regardées une par une.
- **Limites et risques** : la mise en ligne de la page anglaise et la fiche Google Play sont hors périmètre (WFS-509). Points relevés par le juriste hors de ce ticket, transmis au PO : contact de l'éditeur à compléter (ANO-205), lignes internes « Brouillon / Draft » et « texte à publier… » visibles dans le jeu (déjà le cas avant), droit applicable non précisé. Pas de test sur téléphone réel.

## Avis du juriste (RG-04, agent `juriste`, 2026-11-13)

> **Avis juriste : CONFORME**, avec une correction de fidélité à faire avant la fusion (faite dans le 2e commit) :
> 1. `store/cgu-en.md` : « Map packs can also be deleted… » → « Map packs **will also be able to** be deleted… » (le français dit « pourront », futur).
> 2. Facultatif : `store/confidentialite-en.md` « they never leave the device » → « they **do not** leave the device » (fait).
>
> Plan et contenu identiques ligne à ligne (CGU 42 lignes, confidentialité 34 lignes) ; boutons « Accept, Decline, I have read and I accept, The game cannot start without your consent, Closing the game… » justes et neutres. **La version française fait foi** (éditeur français, loi n° 94-665 du 4 août 1994, art. 2) ; phrases de tête des 4 fichiers correctes ; une clause ambiguë s'interprète quand même en faveur du consommateur (C. consom. art. L211-1, directive 93/13/CEE art. 5) : les deux textes doivent rester identiques. **Ne pas changer `VERSION_CGU`** (RG-05) : la phrase de tête ne change pas ce que le joueur accepte.
>
> Hors ticket, avant la sortie publique (transmis au PO) : 🔴 contact de l'éditeur à compléter (ANO-205) ; 🔴 politique publiée à une adresse publique, sans les notes internes (WFS-509) ; 🟠 lignes « Brouillon / Draft » et « texte à publier… » visibles dans le jeu (déjà avant ce ticket) ; 🟠 droit applicable non précisé.

## Tests CPU

`bash tools/qa/tests_cpu.sh build` : **tests CPU : 120 test(s) réussi(s) en 381 s** — log complet : [`tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/tests-cpu.log). `test_cgu` et `test_langue` : OK.

Bancs : [`banc-textes-fr.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/banc-textes-fr.log), [`banc-textes-en.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/banc-textes-en.log) (VERT, 0 échec, 0 dépassement), [`banc-cgu-en.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/banc-cgu-en.log) (parcours WFS-308 / 355 en anglais, 0 écart).

## Captures pour le lead dev (banc PC, `tools/qa/captures_pr.sh WFS-508 textes cgu`, `WFS_LANGUE=fr` puis `en`, filtre `*cgu*`)

Écran 2D (pas de jour / nuit). Les captures `textes-*-cgu-*` sont nouvelles (le banc `textes` de main n'avait pas l'écran des CGU) : pas d'« avant ».

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `textes-en-cgu-haut` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/apres-textes-en-cgu-haut.jpg?raw=true) |
| `textes-en-cgu-milieu` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/apres-textes-en-cgu-milieu.jpg?raw=true) |
| `textes-en-cgu-bas` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/apres-textes-en-cgu-bas.jpg?raw=true) |
| `textes-en-cgu-lecture` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/apres-textes-en-cgu-lecture.jpg?raw=true) |
| `en-cgu-premier` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/avant-en-cgu-premier.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/apres-en-cgu-premier.jpg?raw=true) |
| `en-cgu-defile` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/avant-en-cgu-defile.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/apres-en-cgu-defile.jpg?raw=true) |
| `en-cgu-coche` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/avant-en-cgu-coche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/apres-en-cgu-coche.jpg?raw=true) |
| `en-cgu-refus` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/avant-en-cgu-refus.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/apres-en-cgu-refus.jpg?raw=true) |
| `en-cgu-accepter-appuye` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/avant-en-cgu-accepter-appuye.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/apres-en-cgu-accepter-appuye.jpg?raw=true) |
| `en-reglages-cgu-lecture` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/avant-en-reglages-cgu-lecture.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/apres-en-reglages-cgu-lecture.jpg?raw=true) |
| `en-reglages-cgu-bouton` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/avant-en-reglages-cgu-bouton.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/apres-en-reglages-cgu-bouton.jpg?raw=true) |
| `textes-fr-cgu-haut` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/apres-textes-fr-cgu-haut.jpg?raw=true) |
| `textes-fr-cgu-milieu` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/apres-textes-fr-cgu-milieu.jpg?raw=true) |
| `textes-fr-cgu-bas` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/apres-textes-fr-cgu-bas.jpg?raw=true) |
| `textes-fr-cgu-lecture` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/apres-textes-fr-cgu-lecture.jpg?raw=true) |
| `fr-cgu-premier` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/avant-fr-cgu-premier.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/apres-fr-cgu-premier.jpg?raw=true) |
| `fr-cgu-defile` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/avant-fr-cgu-defile.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/apres-fr-cgu-defile.jpg?raw=true) |
| `fr-cgu-coche` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/avant-fr-cgu-coche.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/apres-fr-cgu-coche.jpg?raw=true) |
| `fr-cgu-refus` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/avant-fr-cgu-refus.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/apres-fr-cgu-refus.jpg?raw=true) |
| `fr-reglages-cgu-lecture` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/avant-fr-reglages-cgu-lecture.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-508/qa/pr/WFS-508/apres-fr-reglages-cgu-lecture.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01Us6uCudS6q2Mm6FiVxo6e7
