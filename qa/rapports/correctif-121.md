# QA correctif CI configgame (PR #121) : valeurs de l'utilisateur reprises dans le code

## QA après fusion, recette de nuit, lot 4 (2026-11-06 08:30:09 UTC, qa-lead)

**Verdict : VALIDÉ**

- Sources : `configgame.json` et `src/core/configgame.h` de `main` (b20b6b1) ; journaux des jobs `texte` et `vitrines` du run 689 de `main` (eb8152a, vert, contient #121 et le correctif #122) ; journal du job `realisme` du run 693 (0d923d5) ; captures `poursuite-police`, `records-poursuite` de la Release `apercu` (construction 689).
- Fusion directe (correctif CI) : recette faite après la fusion, sur `main`.

### Points vérifiés

| Point | Preuve | Constat |
|---|---|---|
| Fichier de l'utilisateur et code concordent | `configgame.json` : `poursuite.score_depart` 1000, `camera.poursuite` recul 9, hauteur 6,3, regard devant 10, regard en hauteur −4,5, zoom 64 ; `configgame.h` : `scoreDepart = 1000`, `loin{9, 6.3, 10, −4.5}`, `fov[0] = 64`, `SCORE_POURSUITE_MAX = 1000` | OK, identiques |
| Lecture du fichier sans écart | journaux des bancs (689 et 693) : `configuration : configgame.json, 68 clé(s) lue(s), 0 valeur(s) du jeu, 0 erreur(s)` | OK |
| Poursuite à 1000 points au départ | journal `texte` (689) : `poursuite : graine 20261105, 10 km, score 1000, 2 voitures de police…` ; capture `poursuite-police` : SCORE 1000 au HUD | OK |
| Record de poursuite gardé après relance (borne 100 → 1000) | journal `vitrines` (689) : `records : sauvegarde relue : … poursuite 1;998;10000` puis `records relus après redémarrage (ok)` ; capture `records-poursuite` : « RECORD Victoire · 998 pts » | OK : un record de 998 (> 100) est relu |
| Tests CPU | journal `texte` (689) : `[OK] test_configgame`, `[OK] test_records` ; CI de `main` verte (689) | OK |
| Parcours course à 5 et poursuite | journal `texte` : `parcours course à 5 (RG-10) : 0 écart(s) OK` ; `vitrines` : `poursuite (parcours) : parcours complet et redémarrage : 0 écart(s) OK` | OK |
| Point à surveiller : appels de dessin en Basse pendant la poursuite | journal `vitrines` (689) : `écart maximal en qualité Basse : +45 appels de dessin (cible ≤ 60) : OK` | OK : le +90 signalé à la fusion n'est plus là (+45) |

### Anomalies

| ANO | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| — | — | — | Aucune anomalie |

### À essayer sur le téléphone (APK de `main`)

- [ ] Lancer une Poursuite : le score part de 1000.
- [ ] Finir une poursuite, fermer le jeu, le relancer : le record de poursuite est toujours là.
- [ ] La vue Poursuite est plus proche qu'avant (regard 10 m, zoom 64°).
