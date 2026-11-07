# QA WFS-88 : iOS, démarrage jusqu'au menu (rien ne change pour Android)

## QA après fusion, recette de nuit (2026-11-07 07:11:34 UTC, qa-lead) — critères d'acceptation et jeu qui fonctionne

**Verdict : VALIDÉ** (partie Android ; partie iOS non vérifiable à cause du GEL iOS)

| Critère | Source | Constat |
|---|---|---|
| Rien ne change pour Android | diff du commit de fusion `1c28ae5` (`renderer.cpp`, `vk.cpp`) | OK : tout le code ajouté est sous `#if defined(__APPLE__)` ; hors Apple, `ETAPE_RENDU` ne fait rien et `vkCmdDrawIndexed` reçoit le même décalage de sommets (`p.vofs`) qu'avant |
| CI de `main` verte, le jeu fonctionne | runs `build.yml` de `main` depuis la fusion (dernier vert : 809, `7f344b9`) | OK : compilation, tests, APK et banc PC verts ; captures de course, de menu et de garage normales |
| iOS : démarrage jusqu'au menu | — | **Non vérifiable** : GEL iOS (règle de l'utilisateur du 2026-11-04), aucune CI iOS lancée. Ce n'est pas un motif de refus. À revoir quand le gel sera levé, avec WFS-78 (refusé le 2026-11-04 pour ce motif) |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| — | — | — | Aucune |
