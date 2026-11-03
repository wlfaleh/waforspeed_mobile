# QA WFS-52 : HUD et notifications dans les marges de sécurité (encoche)

**Verdict : VALIDÉ** (banc d'essai PC), 1er passage, aucune anomalie. **Fusion autorisée.**

- Construction regardée : commit `d2b19dc` (`feature/waforspeed-52`, PR #30), CI au vert (run 41699480375 : `banc-essai`,
  `android`, `publication`). Release `apercu-waforspeed-52` (« commit d2b19dc »), 63 captures et `WaForSpeed.apk`.
- La PR se fusionne sur `main` sans conflit (GitHub : `mergeable`, `clean`). Les 10 commits de `main` après la base
  (`775e694`) ne touchent ni `src/ui/hud.cpp` ni `.github/workflows/build.yml`.
- Règles : `regles-de-gestion/WFS-52.md` (RG-01 à RG-03).
- Code changé : `src/ui/hud.cpp` (+15 / -4 : positions des 3 panneaux = `g.marges` + gouttières de WFS-27, et la capture
  `encoche-hud` dans `benchHud`) et `.github/workflows/build.yml` (1 ligne, copie `encoche-hud-2400`).

## Captures

| Capture | Constat |
|---|---|
| `encoche-hud-2400` (2400x1080) | Bande rouge de 90 px à gauche (x 0 → 89, puis la scène dès x = 90). Panneau Temps / Checkpoint : bord gauche à x ≈ 114 (90 + gouttière 16 × 1,5) ; textes « TEMPS », « 29,2 », « CHECKPOINT 1 » et barre de progression hors de la bande. Score / Combo en haut à droite à sa place de WFS-27 (marge droite 0). Vitesse / Nitro centré entre 90 et 2400 (décalé de 45 px vers la droite par rapport à `hud-2400`), « 152 », « KM/H », « NITRO » et les 10 cases lisibles. Notifications « QUASI-CONTACT » à droite, sous le score. Volant tactile décalé par la marge (WFS-51). Voiture visible, image non déformée, aucun texte ni panneau dans la bande. |
| `encoche-hud` (1280x720) | Même scène : bande de 90 px, panneau Temps à x ≈ 106 (90 + 16), rien dans la bande. |
| `hud-2400`, `hud-course`, `hud-combo`, `hud-nitro`, `hud-temps-bas`, `toast-checkpoint`, `toast-pile` | Sans encoche : panneaux aux positions de WFS-27. Comparés pixel par pixel à `apercu-waforspeed-57` (`cc76b01`, ancêtre de la base de la PR) : écart maximal 1/255, aucun pixel au-delà de 8/255 ; les zones des panneaux bas sont identiques et les zones haut gauche / haut droite ne diffèrent que de ce bruit de 1/255. Le même bruit de 1/255 existe entre deux constructions de `main` (`apercu-waforspeed-56` contre `-57`) : c'est le bruit du rendu de la CI, pas un déplacement. |
| `course-*`, `vitrine-*`, les 49 autres captures | Même comparaison : écart maximal 1/255 ou identiques au pixel (`chargement-*`, `moteur-courbe`, `scene-dediee-vide`). Aucune régression visible. |
| Tests du banc | `mode exact : 39 voitures… 0 écart(s)`, `plage`, `campagne`, `circuit` : 0 écart(s), `course automatique : 22 lignes… 0 écart(s)`. |

## Critères d'acceptation et règles

| Critère / règle | Constat | Résultat |
|---|---|---|
| `encoche-hud` (2400x1080, encoche 90 px) : aucun texte ni panneau dans la bande | panneau à x ≈ 114, bande 0 → 89 vide de HUD | OK |
| `course-*`, `hud-*` sans encoche identiques à `main`, banc `0 écart(s)` | ≤ 1/255 (bruit de la CI, présent aussi entre deux `main`), banc 0 écart | OK |
| RG-01 : position = marge + gouttière de WFS-27 | code : `mg.gauche + gl`, `W − mg.droite − gl − w`, `mg.haut + gt`, `H − mg.bas − gt − h`, centre `(mg.gauche + W − mg.droite)/2` | OK |
| RG-02 : marges 0 → rendu de WFS-27 | formules ramenées à celles de WFS-27 quand les marges valent 0 ; captures inchangées | OK |
| RG-03 : notifications de WFS-47 | `src/ui/toasts.cpp` suit déjà `g.marges.droite` et `g.marges.haut` (l. 66-67) ; la capture n'exerce que la marge gauche, les bandeaux restent à leur place | OK |

## Écart de zone (`build.yml`)

Le PO avait prévu une accroche éventuelle dans `tools/qa/**` ; le développeur a modifié 1 ligne de
`.github/workflows/build.yml` (zone E7), là où sont réellement copiées les captures 2400x1080. C'est une ligne d'accroche
(≤ 10 lignes) annoncée au Journal (entrée WFS-52), donc conforme à la règle du noyau partagé, et elle ne gêne pas WFS-58
(E7, en cours) : la PR est sans conflit. Pas d'anomalie.

Remarque (hors verdict) : `WFS_ENCOCHE` n'est pas fixée dans la CI ; la valeur par défaut de 90 px est celle du critère.

## Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| — | — | — | Aucune anomalie. |
