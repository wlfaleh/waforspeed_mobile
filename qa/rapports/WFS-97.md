# QA WFS-97 : nuit : piste du circuit éclairée, façades sombres (ANO-37)

## QA après fusion, recette de nuit (2026-11-05 19:05:21 UTC, qa-lead, accent qualité, design et parcours sur téléphone Android)

**Verdict : PASSABLE AVEC ANOMALIES MINEURES**

- Sources : Release `apercu` de `main` (construction 649, commit `894208c`, run 42083700609 vert au 2e essai ; 279/279 captures à jour), journaux des jobs de ce run, APK de la Release, compilation et 25 tests CPU en local sur `origin/main` (`7e6414b`).
- Fusion par confiance : recette faite après la fusion ; une anomalie mineure va au tableau `anno.md`, une bloquante deviendrait un ticket de correction prioritaire (PO).
- ANO-37 : ambiance vérifiée ; le reste (fond de rue, fenêtres de nuit) est déjà planifié.

### Points vérifiés

| Point | Capture / preuve | Constat |
|---|---|---|
| Piste du circuit éclairée la nuit | `carte-circuit-nuit`, `course-nuit-circuit-0`, `-haut` | OK : asphalte gris éclairé, taches sous les mâts |
| Façades sombres en ville, plus de lavande ni de rose | `course-nuit-ville-*`, `nuit-trafic-arriere`, `nuit-plage-corniche` | OK, nettement plus sombres ; reste le fond de rue clair (limite connue, RG-06) |
| Néons, flaques et reflets visibles | mêmes captures | OK, mais voir ANO-66 |
| Jour du circuit : asphalte éclairé, ombre de la voiture | `carte-circuit-jour` | OK |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-66 | Mineure | `nuit-plage-corniche`, `nuit-trafic-arriere` | Le reflet de l'enseigne « WAFORSPEED » sur la route mouillée est net et rectangulaire : on dirait un panneau posé à plat sur la chaussée devant la voiture, pas un reflet. **Correction** : étirer le reflet en hauteur (×3), le flouter (mip plus bas, rugosité ≈ 0,35) et l'atténuer avec la distance (zone rendu, WFS-109) |

### À essayer sur le téléphone (APK de `main`, construction 649 ou plus)

- [ ] Rouler de nuit en ville et sur le circuit : on doit voir la route sans que l'image soit trop claire.
