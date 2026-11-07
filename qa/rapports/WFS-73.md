# Recette QA — WFS-73 (conducteurs dans les voitures)

- **Verdict : PASSABLE AVEC ANOMALIES MINEURES** — QA après fusion (fusion par confiance, PR #178, squash 7f344b9), recette de nuit du 2026-11-07, `qa-lead`.
- **Source** : Release `apercu` de `main`, construction 809, commit 7f344b9 (692/692 captures à jour) ; run 809 vert.

| Critère | Capture | Constat |
|---|---|---|
| Conducteur assis face au volant, tête sous le pavillon, ni dans le toit ni flottant | `conducteurs-golf-jour`, `conducteurs-sf90-jour` (agrandies) | OK : Golf et SF90 (plus bas) |
| SUV : conducteur plus haut | `conducteurs-rrsport-jour` / `-nuit` | **Non vérifiable sur la capture** : vitres latérales avant du RR Sport noires et opaques, aucun conducteur visible |
| Vues de la caméra du joueur et du trafic | `camera-poursuite*`, `camera-proche*`, `conducteurs-golf-arriere-*` | OK |
| Aucun conducteur en vue cockpit ni au garage | `cockpit-golf-jour`, `garage-0` | OK |

## Anomalies
| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-110 | Mineure | `conducteurs-rrsport-jour`, `-nuit` | Vitres avant du RR Sport noires et opaques : le conducteur ajouté n'est pas visible, alors que les vraies vitres avant sont claires (seules les arrière sont teintées). **Correction** : matériau des vitres avant du RR Sport transparent (teinte légère), comme la Golf et la SF90 |
