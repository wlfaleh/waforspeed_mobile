# QA WFS-91 : banc d'endurance (ANO-52)

## QA après fusion, recette de nuit (2026-11-05 19:05:21 UTC, qa-lead, accent qualité, design et parcours sur téléphone Android)

**Verdict : VALIDÉ**

- Sources : Release `apercu` de `main` (construction 649, commit `894208c`, run 42083700609 vert au 2e essai ; 279/279 captures à jour), journaux des jobs de ce run, APK de la Release, compilation et 25 tests CPU en local sur `origin/main` (`7e6414b`).
- Fusion par confiance : recette faite après la fusion ; une anomalie mineure va au tableau `anno.md`, une bloquante deviendrait un ticket de correction prioritaire (PO).
- ANO-52 passe en ✅ Vérifiée (0 erreur de validation sans contournement).

### Points vérifiés

| Point | Capture / preuve | Constat |
|---|---|---|
| Job `endurance` de main vert | run 649 | OK au 2e essai (7 min 43 s) ; le 1er essai a échoué pendant `apt-get` (miroir Ubuntu bloqué, rien à voir avec le jeu) |
| 8 cycles, 3 retours d'arrière-plan | journal du job | OK : 10 cycles (8 Checkpoint, Course à 5, Poursuite), 3 retours |
| Mémoire ≤ +10 % | journal du job | OK : +1,5 % |
| 0 erreur de validation inconnue | journal du job | OK : 0 (dont 0 connues) |

### Anomalies

Aucune anomalie.

### À essayer sur le téléphone (APK de `main`, construction 649 ou plus)

- [ ] En course, passer l'appli en arrière-plan puis revenir, 3 fois : pas de plantage, la course reprend en pause.
- [ ] Même chose au garage et au menu.
