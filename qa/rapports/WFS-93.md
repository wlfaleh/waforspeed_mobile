# QA WFS-93 : ville en quartiers, panneaux du circuit, vignes

## QA après fusion, recette de nuit (2026-11-05 19:05:21 UTC, qa-lead, accent qualité, design et parcours sur téléphone Android)

**Verdict : PASSABLE AVEC ANOMALIES MINEURES**

- Sources : Release `apercu` de `main` (construction 649, commit `894208c`, run 42083700609 vert au 2e essai ; 279/279 captures à jour), journaux des jobs de ce run, APK de la Release, compilation et 25 tests CPU en local sur `origin/main` (`7e6414b`).
- Fusion par confiance : recette faite après la fusion ; une anomalie mineure va au tableau `anno.md`, une bloquante deviendrait un ticket de correction prioritaire (PO).
- ANO-49 (bande d'herbe) et ANO-51 (textes morcelés de loin) sont toujours là, déjà planifiées dans WFS-94.

### Points vérifiés

| Point | Capture / preuve | Constat |
|---|---|---|
| Quartier résidentiel | `carte-ville-residentiel`, `course-cam1-*` | OK : immeubles bas à balcons, arbres, terrasses |
| Parc | `carte-ville-parc` | OK : grille, pelouse, arbres, bancs ; immeubles de l'autre côté |
| Port | `carte-ville-port` | OK : entrepôts, grillage ; conteneurs derrière les entrepôts comme au web |
| Textes du circuit lisibles | `carte-circuit-jour`, `-nuit` | OK de près ; morcelés de loin (ANO-51, déjà planifiée) |
| Rangs de vigne fins, qui suivent la pente, sans marches | `carte-campagne-vignes` | **Non** : voir ANO-65 |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-65 | Mineure | `carte-campagne-vignes` | Les rangs de vigne sont faits de tronçons épais (≈ 1 m), chacun posé à sa propre hauteur, avec des trous entre eux : on voit encore un escalier de blocs verts, pas des rangs fins continus. **Correction** : un rang = une seule bande continue qui suit le relief (hauteur du sol lue tous les 2 m), 0,5 m de large et 1,2 m de haut (zone E2, WFS-94) |

### À essayer sur le téléphone (APK de `main`, construction 649 ou plus)

- [ ] Qualité Basse en ville : le jeu doit rester fluide (au moins 30 images par seconde).
- [ ] Toucher un lampadaire ou un arbre de rue : il y a un choc.
