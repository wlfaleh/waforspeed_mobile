Agent : agent dev-artiste-3d (01Us6uCtWv) (session_01Us6uCtWvQUPEnCqGCejEnn)

## Agent : agent dev-artiste-3d (01Us6uCtWv) (session_01Us6uCtWvQUPEnCqGCejEnn)

**Ticket** : WFS-504 — règles : `regles-de-gestion/WFS-504.md`

## Description (à remplir par le développeur avant d'ouvrir la PR)

- **Demande de l'utilisateur** (en une phrase) : le jeu parle français sur un téléphone en français et anglais sur tous les autres, avec un réglage « Langue » (Automatique / Français / English) ; ce ticket pose le système et traduit l'écran Réglages.
- **Ce qui change** (fichiers, comportement, à l'écran) :
  - `src/ui/langue.h/.cpp` (nouveau) : `ui::tr("cle")`, `ui::tr("cle", v1, v2)` ({1}, {2}), langue du téléphone (RG-01), choix Automatique / Français / English appliqué tout de suite (RG-03), journal des textes du banc (RG-07, RG-08) ; `src/ui/langue_regles.h` (nouveau) : règles en calcul pur (détection `fr`, lecture des tables, garde-fou « phrase française »), testées ;
  - `assets/textes/fr.txt`, `assets/textes/en.txt` (nouveaux) : 71 textes chacun (Réglages, vues de la caméra, menu caché de diagnostic), lus une fois avec la police (écran de chargement), jamais en course ;
  - `src/ui/text.*` : `ui::textBoite` (police réduite de 20 % au plus, sinon dépassement signalé au banc) ; `ui::text` note chaque texte en mode banc ;
  - `src/ui/ecrans/reglages*` : tous les textes passent par `ui::tr` ; nouvelle ligne « Langue » en dernière ligne d'Affichage (`L_LANGUE`) ; sur téléphone, Affichage passe en 4 + 3 lignes (Conduite et Son inchangés) ; les boutons « Réglages », « Fermer », « Conditions d'utilisation » gardent la largeur du texte le plus long des deux langues (aucun bouton ne change de taille, le français est identique au pixel) ;
  - `src/game/sauvegarde.*` : clé `langue=auto|fr|en` (absente ou inconnue → auto ; aucune clé supprimée) ;
  - Android : `JeuActivity.onCreate` donne `Locale.getDefault().getLanguage()` au natif (`nativeLangue`, avant le thread du jeu) ; `values/strings.xml` en anglais, `values-fr/strings.xml` en français (boîte « Le jeu s'est fermé la dernière fois », RG-11) ;
  - banc PC : `WFS_LANGUE=fr|en` (sans elle : français, captures existantes inchangées) ; `WFS_BANC=textes` (accroche d'une ligne dans `desktop_main.cpp`).
- **Comment c'est vérifié** (tests CPU, parcours, bancs, mesures) :
  - `tests/test_langue.cpp` (RG-06) : mêmes clés dans les deux tables, aucun texte vide, mêmes {n}, aucune phrase française dans `en.txt`, RG-01 sur `fr`, `fr-CA`, `fr-MA`, `fr_BE`, `en`, `en-GB`, `ar`, `es`, `de`, `""`, sauvegarde sans `langue` → auto ; `test_reglages_onglets` mis à jour (7 lignes dans Affichage, « Langue » en dernier) ; **116 tests CPU au vert** (`tools/qa/tests_cpu.sh build`) ;
  - banc `WFS_BANC=textes` **VERT** en anglais (1280x720 et 2400x1080) et en français : 0 phrase française, 0 texte qui dépasse sur les 3 onglets (menu et pause) ; parcours complet OK : accueil → Réglages → Langue = English (panneau aussitôt en anglais) → retour → course → pause → Réglages en anglais → Automatique ; « English » gardé après un redémarrage simulé (fichier écrit puis relu). Extraits : [`journal-textes.md`](journal-textes.md) ;
  - captures « avant / après » en français (`reglages-onglets`, `WFS_LANGUE=fr`) : seule différence, la ligne « Langue » ; captures anglaises `apres-textes-en-*` (PC) et `apres-telephone-textes-en-*` (2400x1080).
- **Limites et risques connus** :
  - la **ligne technique de mesure** en bas du panneau (« Rendu : … · Mémoire : … ») reste en français : ses textes viennent de `src/render/qualite_systeme.cpp` et `src/ui/ecrans/chargement.cpp`, hors de la zone de ce ticket ; le banc la note à part (« ligne technique non vérifiée ») sans échec ; signalé au PO (à traduire par WFS-505 ou un ticket à part) ; les libellés des interrupteurs du menu caché (WFS-419, `src/render/diagnostic_perf.*`) aussi ;
  - le reste du menu, la pause et la course restent en français : WFS-505 à 508 (hors périmètre) ;
  - non vérifié sur un vrai téléphone (lecture de la langue par JNI compilée seulement par la CI Android) ; les captures ont été faites au commit `a9638ffd`, le commit suivant ne change que la lecture de la langue du téléphone (valeur atomique au lieu d'un verrou à chaque texte), banc `textes` refait : vert.

## Tests CPU

`bash tools/qa/tests_cpu.sh build` : **116 test(s) réussi(s)** (dont `test_langue`, `test_reglages_onglets`), commit testé `d8f997a1` ; journal complet : [`tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/tests-cpu.log).

```
[OK] test_ville_sol (4 s)
[OK] test_virages_fleches (0 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 116 test(s) réussi(s) en 461 s
```

## Téléphone 2400x1080, en français : avant (main) / après (branche) — revue #372, point 2

Sur téléphone, l'onglet Affichage passe de 3 + 3 à 4 + 3 lignes parce qu'il reçoit la 7e ligne « Langue » demandée par RG-02 (la place de 6 rangs du panneau ne change pas). Seul déplacement : « Réduire les mouvements » passe du haut de la 2e colonne au 4e rang de la 1re ; toutes les autres lignes, les boutons et leurs tailles sont inchangés. Conduite et Son : identiques (3 + 3 et 2). Au menu comme en pause, le panneau est le même (seul le surtitre change) : l'Affichage en pause sur téléphone est la capture `textes-fr-reglages-pause` (après), main ne capture pas cet onglet en pause.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| Affichage, menu | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/avant-telephone-reglages-onglet-affichage-menu.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-telephone-reglages-onglet-affichage-menu.jpg?raw=true) |
| Affichage, pause | — | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-textes-fr-reglages-pause.jpg?raw=true) |
| Conduite, pause | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/avant-telephone-reglages-onglet-conduite-pause.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-telephone-reglages-onglet-conduite-pause.jpg?raw=true) |
| Son, pause | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/avant-telephone-reglages-onglet-son-pause.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-telephone-reglages-onglet-son-pause.jpg?raw=true) |
| Son, gaucher | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/avant-telephone-reglages-onglet-son-gaucher.jpg?raw=true) | ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-telephone-reglages-onglet-son-gaucher.jpg?raw=true) |

## Captures anglaises (après seulement : l'anglais n'existe pas sur main)

- `apres-textes-en-reglages-affichage` : ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-textes-en-reglages-affichage.jpg?raw=true)
- `apres-textes-en-reglages-conduite` : ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-textes-en-reglages-conduite.jpg?raw=true)
- `apres-textes-en-reglages-son` : ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-textes-en-reglages-son.jpg?raw=true)
- `apres-textes-en-reglages-pause` : ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-textes-en-reglages-pause.jpg?raw=true)
- `apres-textes-en-menu` : ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-textes-en-menu.jpg?raw=true)
- `apres-telephone-textes-en-reglages-affichage` : ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-telephone-textes-en-reglages-affichage.jpg?raw=true)
- `apres-telephone-textes-en-reglages-conduite` : ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-telephone-textes-en-reglages-conduite.jpg?raw=true)
- `apres-telephone-textes-en-reglages-son` : ![](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-telephone-textes-en-reglages-son.jpg?raw=true)

## Captures pour le lead dev (12 après)

Dossier : [`qa/pr/WFS-504`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `reglages-onglet-affichage-menu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/avant-reglages-onglet-affichage-menu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-reglages-onglet-affichage-menu.jpg?raw=true) |
| `reglages-onglet-conduite-pause` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/avant-reglages-onglet-conduite-pause.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-reglages-onglet-conduite-pause.jpg?raw=true) |
| `reglages-onglet-son-gaucher` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/avant-reglages-onglet-son-gaucher.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-reglages-onglet-son-gaucher.jpg?raw=true) |
| `reglages-onglet-son-pause` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/avant-reglages-onglet-son-pause.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-reglages-onglet-son-pause.jpg?raw=true) |
| `textes-fr-*` ci-dessous : téléphone 2400x1080, `WFS_LANGUE=fr` (le parcours passe par English puis revient en Automatique) | | |
| `textes-fr-menu` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-textes-fr-menu.jpg?raw=true) |
| `textes-fr-parcours-1-english` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-textes-fr-parcours-1-english.jpg?raw=true) |
| `textes-fr-parcours-2-pause-english` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-textes-fr-parcours-2-pause-english.jpg?raw=true) |
| `textes-fr-parcours-3-auto` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-textes-fr-parcours-3-auto.jpg?raw=true) |
| `textes-fr-reglages-affichage` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-textes-fr-reglages-affichage.jpg?raw=true) |
| `textes-fr-reglages-conduite` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-textes-fr-reglages-conduite.jpg?raw=true) |
| `textes-fr-reglages-pause` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-textes-fr-reglages-pause.jpg?raw=true) |
| `textes-fr-reglages-son` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-504/qa/pr/WFS-504/apres-textes-fr-reglages-son.jpg?raw=true) |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01Us6uCtWvQUPEnCqGCejEnn
