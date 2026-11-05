# QA WFS-99 : garage lisible

## QA après fusion, recette de nuit (2026-11-04 23:53:28 UTC, qa-lead)

**Verdict : PASSABLE AVEC ANOMALIES MINEURES**

- **Captures regardées** : Release `apercu` de `main` (construction 562, commit `a256432`, après la fusion `c1f5bbb`), en 1280x720 et 2400x1080.
- Tous les points demandés sont conformes. Il reste une anomalie mineure (petit texte de l'étiquette) qui va au tableau `anno.md`, puisque le ticket est déjà fusionné.

### Points vérifiés

| Point | Capture | Constat |
|---|---|---|
| Aucun néon sur le titre, l'accroche ni la barre d'index | `garage-0`, `garage-m4`, `garage-sf90`, `garage-p208`, `garage-1-2400` | OK. Le premier néon est à x ≈ 686/1280 (0,54) et à 1275/2400 (0,53) ; voile sombre à gauche |
| Voiture de face (3/4 avant) au départ | `garage-0`, `garage-sf90`, `garage-p208` | OK |
| Angle libre après 2 glissés | `garage-1`, `garage-2`, `garage-m4` | OK (profil arrière, comme au web) |
| Taches de lumière au sol | `spots-sol` | OK : orangée à gauche, cyan à droite, claire devant ; discrètes au garage (RG-03) |
| Contre-jour cyan sur la M4 noire | `garage-m4` | OK : liseré cyan sur le toit et le haut de caisse, voiture noire lisible |
| Panneau, flèches, boutons, chevrons | toutes | OK : panneau d'un seul tenant, coin coupé net, aucune rayure (ANO-20) |
| Crédit en bas à droite | `garage-0`, `garage-1-2400` | OK : plus grand, gris #8A90A6, lisible |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-47 | Mineure | `garage-sf90` (et `nitro-lignes`, voir WFS-101) | En 720p, les très petits libellés en capitales (≈ 6 px) perdent des traits fins : l'étiquette « HYPERCAR HYBRIDE » s'affiche « HYPERCAR HYBR DE » (le I disparaît). On voit la même chose sur la jauge « NITRO » pendant la nitro (le T est à moitié effacé). **Correction** : placer ces libellés au pixel entier (pas de position fractionnaire) ou passer l'étiquette à 8 px minimum en 720p |
| (ANO-21) | Bloquante, déjà planifiée | `garage-0`, `garage-p208`, `garage-1-2400` | Points cyan qui flottent au-dessus du toit → WFS-100 (en cours), rien de nouveau |
