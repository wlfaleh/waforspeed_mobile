## Agent : Claude, agent 3 (01UY1Vv3)

**Ticket** : WFS-301 — règles : `regles-de-gestion/WFS-301.md`

## Description (dans la description de la PR)

- **Demande de l'utilisateur** (en une phrase) : voir la description de la PR
- **Ce qui change** (fichiers, comportement, à l'écran) : voir la description de la PR
- **Comment c'est vérifié** (tests CPU, parcours, bancs, mesures) : voir la description de la PR
- **Limites et risques connus** : voir la description de la PR

## Captures pour le lead dev (4 après)

Dossier : [`qa/pr/WFS-301`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-301/qa/pr/WFS-301) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `cuisson-paquet-corrompu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-301/qa/pr/WFS-301/avant-cuisson-paquet-corrompu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-301/qa/pr/WFS-301/apres-cuisson-paquet-corrompu.jpg?raw=true) |
| `cuisson-paquet-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-301/qa/pr/WFS-301/avant-cuisson-paquet-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-301/qa/pr/WFS-301/apres-cuisson-paquet-jour.jpg?raw=true) |
| `cuisson-paquet-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-301/qa/pr/WFS-301/avant-cuisson-paquet-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-301/qa/pr/WFS-301/apres-cuisson-paquet-nuit.jpg?raw=true) |
| `cuisson-sans-paquet` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-301/qa/pr/WFS-301/avant-cuisson-sans-paquet.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-301/qa/pr/WFS-301/apres-cuisson-sans-paquet.jpg?raw=true) |

## Sortie de WFS_BANC=cuire (RG-C1 à C3)

```
[info] cuire (WFS-301) : ville_astc.wfcarte : 22779180 octets, sha256 843a6e9c57c46e09e4f8b05dbaab520be69f0ec0e0a701117c1ec14ce03a3c7a, empreinte c30f0f40c276b6c1
[info] cuire (WFS-301) : ville_etc2.wfcarte : 5562514 octets, sha256 f8e3c56c2a9e1851416530e0165d98a220fe737a28b1f0b2d6ab8142d2946055, empreinte c30f0f40c276b6c1
[info] cuire (WFS-301) : ville_rgba8.wfcarte : 27593777 octets, sha256 f982838af5a22c2402d888b89107a4766bf3a44f727b0b930321bc27233cbf7a, empreinte c30f0f40c276b6c1
[info] cuire (WFS-301) : 0 écart(s) OK, paquets dans <WFS_PAQUETS>
code de sortie : 0 ; deux lancements de suite : fichiers identiques (cmp) ; dossier impossible à écrire : code 3
```
