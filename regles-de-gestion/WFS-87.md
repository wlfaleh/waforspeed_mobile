# WFS-87 — [URGENT] CI : l'APK de `main` toujours publié (Release `apercu` fiable)
Épic : E7 · Priorité : P1 Urgent · Dépend de : WFS-69 ✅ · Taille : ½ à 1 jour · Exception : 2e ticket E7 en parallèle de WFS-78 (fichiers séparés : WFS-78 ne touche que `ios.yml`, `ios/**`, `ios_main.mm`)

## Besoin (qui, quoi, pourquoi)
L'utilisateur teste lui-même l'APK de `main` sur son téléphone en le téléchargeant depuis la Release `apercu`.
Depuis le 2026-11-04 vers 13:30 UTC, le job `publication` de `build.yml` échoue sur `main` (runs 41839138847 et
41842506874 : `release not found` puis `HTTP 403` en créant la Release) alors que compilation, APK et bancs sont verts.
Le script **supprime d'abord** la Release `apercu`, puis la recrée avec ≈ 212 fichiers : si la création échoue, il
**n'y a plus aucun APK à télécharger** (constaté le 2026-11-04 14:18 : `releases/tags/apercu` → 404).
Cause probable : limite secondaire de l'API GitHub (≈ 212 téléversements par construction, plusieurs constructions
en parallèle, 74 Releases d'aperçu de branches). Le QA lit les captures une par une à l'adresse
`releases/download/apercu[-waforspeed-XX]/<nom>.png` : ce format d'adresse doit rester.

## Règles de gestion
- RG-01 : **jamais de trou** : on ne supprime plus la Release avant d'avoir le nouvel APK en ligne. Méthode libre
  (par exemple : téléverser dans la Release existante avec `--clobber` puis déplacer le tag sur le nouveau commit ;
  ou créer sous un tag temporaire puis basculer). À tout instant, `releases/download/apercu/WaForSpeed.apk` répond.
- RG-02 : **l'APK d'abord** : `WaForSpeed.apk` (et `.aab` s'il existe) est publié en premier ; les captures ensuite.
- RG-03 : **nouvelles tentatives** : chaque appel à l'API (vue, création, téléversement, déplacement du tag) est
  retenté jusqu'à 4 fois avec attente croissante (15 s, 30 s, 60 s, 120 s), surtout sur 403 / 429 / 5xx.
- RG-04 : **moins d'appels** : ne téléverser que les captures non vides (déjà le cas) et, si possible, par lots ;
  pas d'appel inutile (une seule lecture de la Release au début).
- RG-05 : la règle « ne jamais remplacer une Release faite d'un commit plus récent » (builds de `main` en parallèle)
  est conservée.
- RG-06 : si l'APK ne peut pas être publié après les tentatives, le job reste **rouge** ; si seules quelques captures
  échouent, le job est vert avec un avertissement (`::warning::`) listant les captures manquantes.
- RG-07 : le script de publication est sorti du YAML dans `tools/qa/publier_apercu.sh` (lisible, testable en local avec
  `bash -n` et `shellcheck` si disponible) ; `build.yml` l'appelle. Même script pour `main` et les branches `feature/*`.
- RG-08 : le titre et les notes de la Release restent les mêmes (numéro de construction, branche, commit).

## À réutiliser
Rien à réutiliser du jeu web (`wlfaleh/waaps` n'a pas de CI Android). Réutiliser le job `publication` actuel de
`.github/workflows/build.yml` (calcul du tag, garde « commit plus récent », liste des fichiers) et le découpage de WFS-69.

## Critères d'acceptation
- [ ] Après fusion, la construction de `main` a un job `publication` **vert** et `releases/download/apercu/WaForSpeed.apk` répond (200)
- [ ] La Release `apercu` pointe sur le dernier commit de `main` ; les captures du banc y sont, à la même adresse qu'avant
- [ ] Journal du job : on voit l'APK publié avant les captures et, le cas échéant, les nouvelles tentatives
- [ ] Une branche `feature/*` publie toujours `apercu-waforspeed-XX` (vérifié sur la CI de la branche)

## Hors périmètre
Supprimer les anciennes Releases d'aperçu des branches (action irréversible : à proposer à l'utilisateur au Journal,
pas à faire) ; `simulateur.yml` ; iOS (`ios.yml`, WFS-78).

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| P1 Urgent | WFS-87 | [URGENT] CI : l'APK de `main` toujours publié (Release `apercu` fiable, plus de trou ni d'échec 403) | E7 (exception : 2e ticket E7, fichiers séparés de WFS-78) | ✅ Fusionné par l'utilisateur (PR #73, QA de nuit à faire) | Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04 | WFS-69 ✅ | `feature/waforspeed-87` | [#73](https://github.com/wlfaleh/waforspeed_mobile/pull/73) |  |

Critères et notes de suivi :

**WFS-87 — [URGENT] CI : APK de `main` toujours publié** · E7 · P1 Urgent · ✅ Fusionné par l'utilisateur (PR #73, 3b62e5f ; CI de main verte, run 41860181020) — Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04, branche `feature/waforspeed-87` (règles : `regles-de-gestion/WFS-87.md`). Zone : `.github/workflows/build.yml` (job `publication`), `tools/qa/publier_apercu.sh` ; ne pas toucher `ios.yml`, `ios/**` (WFS-78).
- [ ] CI de `main` après fusion : job `publication` vert, `releases/download/apercu/WaForSpeed.apk` répond
- [ ] Release `apercu` sur le dernier commit, captures à la même adresse ; jamais de moment sans APK (plus de suppression avant création)
- [ ] branche `feature/*` : `apercu-waforspeed-87` publiée
