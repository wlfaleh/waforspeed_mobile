# QA WFS-49 : Banc, captures du chargement en 2400x1080 sans planche de texte (correction ANO-10)

**Verdict : VALIDÉ** (1er passage), banc d'essai PC. **Fusion autorisée** (PR #22).
Aucune anomalie, bloquante ou mineure, donc rien à ajouter à `anno.md`. ANO-10 est corrigée.

- Construction regardée : commit `52648fd` (`feature/waforspeed-49`), PR #22. Run Actions 41639535304 réussi
  (android, banc-essai, publication). Release `apercu-waforspeed-49` (« commit 52648fd »), 50 captures.
- Référence de non-régression : Release `apercu-waforspeed-51` (commit `04ec9f5`). `git diff --stat 04ec9f5 0b414ab -- . ':!*.md'`
  est vide : le code est identique à la base de la branche (`git merge-base` = `0b414ab`). Cette référence porte donc les
  captures que fait `main` aujourd'hui.
- Diff relu : seul `.github/workflows/build.yml` change (+3/−1). Les `chargement-*-2400` viennent d'un nouveau lancement
  `WFS_TAILLE=2400x1080 ./build/wfs_bench chargement`, sans `WFS_TEXTE_ESSAI`. Les deux lancements « texte » restent
  inchangés. Aucun fichier de `src/**`.

| Critère / règle | Résultat |
|---|---|
| Critère : `chargement-fondu-2400` | OK. Sur `main`, le panneau « DÉMO DU TEXTE » (pangramme, accents, chrono, « 182 KM/H ») chevauchait le fondu. Il n'y est plus : on voit le titre WAFOR SPEED qui s'efface, la barre à 100 % « Construction de la ville 100 % », la Golf bleue et la rue derrière le voile. L'image n'est pas déformée (2400x1080). C'est la seule capture qui change par rapport à `main`. |
| Critère : `chargement-0/1/2/erreur-2400` | OK. Pas de panneau (0 %, 20 % « Chargement du modèle 3D… », 100 % « Construction de la ville », message d'erreur rouge). Ces 4 captures sont identiques au pixel près à `main` : la planche de texte n'est dessinée qu'en course, donc elle n'apparaissait que sur le fondu. |
| Critère / RG-02 : `texte-essai-1280`, `texte-essai-2400` | OK. Identiques au pixel près à `main`. La planche « DÉMO DU TEXTE » est toujours là, au même endroit. |
| RG-02 : autres captures | OK. Les 49 autres captures (1280x720, vitrines, course, trafic, ombres, cartes plage, HUD, encoche, `hud-2400`) sont identiques au pixel près à `main` (comparaison `cmp`). |
| RG-03 : durée de la CI | OK. Un seul lancement du banc en plus (`wfs_bench chargement`). |

## Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| — | — | — | Aucune |
